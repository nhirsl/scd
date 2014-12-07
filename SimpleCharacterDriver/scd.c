/*
 * scd.c -- char module implementation
 *
 * Copyright (C) 2014 Nemanja Hirsl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Mandatory for all device drivers. */
#include <linux/init.h>		/* Init/release handling. */
#include <linux/module.h>	/* Module macros etc. */

#include <linux/fs.h>		/* File operations, file handling etc. */
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/slab.h>		/* For memory usage (kmalloc). */
#include <linux/sched.h>	/* For event waiting. */
#include <linux/cdev.h>		/* For Charater device handling. */
#include <linux/semaphore.h>

#include "scd.h"		/* Local definitions. */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nemanja Hirsl");
MODULE_DESCRIPTION("Simple Exchange Driver");

/* Structure which represents our device. */
struct scd_device {
	wait_queue_head_t in_queue, out_queue;	/* read and write queues */
	char *begin, *end;	/* begin of buffer, end of buffer */
	char *read, *write;	/* where to read, where to write */
	int buffersize;		/* size of the buffer */
	int nreaders, nwriters;	/* number of openings for read and write */
	struct semaphore sem;	/* mutual exclusion semaphore */
	struct cdev cdev;	/* char device structure */
};

/* SCD specific globals. */
static struct scd_device *scd_devices;
static int scd_major = SCD_MAJOR;
static int scd_minor = SCD_MINOR;
static int scd_dev_n = SCD_DEV_N;
static int scd_buff_sz = SCD_BUFFER_SIZE;

/* Forward declarations of helper functions. */
static void scd_setup_cdev(struct scd_device *dev, int next);
static int freespace(const struct scd_device *dev);

/* Module parameters assignable at load time. */
module_param(scd_major, int, S_IRUGO);
module_param(scd_minor, int, S_IRUGO);
/*module_param(scd_dev_n, int, S_IRUGO);  TODO. For now, keep it 1 by default. */

/* Initialization and deinitialization. */
static int __init scd_init_module(void)
{
	int err, i;
	dev_t dev = 0;

	printk(KERN_DEBUG "scd_init_module. Major: %d, minor: %d\n", scd_major,
	       scd_minor);

	/* Create dynamic major unless set differnetly at load time. */
	if (scd_major) {
		dev = MKDEV(scd_major, scd_minor);
		err = register_chrdev_region(dev, scd_dev_n, "scd");
	} else {
		err = alloc_chrdev_region(&dev, scd_minor, scd_dev_n, "scd");
		scd_major = MAJOR(dev);
	}
	if (err < 0) {
		printk(KERN_WARNING
		       "scd_init_module: error getting major: %d, err: %d\n",
		       scd_major, err);
		return err;
	}

	/* Allocate devices as the number can be specified at load time. */
	scd_devices =
	    kmalloc(scd_dev_n * sizeof(struct scd_device), GFP_KERNEL);
	if (!scd_devices) {
		printk(KERN_WARNING
		       "scd_init_module: Can't allocate memory for device.\n");
		unregister_chrdev_region(dev, scd_dev_n);
		return -ENOMEM;
	}
	memset(scd_devices, 0, scd_dev_n * sizeof(struct scd_device));

	/* Initialize each device. */
	for (i = 0; i < scd_dev_n; ++i) {
		printk(KERN_DEBUG "scd_init_module. init device: %d\n", i);
		init_waitqueue_head(&(scd_devices[i].in_queue));
		init_waitqueue_head(&(scd_devices[i].out_queue));
		sema_init(&scd_devices[i].sem, 1);
		scd_setup_cdev(scd_devices + i, i);
	}

	return 0;
}

static void __exit scd_exit_module(void)
{
	int i;
	dev_t devno = MKDEV(scd_major, scd_minor);

	printk(KERN_DEBUG "scd_exit_module. Major: %d, minor: %d\n", scd_major,
	       scd_minor);

	if (!scd_devices) {
		printk(KERN_WARNING
		       "scd_exit_module: Device was not allocated. Exiting.\n");
		return;
	}

	/* Deallocate memory dor each device. */
	for (i = 0; i < scd_dev_n; ++i) {
		cdev_del(&scd_devices[i].cdev);
		kfree(scd_devices[i].begin);
	}
	kfree(scd_devices);
	scd_devices = NULL;

	unregister_chrdev_region(devno, scd_dev_n);

	return;
}

module_init(scd_init_module);
module_exit(scd_exit_module);

/* Open and release. */
static int scd_open(struct inode *inode, struct file *filp)
{
	struct scd_device *dev;

	/* Find and save device data (scd_device) for future use. */
	dev = container_of(inode->i_cdev, struct scd_device, cdev);
	filp->private_data = dev;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	/* Allocate memory for buffer. */
	if (!dev->begin) {
		dev->begin = kmalloc(scd_buff_sz, GFP_KERNEL);
		if (!dev->begin) {
			printk(KERN_WARNING
			       "scd_open. Can't allocate memory for buffer.\n");
			up(&dev->sem);
			return -ENOMEM;
		}
	}

	/* Set values for scd_device. */
	dev->buffersize = scd_buff_sz;
	dev->end = dev->begin + dev->buffersize;
	dev->read = dev->write = dev->begin;
	if (filp->f_mode & FMODE_READ)
		++dev->nreaders;
	if (filp->f_mode & FMODE_WRITE)
		++dev->nwriters;

	up(&dev->sem);

	return nonseekable_open(inode, filp);
}

static int scd_release(struct inode *inode, struct file *filp)
{
	struct scd_device *dev = filp->private_data;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	/* Free allocated space on release. */
	if (filp->f_mode & FMODE_READ)
		--dev->nreaders;
	if (filp->f_mode & FMODE_WRITE)
		--dev->nwriters;
	if (dev->nreaders == 0 && dev->nwriters == 0) {
		kfree(dev->begin);
		dev->begin = NULL;
	}

	up(&dev->sem);

	return 0;
}

/* Read and Write. */
static ssize_t scd_read(struct file *filp, char __user * buf, size_t count,
			loff_t * f_pos)
{
	struct scd_device *dev = filp->private_data;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	/* If there is nothing to read release mutex and handle non-blocking case.
	 * For blocking case: Block (wait) for data to become available for read and
	 * obtain semaphore before checking condition again.
	 */
	while (dev->read == dev->write) {
		printk(KERN_DEBUG "scd_read. Nothing to read.\n");
		up(&dev->sem);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		if (wait_event_interruptible
		    (dev->in_queue, (dev->read != dev->write)))
			return -ERESTARTSYS;
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
	}

	/* There is data available to read and we hold mutex. */
	if (dev->write > dev->read)	/* Write ahead of read. We can read at most up to write pointer. */
		count = min(count, (size_t) (dev->write - dev->read));
	else			/* Write has wrapped. Return data up to the end of buffer. */
		count = min(count, (size_t) (dev->end - dev->read));

	/* Copy data to user space. */
	if (copy_to_user(buf, dev->read, count)) {	/* Returns number of bytes that could not be copied. On success, zero. */
		up(&dev->sem);
		return -EFAULT;
	}

	/* Adjust read pointer to reflect read data. */
	dev->read += count;
	if (dev->read == dev->end)
		dev->read = dev->begin;

	up(&dev->sem);

	/* Wake up any writers. */
	wake_up_interruptible(&dev->out_queue);
	return count;
}

static ssize_t scd_write(struct file *filp, const char __user * buf,
			 size_t count, loff_t * f_pos)
{
	struct scd_device *dev = filp->private_data;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	/* If there is no available space for writting release mutex and handle non-blocking case.
	 * For blocking case: Block (wait) for free space to become available and
	 * obtain semaphore before checking condition again.
	 * The buffer is full if Write is just behind of Read.
	 */
	while (!freespace(dev)) {
		printk(KERN_DEBUG
		       "scd_write. No freespace available for writting.\n");
		up(&dev->sem);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		if (wait_event_interruptible(dev->out_queue, freespace(dev)))
			return -ERESTARTSYS;
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
	}

	/* There is space available for writting. */
	if (dev->write >= dev->read) {	/* Write is ahead of read. */
		if (dev->read == dev->begin)	/* If Read is at the beggining, do not wrap write pointer. */
			count =
			    min(count, (size_t) (dev->end - dev->write - 1));
		else		/* Write to the end of buffer (and wrap). */
			count = min(count, (size_t) (dev->end - dev->write));
	} else			/* Write has wrapped. Write up to the read pointer (-1). */
		count = min(count, (size_t) (dev->read - dev->write - 1));

	if (copy_from_user(dev->write, buf, count)) {	/* Returns number of bytes that could not be copied. On success, zero. */
		up(&dev->sem);
		return -EFAULT;
	}

	/* Adjust write pointer to reflect written data. */
	dev->write += count;
	if (dev->write == dev->end)
		dev->write = dev->begin;
	up(&dev->sem);

	/* Wake up any readers. */
	wake_up_interruptible(&dev->in_queue);
	return count;
}

/* Poll and Ioctl. */
static unsigned int scd_poll(struct file *filp, poll_table * wait)
{
	struct scd_device *dev = filp->private_data;
	unsigned int mask = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	/* Add wait queues to the poll_table. */
	poll_wait(filp, &dev->in_queue, wait);
	poll_wait(filp, &dev->out_queue, wait);

	/* Set mask to reflect inner state: 
	 * if there is anything to read or space available for writting. 
	 */
	if (dev->read != dev->write)
		mask |= POLLIN | POLLRDNORM;
	if (freespace(dev))
		mask |= POLLOUT | POLLWRNORM;

	up(&dev->sem);
	return mask;
}

static long scd_unlocked_ioctl(struct file *filp, unsigned int cmd,
			       unsigned long arg)
{
	struct scd_device *dev = filp->private_data;
	int err = 0;

	/* Don't act on wrong cmds. 
	 * Return EINVAL (invalid argument). May return ENOTTY (inappropriate ioctl) instead.
	 */
	if (_IOC_TYPE(cmd) != SCD_IOMAGIC)
		return -EINVAL;
	if (_IOC_NR(cmd) > SCD_IOMAX)
		return -EINVAL;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	switch (cmd) {
		/* Reset. Consider checking if user has capability to alter the size. */
	case SCD_IORBSIZE:
		/*if (!capable(CAP_SYS_ADMIN))
		   err = -EPERM;
		   else */
		scd_buff_sz = SCD_BUFFER_SIZE;
		break;
		/* Get. */
	case SCD_IOGBSIZE:
		if ((err = put_user(scd_buff_sz, (int __user *)arg)) != 0)
			printk(KERN_WARNING
			       "scd_unlocked_ioctl. Error %d on put_user\n",
			       err);
		break;
		/* Set. Consider checking if user has capability to alter the size. */
	case SCD_IOSBSIZE:
		/*if (!capable(CAP_SYS_ADMIN))
		   err = -EPERM;
		   else */
		if ((err = get_user(scd_buff_sz, (int __user *)arg)) != 0)
			printk(KERN_WARNING
			       "scd_unlocked_ioctl. Error %d on get_user\n",
			       err);
		break;
	}

	up(&dev->sem);
	return err;
}

/* The file operations for the device. */
static struct file_operations scd_pipe_fops = {
	.owner = THIS_MODULE,
	.read = scd_read,
	.write = scd_write,
	.poll = scd_poll,
	.unlocked_ioctl = scd_unlocked_ioctl,
	.open = scd_open,
	.release = scd_release,
	.llseek = no_llseek,
};

/* Helper functions. */
/* Set up cdev entry. */
static void scd_setup_cdev(struct scd_device *dev, int next)
{
	int err;
	dev_t devno = MKDEV(scd_major, scd_minor + next);

	cdev_init(&dev->cdev, &scd_pipe_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scd_pipe_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_WARNING "Error %d adding scd %d\n", err, next);
}

/* Returns avail free space. Should be called with decremented semaphore. */
static int freespace(const struct scd_device *dev)
{
	int space = 0;

	/* Calculate how much space is available for writting in circular buffer.
	 * The buffer is full if Write is just behind of Read.
	 * 1. Read and Write are on the same position, nothing to read: Available space for writting is buffersize - 1.
	 * 2. Read is ahead of Write. Available space is just up to the read pointer.
	 * 3. Write is ahead of Read. Available space is everything except non read part.
	 */
	if (dev->read == dev->write)
		space = dev->buffersize - 1;
	else if (dev->read > dev->write)
		space = dev->read - dev->write - 1;
	else			/* dev->write > dev->read */
		space = dev->buffersize - (dev->write - dev->read) - 1;

	return space;
}

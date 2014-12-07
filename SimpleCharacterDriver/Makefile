obj-m := scd.o 
SOURCE	:= scd.h scd.c

# KDIR is the location of the kernel source.
KDIR  := /lib/modules/$(shell uname -r)/build

# PWD is the current working directory
PWD   := $(shell pwd)

# default is the default make target.
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c modules.order Module.symvers .tmp_versions

beautify:
	indent -linux $(SOURCE)

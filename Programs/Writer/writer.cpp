/*
 * writer.cpp -- Example program which writes to SimpleExchangeDevice
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <cstring>

#include "writer.h"
#include "../log.h"

SEDWriter::SEDWriter(std::string device)
{
    watchFd = inotify_init();
    memset(buffer, 0, sizeof buffer);

    sedFd = open(device.c_str(), O_WRONLY);
    if (sedFd == -1) {
        LOGDEBUG() << "Error opening file " << device;
    }

    Log::LLevel() = DEBUG;
}

SEDWriter::~SEDWriter()
{
    inotify_rm_watch(watchFd, watch);
    close(watchFd);
    close(sedFd);
}

bool SEDWriter::copyFileToDevice(const char *source)
{
    LOGDEBUG() << "Copying from: " << source;
    int readFd = open(source, O_RDONLY);
    if (readFd == -1) {
        LOGDEBUG() << "Error opening file " << source << "errno: " << errno;
        return false;
    }

    bool ret = true;
    const int sz = 1024;
    char data[sz];
    ssize_t numRead = 0;
    while ((numRead = read(readFd, data, sz)) > 0) {
        int lenW = numRead;
        ssize_t countW = 0;
        char* currentPos = data;
        while (1) {
            countW = write(sedFd, currentPos, lenW);
            if (countW < 0) {/* Error occured. */
                ret = false;
                LOGDEBUG() << "Error writing to sed ";
                break;
            } else if (countW == lenW) /* Everything was written. */
                break;
            else { /* countW > 0 && countW < lenW.*/
                lenW -= countW;
                currentPos += countW;
            }
        }
    }

    close(readFd);
    return ret;
}

bool SEDWriter::Watch(std::string folder)
{
    watch = inotify_add_watch(watchFd, folder.c_str(), IN_CREATE);
    int length = read(watchFd, buffer, BUF_LEN);
    if (length < 0) {
        LOGERR() << "Error reading inotify. ";
        return false;
    }

    int i = 0;
    while (i < length) {
        struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
        if (event->len) {
            if (event->mask & IN_CREATE) {
                if (event->mask & IN_ISDIR) {
                    LOGDEBUG() << "Directory created" << event->name;
                } else {
                    LOGDEBUG() << "File created" << event->name;
                }
                if (copyFileToDevice(event->name)) {
                    LOGDEBUG() << "Successfull copied from: " << event->name;
                } else {
                    LOGDEBUG() << "Error copying: " << event->name;
                }
                break; /* Copy just one file and exit. */
            }
        }
        i += EVENT_SIZE + event->len;
    }
}

int main( int argc, char **argv )
{
    SEDWriter sedWriter("/dev/sed");;
    sedWriter.Watch("./");

    return 0;
}

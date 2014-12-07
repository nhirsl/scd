/*
 * integration_test.cpp -- Tests for SimpleCharacterDriver
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

#include <gtest/gtest.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include "../../SimpleCharacterDriver/scd.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>
#include <climits>

class ScdIntegrationTests : public ::testing::TestWithParam<int>
{
protected:
    ScdIntegrationTests() : device_name ("/dev/scd")
    {
    }

    virtual ~ScdIntegrationTests()
    {
    }

    virtual void SetUp()
    {
        fd = open (device_name.c_str(), O_RDWR);
        ASSERT_NE(fd, -1);
        pfd.fd = fd;
    }

    virtual void TearDown()
    {
        /* Reset buffer size to default. */
        int err = ioctl(fd, SCD_IORBSIZE);
        ASSERT_NE(err, -1);
        /* Close file. */
        err = close(fd);
        ASSERT_EQ(err, 0);
    }

    const std::string device_name;
    int fd;
    struct pollfd pfd;
};

TEST_F(ScdIntegrationTests, ReadWrite)
{
    std::string str ("Test");

    std::thread t([&]() {
        int len = str.length();
        int err = write(fd, str.c_str(), len);
        EXPECT_NE(err, -1);
    });

    char buff[100];
    memset(buff, 0, sizeof buff);
    int err = read(fd, buff, sizeof buff);
    EXPECT_NE(err, -1);
    EXPECT_EQ(str, buff);

    t.join();
}

TEST_F(ScdIntegrationTests, PollRead)
{
    pfd.events = POLLIN;
    std::atomic_bool do_write;
    std::string str ("Test");

    std::thread t([&]() {
        do_write = true;
        while (do_write) {
            /* Give a chance to main thread to block on poll. */
            std::chrono::milliseconds dura( 2000 );
            std::this_thread::sleep_for( dura );

            if (do_write) {
                int len = str.length();
                int err = write(fd, str.c_str(), len);
                EXPECT_NE(err, -1);
            }
        }
    });

    /* Wait for 10 seconds. If TO occurs we'll set do_write so the thread can exit.*/
    int err = poll(&pfd, 1, 10000);
    /* Error if TO occurs. */
    EXPECT_GT(err, 0);

    do_write = false;

    if (pfd.revents & POLLIN) {
        char buff[100];
        memset(buff, 0, sizeof buff);
        err = read(fd, buff, sizeof buff);
        EXPECT_NE(err, -1);
        EXPECT_EQ(str, buff);
    }
    t.join();
}

TEST_F(ScdIntegrationTests, PollWrite)
{
    pfd.events = POLLOUT;
    std::atomic_bool do_read;
    std::string str ("Test");

    std::thread t([&]() {
        /* Wait for 10 seconds. */
        int err = poll(&pfd, 1, 10000);
        /* Error if TO occurs. */
        EXPECT_GT(err, 0);

        if (pfd.revents & POLLOUT) {
            int len = str.length();
            err = write(fd, str.c_str(), len);
            EXPECT_NE(err, -1);
            do_read = true;
        }
    });

    /* Give a chance to the thread to poll. */
    std::chrono::milliseconds dura( 2000 );
    std::this_thread::sleep_for( dura );

    char buff[100];
    memset(buff, 0, sizeof buff);
    int err = read(fd, buff, sizeof buff);
    EXPECT_NE(err, -1);
    EXPECT_EQ(str, buff);

    t.join();
}

INSTANTIATE_TEST_CASE_P(IntegrationTests, ScdIntegrationTests, ::testing::Values(1, 2, 100, 101, 16384, 16385, 32768, 32769, 65536, 65537));
TEST_P(ScdIntegrationTests, ReadWriteExchange)
{
    pfd.events = POLLIN;
    const int size = GetParam();;
    const int size_read = 100;
    char w_buff[size];
    char r_buff[size_read];

    memset(w_buff, 0, sizeof w_buff);
    memset(r_buff, 0, sizeof r_buff);

    std::thread t([&]() {
        int w_len = size;
        int w_count = 0;
        char* current_pos = w_buff;
        while (1) {
            w_count = write(fd, current_pos, w_len);
            EXPECT_NE(w_count, -1);
            if (w_count <= 0) /* Nothing was written or error occured. */
                break;
            else if (w_count == w_len) /* Everything was written. */
                break;
            else { /* w_count > 0 && w_count < w_len.*/
                w_len -= w_count;
                current_pos += w_count;
            }
        }
    });

    /* Wait for 10 seconds. If TO occurs we'll set do_write so the thread can exit.*/
    int err = poll(&pfd, 1, 10000);
    /* Error if TO occurs. */
    EXPECT_GT(err, 0);

    if (pfd.revents & POLLIN) {
        int r_len = size_read;
        int r_count = 0;
        while (r_count < size) {
            int ret = read(fd, r_buff, r_len);
            EXPECT_NE(ret, -1);
            if (ret <= 0)
                break;
            else { /* r_count > 0 && r_count <= r_len.*/
                r_count += ret;
            }
        }
        EXPECT_EQ(r_count, size);
    }
    t.join();
}


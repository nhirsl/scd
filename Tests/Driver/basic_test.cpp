/*
 * basic_test.cpp -- Tests for SimpleCharacterDriver
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

#include "../../SimpleCharacterDriver/scd.h"

class ScdBasicTests : public ::testing::Test
{
protected:
    ScdBasicTests() : device_name ("/dev/scd")
    {
    }

    virtual ~ScdBasicTests()
    {
    }

    virtual void SetUp()
    {
        fd = open (device_name.c_str(), O_RDONLY);
        ASSERT_NE(fd, -1);
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
};

TEST_F(ScdBasicTests, OpenCloseRDONLY)
{
    int err;

    std::string str ("Test");
    int len = str.length();
    err = write(fd, str.c_str(), len);
    EXPECT_EQ(err, -1);
}

TEST_F(ScdBasicTests, GetIOCTL)
{
    int err;

    /* Get buffer size and verify its value. */
    int buf_sz;
    err = ioctl(fd, SCD_IOGBSIZE, &buf_sz);
    EXPECT_NE(err, -1);

    EXPECT_EQ(SCD_BUFFER_SIZE, buf_sz);
}

TEST_F(ScdBasicTests, SetIOCTL)
{
    int err;

    /* Set new buffer size. */
    int buf_sz = 0x4000;
    err = ioctl(fd, SCD_IOSBSIZE, &buf_sz);
    EXPECT_NE(err, -1);

    /* Verify that new size has been set. */
    int get_buf_sz;
    err = ioctl(fd, SCD_IOGBSIZE, &get_buf_sz);
    EXPECT_NE(err, -1);

    EXPECT_EQ(get_buf_sz, buf_sz);
}

TEST_F(ScdBasicTests, SetIOCTLOpen)
{
    int err;

    /* Set new buffer size. */
    int buf_sz = 0x4000;
    err = ioctl(fd, SCD_IOSBSIZE, &buf_sz);
    EXPECT_NE(err, -1);

    /* Verify that new size has been set. */
    int get_buf_sz;
    err = ioctl(fd, SCD_IOGBSIZE, &get_buf_sz);
    EXPECT_NE(err, -1);

    EXPECT_EQ(get_buf_sz, buf_sz);

    int fd2 = open (device_name.c_str(), O_RDONLY);
    ASSERT_NE(fd, -1);

    /* Verify that new size has been set. */
    get_buf_sz;
    err = ioctl(fd2, SCD_IOGBSIZE, &get_buf_sz);
    EXPECT_NE(err, -1);

    EXPECT_EQ(get_buf_sz, buf_sz);
}

TEST_F(ScdBasicTests, ResetIOCTL)
{
    int err;

    /* Set buffer size which differs from default. */
    int buf_sz = 0x4000;
    err = ioctl(fd, SCD_IOSBSIZE, &buf_sz);
    EXPECT_NE(err, -1);

    /* Verify new size. */
    int get_buf_sz;
    err = ioctl(fd, SCD_IOGBSIZE, &get_buf_sz);
    EXPECT_NE(err, -1);

    EXPECT_EQ(get_buf_sz, buf_sz);

    /* Reset buffer size to default. */
    err = ioctl(fd, SCD_IORBSIZE);
    EXPECT_NE(err, -1);

    /* Get new value and verify it is the same as default. */
    err = ioctl(fd, SCD_IOGBSIZE, &get_buf_sz);
    EXPECT_NE(err, -1);

    EXPECT_EQ(SCD_BUFFER_SIZE, get_buf_sz);
}


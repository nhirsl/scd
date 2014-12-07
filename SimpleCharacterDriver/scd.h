/*
 * scd.h -- definitions for the char module
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

#ifndef SCD_MAJOR
#define SCD_MAJOR 0		/* Major numberm, dynamic by default */
#endif

#ifndef SCD_MINOR
#define SCD_MINOR 0		/* Minor numberm, 0 default */
#endif

#ifndef SCD_DEV_N
#define SCD_DEV_N 1		/* Number of devices, 1 by default */
#endif

#ifndef SCD_BUFFER_SIZE
#define SCD_BUFFER_SIZE 0x8000	/* Circular buffer size. 32K by default. */
#endif

#define SCD_IOMAGIC 0xDE
#define SCD_IORBSIZE _IO(SCD_IOMAGIC, 0)
#define SCD_IOGBSIZE _IO(SCD_IOMAGIC, 1)
#define SCD_IOSBSIZE _IO(SCD_IOMAGIC, 2)
#define SCD_IOMAX 2

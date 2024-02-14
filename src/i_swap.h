//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Endianess handling, swapping 16bit and 32bit.
//


#ifndef __I_SWAP__
#define __I_SWAP__

// Endianess handling.
// WAD files are stored little endian.

// These are deliberately cast to signed values; this is the behaviour
// of the macros in the original source and some code relies on it.

static inline short le16_(short x) {
    const unsigned char *xb = ((const unsigned char *) &x);
    return (xb[1] << 8) | xb[0];
}

static inline int le32_(int x) {
    const unsigned char *xb = ((const unsigned char *) &x);
    return (xb[3] << 24) | (xb[2] << 16) | (xb[1] << 8) | xb[0];
}

#define SHORT(x)  ((signed short) le16_(x))
#define LONG(x)   ((signed int) le32_(x))

// Defines for checking the endianness of the system.

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SYS_BIG_ENDIAN
#endif

#endif


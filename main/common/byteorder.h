/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// provide swapping for conversion between little / big endian
// inspired by byteorder/swab.h from the Linux kernel

#ifndef _BYTEORDER_H_
#define _BYTEORDER_H_

#define swap_short(x) \
                ((((x) & (unsigned short)0x00ffU) << 8) | \
                (((x) & (unsigned short)0xff00U) >> 8) )

#define swap_int(x) \
                ((((x) & (unsigned int)0x000000ffUL) << 24) | \
                (((x) & (unsigned int)0x0000ff00UL) <<  8) | \
                (((x) & (unsigned int)0x00ff0000UL) >>  8) | \
                (((x) & (unsigned int)0xff000000UL) >> 24) )

inline bool cpu_little_endian() // verbose way of testing
{
  unsigned const int i = 0x01020304;
  unsigned char *p = (unsigned char *) &i;

  return p[0] == 4 && p[1] == 3 && p[2] == 2 && p[3] == 1;
}

#ifdef DEBUG_BYTEORDER

inline void print_verbosely(int i)
{
  unsigned char *p = (unsigned char *) &i;
  for(int j = 0; j < 4; j++) printf("%02x ",(int) p[j]);
  printf("[%d]\n", i);
}

inline void endian_test()
{
  int t = 0x03422711;

  printf("native:  "); print_verbosely(t);
  printf("swapped: "); print_verbosely(swap_int(t));
}

// x86:
// native:  11 27 42 03 [54667025]
// swapped: 03 42 27 11 [287785475]

// sparc:
// native:  03 42 27 11 [54667025]
// swapped: 11 27 42 03 [287785475]

#endif

#endif

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

/** \file mfile.h 
 * \c FILE like interface to strings in memory, similar to C++ ostringstreams
 */

#ifndef _MFILE_H_
#define _MFILE_H_

#include <stdarg.h>

#define MFILE_BUFF_SIZE 1048576
#define MFILE_MAX_LINE 524288

#ifdef __cplusplus
extern "C" {
#endif                                                                        

/** file like structure in memory, similar to ostringstreams in C++ */
struct MFILE
{
  int size;
  char *buff;
  char *ptr;
};

struct MFILE *mopen();
void mclose( struct MFILE *f );
void mflush( struct MFILE *f );
int mprintf( struct MFILE *f, const char *format, ... );
int vmprintf( struct MFILE *f, const char *format, va_list ap );
int mlength( struct MFILE *f );
char *mstring(struct MFILE *);

#ifdef __cplusplus
}
#endif

#endif

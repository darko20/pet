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

/* pseudo files kept in memory */

#include <stdlib.h>
#ifdef SMARTHEAP
#include <smrtheap.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mfile.h"

extern FILE *ferr;

struct MFILE *mopen()
{
  struct MFILE *f;

  f = (struct MFILE *) malloc( sizeof( struct MFILE ) );
  assert( f != NULL );
  
  f->size = MFILE_BUFF_SIZE;
  f->ptr = f->buff =(char *)  malloc( f->size );

  assert( f->buff != NULL );

  return f;
}

void mflush( struct MFILE *f )
{
  f->ptr = f->buff;
  f->ptr[ 0 ] = '\0';
}

int mlength( struct MFILE *f )
{
  return f->ptr - f->buff;
}

char *mstring(struct MFILE *file) {
  return(file->buff);
} /* mstring() */

void mclose( struct MFILE *f )
{
  free( f->buff );
  free( f );
}

int vmprintf( struct MFILE *f, char *format, va_list ap )
{
  int n;

  if(f->size - (f->ptr - f->buff) < MFILE_MAX_LINE)
    {
      n = (f->ptr - f->buff);
      f->size += MFILE_BUFF_SIZE;
      
      f->buff = (char *) realloc( f->buff, f->size );
      assert( f->buff != NULL );
      
      f->ptr = f->buff + n;
    }

  n = vsprintf( f->ptr, format, ap );

  if (n >= MFILE_MAX_LINE)
    {
      fprintf( ferr, "mprintf(): line too long for mprintf (%s)...\n", format );
      exit(1);
    }
  
  f->ptr += n;

  return n;
}

int mprintf( struct MFILE *f, char *format, ... )
{
  va_list ap;
  int n;

  va_start(ap, format);
  n = vmprintf( f, format, ap );
  va_end( ap );

  return n;
}



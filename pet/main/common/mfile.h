/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* FILE lile interface to strings in memory */

#ifndef _MFILE_H_
#define _MFILE_H_

#include <stdarg.h>

#define MFILE_BUFF_SIZE 65536
#define MFILE_MAX_LINE 31744

struct MFILE
{
  int size;
  char *buff;
  char *ptr;
};

struct MFILE *mopen();
void mclose( struct MFILE *f );
void mflush( struct MFILE *f );
int mprintf( struct MFILE *f, char *format, ... );
int vmprintf( struct MFILE *f, char *format, va_list ap );
int mlength( struct MFILE *f );
char *mstring(struct MFILE *);
#endif

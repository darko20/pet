/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* object representing a binary dump 
 * abstracts away from low level representation (byteorder etc)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "byteorder.h"
#include "dumper.h"

#define DUMP_LITTLE_ENDIAN true

#define BUFF_SIZE (128 * 1024)

dumper::dumper(FILE *f, bool write)
{
  _f = f;
  _buff = 0;
  _coe = false;
  _write = write;
  _swap = DUMP_LITTLE_ENDIAN != cpu_little_endian();
}

dumper::dumper(const char *fname, bool write)
{
  _write = write;
  _f = fopen(fname, _write ? "wb" : "rb");

  if(_f == NULL)
    throw error("unable to open dump file");

  _buff = (char *) malloc(BUFF_SIZE);
  setvbuf(_f, _buff, _IOFBF, BUFF_SIZE);

  _coe = true;
  _swap = DUMP_LITTLE_ENDIAN != cpu_little_endian();
}

dumper::~dumper()
{
  if(_coe)
    {
      fclose(_f);
      if(_buff) free(_buff);
    }
}

void dumper::dump_char(char i)
{
  if(!_write || fwrite(&i, sizeof(i), 1, _f) != 1)
    throw error("couldn't write character to file");
}

char dumper::undump_char()
{
  char i;
  if(_write || fread(&i, sizeof(i), 1, _f) != 1)
    throw error("couldn't read character from file");
  return i;
}

void dumper::dump_short(short i)
{
  if(_swap) i = swap_short(i);
  if(!_write || fwrite(&i, sizeof(i), 1, _f) != 1)
    throw error("couldn't write short to file");
}

short dumper::undump_short()
{
  short i;
  if(_write || fread(&i, sizeof(i), 1, _f) != 1)
    throw error("couldn't read short from file");
  if(_swap)
    return swap_short(i);
  else
    return i;
}

void dumper::dump_int(int i)
{
  if(_swap) i = swap_int(i);
  if(!_write || fwrite(&i, sizeof(i), 1, _f) != 1)
    throw error("couldn't write integer to file");
}

int dumper::undump_int()
{
  int i;
  if(_write || fread(&i, sizeof(i), 1, _f) != 1)
    throw error("couldn't read integer from file");
  if(_swap)
    return swap_int(i);
  else
    return i;
}

void dumper::dump_string(const char *s)
{
  int len;
  len = strlen(s) + 1;

  if(!_write)
    throw error("not in write mode");

  dump_short(len);

  if(fwrite(s, sizeof(char), len, _f) != (unsigned int) len)
    throw error("error writing string to file");
}

char * dumper::undump_string()
{
  int len;
  char *s;

  if(_write)
    throw error("not in read mode");

  len = undump_short();

  s = (char *) malloc(sizeof(char) * len);
  
  if(s == 0 || fread(s, sizeof(char), len, _f) != (unsigned int) len)
    throw error("error reading string from file");

  return s;
}

int dumper::dump_int_variable()
{
  int pos = tell();
  dump_int(-1);
  return pos;
}

void dumper::set_int_variable(int pos, int val)
{
  int curr = tell();
  seek(pos);
  dump_int(val);
  seek(curr);
}


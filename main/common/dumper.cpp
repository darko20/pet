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

/* object representing a binary dump 
 * abstracts away from low level representation (byteorder etc)
 */

#include "pet-system.h"
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
    throw tError("unable to open file `" + string(fname) + "'");

  _buff = new char[BUFF_SIZE];
  setvbuf(_f, _buff, _IOFBF, BUFF_SIZE);

  _coe = true;
  _swap = DUMP_LITTLE_ENDIAN != cpu_little_endian();
}

dumper::~dumper()
{
  if(_coe)
    {
      fclose(_f);
      delete[] _buff;
    }
}

void dumper::dump_char(char i)
{
  if(!_write || fwrite(&i, sizeof(i), 1, _f) != 1)
    throw tError("couldn't write character to file");
}

char dumper::undump_char()
{
  char i;
  if(_write || fread(&i, sizeof(i), 1, _f) != 1)
    throw tError("couldn't read character from file");
  return i;
}

void dumper::dump_short(short i)
{
  if(_swap) i = swap_short(i);
  if(!_write || fwrite(&i, sizeof(i), 1, _f) != 1)
    throw tError("couldn't write short to file");
}

short dumper::undump_short()
{
  short i;
  if(_write || fread(&i, sizeof(i), 1, _f) != 1)
    throw tError("couldn't read short from file");
  if(_swap)
    return swap_short(i);
  else
    return i;
}

void dumper::dump_int(int i)
{
  if(_swap) i = swap_int(i);
  if(!_write || fwrite(&i, sizeof(i), 1, _f) != 1)
    throw tError("couldn't write integer to file");
}

int dumper::undump_int()
{
  int i;
  if(_write || fread(&i, sizeof(i), 1, _f) != 1)
    throw tError("couldn't read integer from file");
  if(_swap)
    return swap_int(i);
  else
    return i;
}

void dumper::dump_string(const char *s)
{
  if(!_write)
    throw tError("not in write mode");

  if(s == 0)
    {
      dump_short(0);
      return;
    }

  int len;
  len = strlen(s) + 1;

  dump_short(len);

  if(fwrite(s, sizeof(char), len, _f) != (unsigned int) len)
    throw tError("error writing string to file");
}

char *dumper::undump_string()
{
  int len;
  char *s;

  if(_write)
    throw tError("not in read mode");

  len = undump_short();

  if(len == 0)
    return 0;

  s = new char[len];
  
  if(s == 0 || fread(s, sizeof(char), len, _f) != (unsigned int) len)
    throw tError("error reading string from file");

  return s;
}

int dumper::dump_int_variable()
{
  long int pos = tell();
  dump_int(-1);
  return pos;
}

void dumper::set_int_variable(long int pos, int val)
{
  long int curr = tell();
  seek(pos);
  dump_int(val);
  seek(curr);
}


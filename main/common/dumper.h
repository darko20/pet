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

#ifndef _DUMPER_H_
#define _DUMPER_H_

#include "errors.h"

class dumper
{
 public:
  dumper(FILE *f, bool write = false);
  dumper(const char *fname, bool write = false);

  ~dumper();

  void dump_char(char i);
  char undump_char();

  void dump_short(short i);
  short undump_short();

  void dump_int(int i);
  int undump_int();

  void dump_string(const char *s);
  char *undump_string();

  int dump_int_variable();
  void set_int_variable(long int pos, int val);

  inline long int tell()
    { return ftell(_f); }
  inline void seek(long int pos)
    { if(fseek(_f, pos, SEEK_SET) != 0) throw tError("cannot seek"); }

 private:
  FILE *_f;
  char *_buff;
  bool _write; // writeable?
  bool _coe;   // close on exit
  bool _swap;  // swap byteorder?
};

#endif

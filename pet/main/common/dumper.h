/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* object representing a binary dump 
 * abstracts away from low level representation (byteorder etc)
 */

#ifndef _DUMPER_H_
#define _DUMPER_H_

#include <stdio.h>
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
  void set_int_variable(int pos, int val);

  inline long int tell()
    { return ftell(_f); }
  inline void seek(long int pos)
    { if(fseek(_f, pos, SEEK_SET) != 0) throw error("cannot seek"); }

 private:
  FILE *_f;
  char *_buff;
  bool _write; // writeable?
  bool _coe;   // close on exit
  bool _swap;  // swap byteorder?
};

#endif

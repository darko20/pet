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

/** \file dumper.h
 * Object representing a binary dump.
 * abstracts away from low level representation (byteorder etc)
 */

#ifndef _DUMPER_H_
#define _DUMPER_H_

#include "errors.h"
#include <cstdio>

/** This class implements binary serialization of some basic data structures.
 * It dumps multi-byte data such as shorts or ints always in little endian
 * mode. On high-endian architectures, data is appropriately swapped before
 * writing and after reading.
 */
class dumper
{
 public:
  /** Create binary serializer reading or writing data to/from \a f
   * \pre \a f has to be an open stream with binary read or write access.
   */
  dumper(FILE *f, bool write = false);
  /** Create binary serializer reading or writing data to/from file with name
   *  \a fname.
   * \throws tError if the file can not be opened appropriately.
   */
  dumper(const char *fname, bool write = false);

  /** Destructor: close file if opened by dumper, destroy buffers */
  ~dumper();

  /*@{*/
  /** Dump/Undump a \c char (one byte) */
  void dump_char(char i);
  char undump_char();
  /*@}*/

  /*@{*/
  /** Dump/Undump a \c short (two bytes) */
  void dump_short(short i);
  short undump_short();
  /*@}*/

  /*@{*/
  /** Dump/Undump an \c int (four bytes) */
  void dump_int(int i);
  int undump_int();
  /*@}*/

  /*@{*/
  /** Dump/Undump a \c char* string: two bytes length, then the char array. */
  void dump_string(const char *s);
  char *undump_string();
  /*@}*/

  /** Reserve space for an \c int in the file and return the position where the
   *  \c int may be stored later on
   */
  int dump_int_variable();
  /** Store the \c int (four byte value) \a val into the previously reserved
   *  position \a pos.
   * \see dump_int_variable
   */
  void set_int_variable(long int pos, int val);

  /** Return the position of the file pointer */
  inline long int tell()
    { return ftell(_f); }
  /** Set the file pointer to position \a pos */
  inline void seek(long int pos)
    { if(fseek(_f, pos, SEEK_SET) != 0) throw tError("cannot seek"); }

 private:
  FILE *_f;
  char *_buff;
  /** writeable? */
  bool _write;
  /** close on exit */
  bool _coe;
  /** swap byteorder? */
  bool _swap;
};

#endif

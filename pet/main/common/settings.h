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

/* a settings file */

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "lex-tdl.h"
#include "types.h"

#define SET_SUBDIRECTORY "pet"
#define SET_EXT ".set"
#define SET_TABLE_SIZE 100

struct setting
{
  char *name;
  int n, allocated;
  char **values;

  struct lex_location* loc;
};

class settings
{
 public:
  settings(const char *name, const char *base, char *message = 0);
  ~settings();
  
  setting *lookup(const char *name);

  char *value(const char *name);
  char *req_value(const char *name);

  // string equality based member test
  bool member(const char *name, const char *value);

  // type status based member test
  bool statusmember(const char *name, type_t key);

  // string equality based assoc
  char *assoc(const char *name, const char *key, int arity = 2, int nth = 1);

#ifndef FLOP
  // subtype based map
  set<string> smap(const char *name, int key_type);
#endif

  struct lex_location *lloc() { return _lloc; }

  static char *basename(const char *name);
  
 private:
  int _n;
  setting **_set;
  char *_fname, *_prefix;
  struct lex_location *_lloc;

  map<string,list_int *> _li_cache; // cache for settings converted to lists
                                    // of integers (e.g. status values)
  void parse();
  void parse_one();
};

#endif

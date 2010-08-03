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

/** \file settings.h
 * Information contained in a settings file.
 */

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "lex-tdl.h"
#include "types.h"
#include <set>
#include <map>

#define SET_EXT ".set"
#define SET_TABLE_SIZE 1024

/** A setting from the settings file (in TDL format) */
struct setting
{
  char *name;
  int n, allocated;
  char **values;

  struct lex_location* loc;
};

/** a set of settings with access functions */
class settings
{
 public:
  settings(std::string name, std::string base, const char *message = 0);
  settings(const std::string &input);
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
  std::set<std::string> smap(const char *name, int key_type);
#endif

  struct lex_location *lloc() { return _lloc; }

 private:
  int _n;
  setting **_set;
  std::string _fname, _prefix;
  struct lex_location *_lloc;

  /** cache for settings converted to lists of integers (e.g. status values) */
  std::map<std::string, struct list_int *> _li_cache;

  void parse();
  void parse_one();
};

#endif

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

//
// This modules implements the link to the Inquizit dictionary, including
// the mapping to the grammar lexicon.
//

#ifndef _IQT_H_
#define _IQT_H_

#ifndef IQTEMU
#include "iqtDct3.h"
#endif

//
// Mapping to the grammar lexicon.
//

class iqtMapEntry
{
 public:
  iqtMapEntry()
    : _valid(false)
  {}

  iqtMapEntry(type_t type, list<pair<string, string> > paths)
    : _valid(true), _type(type), _paths(paths)
  {}

  iqtMapEntry(const iqtMapEntry &other)
    : _valid(other._valid), _type(other._type), _paths(other._paths)
  {}

  iqtMapEntry& operator=(const iqtMapEntry& other)
  {
    iqtMapEntry temp(other);
    Swap(temp);
    return *this;
  }

  void Swap(iqtMapEntry &other) throw()
  {
    swap(_valid, other._valid);
    swap(_type, other._type);
    swap(_paths, other._paths);
  }

  bool valid() const { return _valid; }
  type_t type() const { return _type; }
  const list<pair<string, string> > &paths() const { return _paths; }

  void print(FILE *f) const;

 private:
  bool _valid;
  type_t _type;
  list<pair<string, string> > _paths;

  friend bool operator==(const iqtMapEntry &a, const iqtMapEntry &b);
  friend bool operator<(const iqtMapEntry &a, const iqtMapEntry &b);
};

class iqtMapping
{
 public:
  iqtMapping(const string &mappath);

  ~iqtMapping()
  {}

  iqtMapEntry get(const string &tag)
  {
    return _map[tag];
  }

  // Return representative of equivalence class that t belongs to
  type_t equiv_rep(type_t t);
  
  // Return rank of t within its equivalence class
  int equiv_rank(type_t t);

  // Is the .tag. specified as undesired for .word. ?
  bool undesired(const string &word, const string &tag);
  
 private:
  map<string, iqtMapEntry> _map;

  map<type_t, type_t> _equivs;
  map<type_t, int> _rank;

  multimap<string, string> _undesired;

  void add_equiv(const list<string> &elems);
  void add_undesired(list<string> elems);
};

//
// The class iqtDictionary encapsulates access to the Inquizit dictionary
// and the associated mapping.
//

class iqtDictionary
{
 public:
  iqtDictionary(const string &dictpath, const string &mappath);
  ~iqtDictionary();

  bool getHeadlist(const string &word, string &headlist);
  bool getHeadlist(const string &word, list<list<string> > &headlist);
  bool getMapped(const string &word, list<iqtMapEntry> &mapped);

  // Return representative of equivalence class that t belongs to
  type_t equiv_rep(type_t t);
  
  // Return rank of t within its equivalence class
  int equiv_rank(type_t t);

#ifdef IQTEMU
  void lookupAll();
#endif

 private:
#ifndef IQTEMU
  PDCTSTATE _pDict;
#else
  map<string, string> _dict;
#endif
  class iqtMapping *_pMap;

#ifdef IQTEMU
  void readEmu(const string &);
#endif
};

#endif

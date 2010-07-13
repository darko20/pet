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
// This modules implements the link to an external dictionary, including
// the mapping to the grammar lexicon.
//

#ifndef _EXTDICT_H_
#define _EXTDICT_H_

//
// Mapping to the grammar lexicon.
//

class extDictMapEntry
{
 public:
  extDictMapEntry()
    : _valid(false)
  {}

  extDictMapEntry(type_t type, list<pair<string, string> > paths)
    : _valid(true), _type(type), _paths(paths)
  {}

  extDictMapEntry(const extDictMapEntry &other)
    : _valid(other._valid), _type(other._type), _paths(other._paths)
  {}

  extDictMapEntry& operator=(const extDictMapEntry& other)
  {
    extDictMapEntry temp(other);
    Swap(temp);
    return *this;
  }

  void Swap(extDictMapEntry &other) throw()
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

  friend bool operator==(const extDictMapEntry &a, const extDictMapEntry &b);
  friend bool operator<(const extDictMapEntry &a, const extDictMapEntry &b);
};

class extDictMapping
{
 public:
  extDictMapping(const string &mappath);

  ~extDictMapping()
  {}

  extDictMapEntry get(const string &tag)
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
  map<string, extDictMapEntry> _map;

  map<type_t, type_t> _equivs;
  map<type_t, int> _rank;

  multimap<string, string> _undesired;

  void add_equiv(const list<string> &elems);
  void add_undesired(list<string> elems);
};

//
// The class extDictionary encapsulates access to an external dictionary
// and the associated mapping.
//

class extDictionary
{
 public:
  extDictionary(const string &dictpath, const string &mappath);
  ~extDictionary();

  bool getHeadlist(const string &word, string &headlist);
  bool getHeadlist(const string &word, list<list<string> > &headlist);
  bool getMapped(const string &word, list<extDictMapEntry> &mapped);

  // Return representative of equivalence class that t belongs to
  type_t equiv_rep(type_t t);
  
  // Return rank of t within its equivalence class
  int equiv_rank(type_t t);

  void lookupAll();

 private:
  map<string, string> _dict;
  class extDictMapping *_pMap;

  void readEmu(const string &);
};

#endif

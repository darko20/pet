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

#include "config.h"

#ifdef HAVE_EXTDICT

#include "pet-system.h"
#include "errors.h"
#include "utility.h"
#include "cheap.h"
#include "grammar.h"
#include "parse.h"
#include "extdict.h"

//
// Utility functions
//

// Parse a string of elements seperated by |.
void extDictParseList(string s, list<string> &L)
{
  L.clear();
  
  string elem;
  for(string::iterator curr = s.begin(); curr != s.end(); ++curr)
  {
    if(*curr == '|')
    {
      L.push_back(elem);
      elem.erase();
    }
    else
    {
      elem += *curr;
    }
  }

  if(!elem.empty())
    L.push_back(elem);
}

// Parse a string of elements seperated by whitespace, ignoring comments.
void extDictParseLine(string s, list<string> &L)
{
  L.clear();
  
  string elem;
  for(string::iterator curr = s.begin(); curr != s.end(); ++curr)
  {
    if(isspace(*curr))
    {
      if(!elem.empty())
      {
        L.push_back(elem);
        elem.erase();
      }
    }
    else if(*curr == ';')
    {
      if(!elem.empty())
      {
        L.push_back(elem);
        elem.erase();
      }
      break;
    }
    else
    {
      elem += *curr;
    }
  }

  if(!elem.empty())
    L.push_back(elem);
}

//
// Dictionary access class
//

extDictionary::extDictionary(const string &dictpath, const string &mappath)
  : _pMap(0)
{
  readEmu(dictpath + string("/extdict.lex"));

  _pMap = new extDictMapping(mappath);
}

void
extDictionary:: readEmu(const string &dictpath)
{
  push_file(dictpath.c_str(), "loading");
  
  while(LLA(0) != EOF)
  {
    char *s = LMark();
    int i = 0;
    while(LLA(i) != ' ' && LLA(i) != EOF) i++;
    string lhs(s, i);
    LConsume(i);
    
    if(LLA(0) != ' ')
    {
      fprintf(ferr, "\nMissing space at %s:%d.%d", curr_fname(),
              curr_line(), curr_col());
      lhs = "";
    }
    else
      LConsume(1);
    
    s = LMark();
    i = 0;
    while(LLA(i) != '\n' && LLA(i) != EOF) i++;
    string rhs(s, i);
    LConsume(i);

    if(LLA(0) != '\n')
    {
      fprintf(ferr, "\nMissing newline at %s:%d.%d", curr_fname(),
              curr_line(), curr_col());
      rhs = "";
    }
    else
      LConsume(1);
    
    if(!lhs.empty() && !rhs.empty())
    {
      _dict[lhs] = rhs;
    }
    
  }
}
      
extDictionary::~extDictionary()
{
  delete _pMap;
}

bool extDictionary::getHeadlist(const string &word, string &headlist)
{
  headlist.erase();
  
  map<string, string>::iterator it = _dict.find(word);
  if(it != _dict.end())
  {
    headlist = it->second;
    return true;
  }

  return false;
}

bool extDictionary::getHeadlist(const string &word,
                                list<list<string> > &headlist)
{
  headlist.clear();

  string s;
  if(!getHeadlist(word, s))
    return false;

  list<string> L;
  extDictParseList(s, L);
  
  list<string> curr;
  for(list<string>::iterator it = L.begin(); it != L.end(); ++it)
  {
    if(*it == string("-----"))
    {
      if(!curr.empty())
        headlist.push_back(curr);
      curr.clear();
    }
    else
    {
      if(!_pMap->undesired(word, *it))
        curr.push_back(*it);
    }
  }
  if(!curr.empty())
    headlist.push_back(curr);

  return true;
}

bool extDictionary::getMapped(const string &word, list<extDictMapEntry> &resmapped)
{
  resmapped.clear();

  // Don't map one or two letter words.
  if(word.length() <= 2)
    return true;

  // Get EXTDICT headlist.
  list<list<string> > headlist;
  getHeadlist(word, headlist);
  
  list<extDictMapEntry> allmapped;
  for(list<list<string> >::iterator codeit = headlist.begin();
      codeit != headlist.end(); ++codeit)
  {
    list<string> code = *codeit;
    string headelem = code.front();
    code.pop_front();

    // Ignore code if we cannot map the first element.
    if(!_pMap->get(headelem).valid())
      continue;

    // Map at most ten elements.
    list<extDictMapEntry> mapped;
    for(list<string>::iterator elem = code.begin();
        elem != code.end() && mapped.size() < 10; ++elem)
    {
      extDictMapEntry e = _pMap->get(*elem);
      if(e.valid())
        mapped.push_back(e);
    }

    // If nothing was found, return mapping of first element.
    if(mapped.empty())
      mapped.push_back(_pMap->get(headelem));
    
    // Eliminate duplicates.
    mapped.sort();
    mapped.unique();
    
    // Append to result list.
    allmapped.splice(allmapped.end(), mapped);
  }

  // Now filter according to the equivalence classes and the subsumption
  // relation they impose:
  // For each entry in allmapped, see if there's another entry in allmapped
  // that subsumes it. If not, add it to the output list resmapped.

  for(list<extDictMapEntry>::iterator it1 = allmapped.begin();
      it1 != allmapped.end(); ++it1)
  {
    bool subsumed = false;
    for(list<extDictMapEntry>::iterator it2 = allmapped.begin();
        !subsumed && it2 != allmapped.end(); ++it2)
    {
      if(it2 == it1) continue;

      if(equiv_rep(it2->type()) == equiv_rep(it1->type())
         && equiv_rank(it2->type()) < equiv_rank(it1->type()))
        subsumed = true;
    }
    if(!subsumed)
      resmapped.push_back(*it1);
  }

  return true;
}

type_t extDictionary::equiv_rep(type_t t)
{
  return _pMap->equiv_rep(t);
}

int extDictionary::equiv_rank(type_t t)
{
  return _pMap->equiv_rank(t);
}

//
// Mapping.
// 

void extDictMapEntry::print(FILE *f) const
{
  fprintf(f, " %s [", type_name(_type));
  for(list<pair<string, string> >::const_iterator it = _paths.begin();
      it != _paths.end(); ++it)
  {
    fprintf(f, " %s: %s", it->first.c_str(), it->second.c_str());
  }
  fprintf(f, " ]");
}

bool operator==(const extDictMapEntry &a, const extDictMapEntry &b)
{
  return a._type == b._type;
}

bool operator<(const extDictMapEntry &a, const extDictMapEntry &b)
{
  return a._type < b._type;
}

extDictMapping::extDictMapping(const string& mappath)
{
  FILE *f = fopen(mappath.c_str(), "r");
  if(!f)
    throw error(string("Could not open map file \"") + mappath + string("\""));
  
  string line;
  while(!feof(f))
  {
    line = read_line(f);
    if(line.empty())
      continue;
    
    list<string> elems;

    extDictParseLine(line, elems);
    if(elems.empty())
      continue;
    
    if(elems.size() == 1)
    {
      fprintf(ferr, "\nIgnoring entry `%s' in mapping (entry incomplete).",
              line.c_str());
      continue;
    }

    string lhs = elems.front();
    elems.pop_front();

    if(lhs == "%equivalent")
    {
      add_equiv(elems);
      continue;
    }

    if(lhs == "%undesired")
    {
      if(elems.size() < 2)
      {
        fprintf(ferr, "\nIgnoring incomplete entry `%s'.",
                line.c_str());
        continue;
      }
      
      add_undesired(elems);
      continue;
    }
    
    string rhs = elems.front();
    elems.pop_front();
    
    type_t type = lookup_type(rhs.c_str());
    
    if(type == -1)
    {
      if(rhs != "unknown_le")
        fprintf(ferr, "\nIgnoring entry `%s' in mapping (unknown type `%s').",
                line.c_str(), rhs.c_str());
      continue;
    }

    // the reminder of the entry should be of the form
    // ( ':' path value)*
    int state = 1;
    string path;
    string value;
    list<pair<string, string> > paths;
    for(list<string>::iterator it = elems.begin(); it != elems.end(); ++it)
    {
      if(state == 1)
      {
        if(*it != string(":"))
        {
          fprintf(ferr, "\nIgnoring entry `%s' in mapping (not wellformed).",
                  line.c_str());
          state = -1;
          break;
        }
        state = 2;
      }
      else if(state == 2)
      {
        path = string("SYNSEM.LOCAL.KEYS.") + *it;
        state = 3;
      }
      else if(state == 3)
      {
        value = *it;
        if(lookup_type(value.c_str()) == -1)
        {
          fprintf(ferr, "\nIgnoring entry `%s' in mapping (unknown type `%s').",
                  line.c_str(), value.c_str());
          state = -1;
          break;
        }
        paths.push_back(make_pair(path, value));
        state = 1;
      }
    }

    if(state != 1) // skip entry
    {
      fprintf(ferr, "\nIgnoring entry `%s' in mapping.",
              line.c_str());
      continue;
    }

    extDictMapEntry entry(type, paths);
    _map[lhs] = entry;

    if(verbosity > 9)
    {
      entry.print(fstatus);
      fprintf(fstatus, "\n");
    }
  }
}

void
extDictMapping::add_equiv(const list<string> &elems)
{
  list<type_t> ts;
  int rank = 0;
  
  for(list<string>::const_iterator it = elems.begin(); it != elems.end(); ++it)
  {
    type_t t = lookup_type(it->c_str());
    if(t != -1)
    {
      ts.push_back(t);
      _rank[t] = ++rank;
    }
    else
      fprintf(ferr, "\nIgnoring unknown type `%s' in equivalence class.", 
              it->c_str());
  }

  for(list<type_t>::iterator it = ts.begin(); it != ts.end(); ++it)
  {
    _equivs[*it] = ts.front();
  }
}

void
extDictMapping::add_undesired(list<string> elems)
{
  string word = elems.front();
  elems.pop_front();

  for(list<string>::const_iterator it = elems.begin(); it != elems.end(); ++it)
  {
    _undesired.insert(make_pair(word, *it));
  }
}

type_t
extDictMapping::equiv_rep(type_t t)
{
  map<type_t, type_t>::iterator it = _equivs.find(t);
  if(it != _equivs.end())
    return it->second;
  else
    return t;
}

int
extDictMapping::equiv_rank(type_t t)
{
  map<type_t, type_t>::iterator it = _equivs.find(t);
  if(it != _equivs.end())
    return it->second;
  else
    return 0;
}

bool
extDictMapping::undesired(const string &word, const string &tag)
{
  pair<multimap<string, string>::iterator,
    multimap<string, string>::iterator> eq =
    _undesired.equal_range(word);
  
  for(multimap<string, string>::iterator it = eq.first;
      it != eq.second; ++it)
  {
    if(it->second == tag)
      return true;
  }

  return false;
}

//
// 

void
extDictionary::lookupAll()
{
  for(map<string, string>::iterator it = _dict.begin(); it != _dict.end(); ++it)
  {
    list<lex_stem *> les = 
      Grammar->lookup_stem(it->first);
    Grammar->clear_dynamic_stems();
  }
}

#endif

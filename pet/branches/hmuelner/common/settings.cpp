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

/* parse the settings file */

#include "lex-tdl.h"
#include "settings.h"
#include "utility.h"
#include "types.h"
#include "errors.h"
#include "list-int.h"

#include "logging.h"

#include <cstdlib>
#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>

using std::string;
using boost::algorithm::iequals;

settings::settings(string name, string base_dir, const char *message)
: _li_cache(), _lloc() {

  _fname = find_set_file(name, SET_EXT, base_dir);

  _prefix = dir_name(_fname);

  if(!_fname.empty()) {
    push_file(_fname, message);
    const char *sv = lexer_idchars;
    lexer_idchars = "_+-*?$";
    parse();
    lexer_idchars = sv;

  }
}


settings::settings(const std::string &input)
: _li_cache(), _lloc() {

  push_string(input, NULL);
  const char *sv = lexer_idchars;
  lexer_idchars = "_+-*?$";
  parse();
  lexer_idchars = sv;
}


settings::~settings()
{
  for(int i = 0; i < _set.size(); i++) {
    delete _set[i];
  }
}

/** Do a linear search for \a name in the settings and return the first
* matching setting.
*/
setting *settings::lookup(const char *name) {
  for(int i = 0; i < _set.size(); i++)
    if(_set[i]->_name == name) {
      if(i != 0) {
        // put to front, so further lookup is faster
        setting *tmp = _set[i]; _set[i] = _set[0]; _set[0] = tmp;
      }

      _lloc = _set[0]->_loc;
      return _set[0];
    }

    return 0;
}

/** Return the value of the first setting matching \a name or NULL if there is
* no such setting.
*/
const char *settings::value(const char *name)
{
  setting *s = lookup(name);
  if(s == 0) return 0;

  return s->values[0].c_str();
}

/** Return the value of the first setting matching \a name or throw an error if
*  there is no such setting (short for required_value).
*/
const char *settings::req_value(const char *name)
{
  const char *v = value(name);
  if(v == 0)
  {
    throw tError("no definition for required parameter `" + string(name) + "'");
  }
  return v;
}

/** \brief Is \a value in the list for \a name? If so, return \c true */
bool settings::member(const char *name, const char *value)
{
  setting *set = lookup(name);

  if(set == 0) return false;

  for(int i = 0; i < set->n(); i++)
    if(iequals(set->values[i], value))
      return true;

  return false;
}

/** Get an element of a setting that is an assoc list.
*
*  \param name The setting to look for
*  \param key The key to search in the assoc list
*  \param arity The number of elements in one assoc-cons
*  \param nth The number of the element to return
*  \return NULL if there is no such setting or no such key in the assoc list,
*          the \a nth element of the assoc cons otherwise
*/
const char *settings::assoc(const char *name, const char *key, int arity, int nth)
{
  setting *set = lookup(name);
  if(set == 0) return 0;

  assert(nth <= arity && arity > 1);

  for(int i = 0; i < set->n(); i+=arity)
  {
    if(i+nth >= set->n()) return 0;
    if(iequals(set->values[i], key))
      return set->values[i+nth].c_str();
  }

  return 0;
}

#ifndef FLOP
// subtype based map
std::set<std::string> settings::smap(const char *name, int key_type)
{
  std::set<std::string> res;

  setting *set = lookup(name);
  if(set == 0) return res;

  if(key_type == -1) return res;

  for(int i = 0; i < set->n(); i+=2)
  {
    if(i+2 > set->n())
    {
      LOG(logAppl, WARN, "warning: incomplete last entry in `" << name
        << "' mapping - ignored");
      break;
    }

    const char* lhs = set->values[i].c_str();
    const char* rhs = set->values[i+1].c_str();
    int id = lookup_type(lhs);
    if(id != -1)
    {
      if(subtype(key_type, id))
        res.insert(rhs);
    }
    else
      LOG(logAppl, WARN, "unknown type `" << name << "' in `"
      << lhs << "' mapping - ignored");

  }

  return res;
}
#endif

/** Return \c true, if the \a key is a type with status \a name.
*/
bool settings::statusmember(const string& name, type_t key)
{
  // first try to find the list of status types for name in the cache
  CacheType::const_iterator l = _li_cache.find(name);
  if(l == _li_cache.end())
  {
    setting *set = lookup(name.c_str());
    // convert the set of status names into a list of code numbers and store
    // it in the cache. All status names that do not occur in the grammar
    // are reported to be unknown.
    bool contained = false;
    if (set != 0) {
      std::vector<int> codeNumbers;
      for(int i = 0; i < set->n(); ++i) {
        int v = lookup_status(set->values[i].c_str());
        if (v == -1)
        {
          LOG(logAppl, WARN, "ignoring unknown status `"
            << set->values[i] << "' in setting `" << name << "'");
        }
        else {
          codeNumbers.push_back(v);
          contained = contained || (v == key);
        }
      }
      _li_cache[name] = codeNumbers;
    }
    return contained;
  }
  return std::find(l->second.begin(), l->second.end(), key) != l->second.end();
}

void settings::parse_one()
{
  string option = LA(0)->text;
  consume(1);

  setting *set = lookup(option.c_str());
  if(set)
  {
    LOG(logAppl, WARN,
      "more than one definition for setting `" << option << "'...");
  }
  else
  {
    _set.push_back(new setting());
    set = _set.back();
    set->_name = option;
    set->_loc = LA(0)->loc;
  }

  if(LA(0)->tag != T_DOT)
  {
    match(T_ISEQ, "option setting", true);

    while(LA(0)->tag != T_DOT && LA(0)->tag != T_EOF)
    {
      if(LA(0)->tag == T_ID || LA(0)->tag == T_KEYWORD ||
        LA(0)->tag == T_STRING)
      {
        set->values.push_back(LA(0)->text);
      }
      else
      {
        LOG(logSyntax, WARN, LA(0)->loc.fname << ":"
          << LA(0)->loc.linenr << ": warning: ignoring "
          << LA(0)->text);
      }

      consume(1);
    }
  }

  match(T_DOT, "end of option setting", true);
}

void settings::parse() {
  do {
    if(LA(0)->tag == T_ID) {
      parse_one();
    }
    else if(LA(0)->tag == T_COLON) {
      consume(1);
    }
    else if(LA(0)->tag == T_KEYWORD && LA(0)->text == "include") {
      consume(1);

      if(LA(0)->tag != T_STRING) {
        LOG(logSyntax, WARN, LA(0)->loc.fname << ":" << LA(0)->loc.linenr
          << ": warning: expecting include file name");
      }
      else {
        string ofname = _prefix + LA(0)->text + SET_EXT;
        consume(1);

        match(T_DOT, "`.'", true);

        if(file_exists_p(ofname.c_str())) {
          push_file(ofname, "including");
        } else {
          LOG(logAppl, WARN, "file `" << ofname << "' not found. skipping...");
        }
      }
    }
    else {
      syntax_error("expecting identifier", LA(0));
      consume(1);
    }
  } while(LA(0)->tag != T_EOF);

  consume(1);
}

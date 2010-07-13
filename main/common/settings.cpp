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

using std::string;

settings::settings(string name, string base_dir, const char *message)
  : _li_cache() {
  _n = 0;
  _set = new setting*[SET_TABLE_SIZE];

  _fname = find_set_file(name, SET_EXT, base_dir);

  _prefix = dir_name(_fname);

  if(! _fname.empty()) {
    push_file(_fname, message);
    const char *sv = lexer_idchars;
    lexer_idchars = "_+-*?$";
    parse();
    lexer_idchars = sv;

  }
  _lloc = 0;
}


settings::settings(const std::string &input)
  : _li_cache() {
  _n = 0;
  _set = new setting*[SET_TABLE_SIZE];

  push_string(input, NULL);
  const char *sv = lexer_idchars;
  lexer_idchars = "_+-*?$";
  parse();
  lexer_idchars = sv;
  _lloc = 0;
}


settings::~settings() {
  for(int i = 0; i < _n; i++) {
    for(int j = 0; j < _set[i]->n; j++)
      free(_set[i]->values[j]);
    free(_set[i]->values);
    delete _set[i];
  }

  delete[] _set;

  for(std::map<std::string, struct list_int *>::iterator it = _li_cache.begin();
      it != _li_cache.end(); ++it) {
    free_list(it->second);
  }
}

/** Do a linear search for \a name in the settings and return the first
 * matching setting.
 */
setting *settings::lookup(const char *name) {
  for(int i = 0; i < _n; i++)
    if(strcmp(_set[i]->name, name) == 0) {
      if(i != 0) {
        // put to front, so further lookup is faster
        setting *tmp;
        tmp = _set[i]; _set[i] = _set[0]; _set[0] = tmp;
      }

      _lloc = _set[0]->loc;
      return _set[0];
    }

  _lloc = 0;
  return 0;
}

/** Return the value of the first setting matching \a name or NULL if there is
 * no such setting.
 */
char *settings::value(const char *name) {
  setting *s;

  s = lookup(name);
  if(s == 0) return 0;

  return s->values[0];
}

/** Return the value of the first setting matching \a name or throw an error if
 *  there is no such setting (short for required_value).
 */
char *settings::req_value(const char *name)
{
  char *v = value(name);
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

  for(int i = 0; i < set->n; i++)
    if(strcasecmp(set->values[i], value) == 0)
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
char *settings::assoc(const char *name, const char *key, int arity, int nth)
{
  setting *set = lookup(name);
  if(set == 0) return 0;

  assert(nth <= arity && arity > 1);

  for(int i = 0; i < set->n; i+=arity)
    {
      if(i+nth >= set->n) return 0;
      if(strcasecmp(set->values[i], key) == 0)
        return set->values[i+nth];
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

  for(int i = 0; i < set->n; i+=2)
    {
      if(i+2 > set->n)
        {
          LOG(logAppl, WARN, "warning: incomplete last entry in `" << name
              << "' mapping - ignored");
          break;
        }

      char *lhs = set->values[i], *rhs = set->values[i+1];
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
bool settings::statusmember(const char *name, type_t key)
{
  // first try to find the list of status types for name in the cache
  list_int *l = _li_cache[string(name)];
  if(l == 0)
    {
      setting *set = lookup(name);
      // convert the set of status names into a list of code numbers and store
      // it in the cache. All status names that do not occur in the grammar
      // are reported to be unknown.
      if(set != 0)
        {
          for(int i = 0; i < set->n; i++)
            {
              int v = lookup_status(set->values[i]);
              if(v == -1)
                {
                  LOG(logAppl, WARN, "ignoring unknown status `"
                      << set->values[i] << "' in setting `" << name << "'");
                }
              else
                l = cons(v, l);
            }
          _li_cache[string(name)] = l;
        }
    }
  return contains(l, key);
}

void settings::parse_one()
{
  char *option;
  struct setting *set;
  option = LA(0)->text; LA(0)->text = NULL;
  consume(1);

  set = lookup(option);
  if(set)
    {
      LOG(logAppl, WARN,
          "more than one definition for setting `" << option << "'...");
    }
  else
    {
      assert(_n < SET_TABLE_SIZE);
      set = _set[_n++] = new setting;
      set->name = option;
      set->loc = LA(0)->loc; LA(0)->loc = NULL;
      set->n = 0;
      set->allocated = SET_TABLE_SIZE;
      set->values = (char **) malloc(set->allocated * sizeof(char *));
    }

  if(LA(0)->tag != T_DOT)
    {
      match(T_ISEQ, "option setting", true);

      while(LA(0)->tag != T_DOT && LA(0)->tag != T_EOF)
        {
          if(LA(0)->tag == T_ID || LA(0)->tag == T_KEYWORD ||
             LA(0)->tag == T_STRING)
            {
              if(set->n >= set->allocated)
                {
                  set->allocated += SET_TABLE_SIZE;
                  set->values = (char **) realloc(set->values, set->allocated * sizeof(char *));

                }
              set->values[set->n++] = LA(0)->text; LA(0)->text=NULL;
            }
          else
            {
              LOG(logSyntax, WARN, LA(0)->loc->fname << ":"
                  << LA(0)->loc->linenr << ": warning: ignoring "
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
    else if(LA(0)->tag == T_KEYWORD && strcmp(LA(0)->text, "include") == 0) {
      consume(1);

      if(LA(0)->tag != T_STRING) {
        LOG(logSyntax, WARN, LA(0)->loc->fname << ":" << LA(0)->loc->linenr
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

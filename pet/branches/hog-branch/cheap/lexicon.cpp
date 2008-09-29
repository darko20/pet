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

/* Lexicon classes */

#include "lexicon.h"
#include "cheap.h"
#include "utility.h"
#include "grammar.h"
#include "morph.h"

using std::string;
using std::list;
using std::vector;

int lex_stem::next_id = 0;

fs
lex_stem::instantiate()
{
    fs e(_lexical_type);

    fs expanded = unify(e, (fs(_instance_type)), e);

    if(!expanded.valid())
    {
        string msg = string("invalid lex_stem `") + printname()
                     + "' (cannot expand)";

        if(!cheap_settings->lookup("lex-entries-can-fail"))
            throw tError(msg);
        else if(verbosity > 4)
            fprintf(stderr, "%s\n", msg.c_str());
        return expanded;
    }

    /*
    if(!_mods.empty())
    {
        if(!expanded.modify(_mods))
        {
            string m;
            for(modlist::iterator mod = _mods.begin(); mod != _mods.end();
                ++mod)
                m += string("[") + mod->first + string(" : ")
                    + type_name(mod->second) + string("]");
            throw tError(string("invalid lex_stem `") + printname()
                        + "' (cannot apply mods " + m + ")");
        }
    }
*/
    return expanded;
}


std::vector<std::string>
lex_stem::get_stems() {
  fs_alloc_state FSAS;
  vector <string> orth;
  struct dag_node *dag = FAIL;
  fs e = instantiate();
   
  if(e.valid()) 
    dag = dag_get_path_value(e.dag(),
                             cheap_settings->req_value("orth-path"));
  if(dag == FAIL) {
    fprintf(ferr, "no orth-path in `%s'\n", type_name(_instance_type));
    if(verbosity > 9) {
      dag_print(stderr, e.dag());
      fprintf(ferr, "\n");
    }
        
    orth.push_back("");
    return orth;
  }

  list <struct dag_node *> stemlist = dag_get_list(dag);

  if(stemlist.size() == 0) {
    // might be a singleton
    if(is_type(dag_type(dag))) {
      string s(type_name(dag_type(dag)));
      switch(s[0]) {
      case '"':
        orth.push_back(s.substr(1, s.length()-2));
        break;
      case '\'':
        orth.push_back(s.substr(1, s.length()-1));
        break;
      default:
        orth.push_back(s);
        break;
      }
    }
    else {
      fprintf(ferr, "no valid stem in `%s'\n", type_name(_instance_type));
    }
        
    return orth;
  }
    
  int n = 0;

  for(list<dag_node *>::iterator iter = stemlist.begin();
      iter != stemlist.end(); ++iter) {
    dag = *iter;
    if(dag == FAIL) {
      fprintf(ferr, "no stem %d in `%s'\n", n, type_name(_instance_type));
      return vector<string>();
    }
        
    if(is_type(dag_type(dag))) {
      string s(type_name(dag_type(dag)));
      orth.push_back(s.substr(1,s.length()-2));
    } else {
      fprintf(ferr, "no valid stem %d in `%s'\n",n,type_name(_instance_type));
      return vector<string>();
    }
    n++;
  }
    
  return orth;
}

lex_stem::lex_stem(type_t instance_type //, const modlist &mods
                   , type_t lex_type
                   , const std::list<std::string> &orths)
  : _id(next_id++), _instance_type(instance_type)
  , _lexical_type(lex_type == -1 ? leaftype_parent(instance_type) : lex_type)
                  // , _mods(mods)
  , _orth(0) {
  
  if(orths.size() == 0) {
    vector<string> orth = get_stems();
    _nwords = orth.size();
      
    if(_nwords == 0) return; // invalid entry
      
    _orth = new char*[_nwords];
        
    for(int j = 0; j < _nwords; j++) {
#ifdef HAVE_ICU
      string lc_str = Conv->convert(Conv->convert(orth[j]).toLower());
      _orth[j] = strdup(lc_str.c_str());
#else
      _orth[j] = strdup(orth[j].c_str());
      strtolower(_orth[j]);
#endif
    }
  } else {
    _nwords = orths.size();
    _orth = new char *[_nwords];
    int j = 0;
    for(list<string>::const_iterator it = orths.begin(); it != orths.end();
        ++it, ++j) {
#ifdef HAVE_ICU
      string lc_str = Conv->convert(Conv->convert(*it).toLower());
      _orth[j] = strdup(lc_str.c_str());
#else
      _orth[j] = strdup(it->c_str());
      strtolower(_orth[j]);
#endif
    }
  }

  if(verbosity > 14) {
    print(fstatus); fprintf(fstatus, "\n");
  }
}


lex_stem::~lex_stem()
{
  for(int j = 0; j < _nwords; j++)
    free(_orth[j]);
  if(_orth)
    delete[] _orth;
}

void
lex_stem::print(FILE *f) const
{
  fprintf(f, "%s:", printname());
  for(int i = 0; i < _nwords; i++)
    fprintf(f, " \"%s\"", _orth[i]);
}

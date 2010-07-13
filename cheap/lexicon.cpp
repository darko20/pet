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
#include "settings.h"
#include "logging.h"
#include <boost/algorithm/string/case_conv.hpp>

using boost::algorithm::to_upper;
using boost::algorithm::to_lower;
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
        else 
            LOG(logGrammar, DEBUG, msg);
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
    LOG(logGrammar, WARN,
        "no orth-path in `" << type_name(_instance_type) << "'");
    LOG(logGrammar, DEBUG, e.dag());
        
    orth.push_back("");
    return orth;
  }

  list<dag_node*> stemlist = dag_get_list(dag);

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
      LOG(logGrammar, WARN, 
          "no valid stem in `" << type_name(_instance_type) << "'") ;
    }
        
    return orth;
  }
    
  int n = 0;

  for(list<dag_node *>::iterator iter = stemlist.begin();
      iter != stemlist.end(); ++iter) {
    dag = *iter;
    if(dag == FAIL) {
      LOG(logGrammar, WARN,
          "no stem "<< n << " in `" << type_name(_instance_type) << "'");
      return vector<string>();
    }
        
    if(is_type(dag_type(dag))) {
      string s(type_name(dag_type(dag)));
      orth.push_back(s.substr(1,s.length()-2));
    } else {
      LOG(logGrammar, WARN, "no valid stem " << n 
          << " in `" << type_name(_instance_type) << "'");
      return vector<string>();
    }
    n++;
  }
    
  return orth;
}

void lex_stem::print(std::ostream &out) const
{
  out << printname() << ':';
  for(size_t i = 0; i < _orth.size(); ++i)
    out << " \"" << _orth[i] << "\"";
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
    size_t nwords = orth.size();
      
    if (nwords == 0) return; // invalid entry
      
    _orth.clear();
    _orth.reserve(nwords);
        
    for(int j = 0; j < nwords; j++) {
#ifdef HAVE_ICU
      string lc_str = Conv->convert(Conv->convert(orth[j]).toLower());
      _orth.push_back(lc_str);
#else
      _orth.push_back(orth[j]);
      to_lower(_orth[j]);
#endif
    }
  } else {
    size_t nwords = orths.size();
    _orth.clear();
    _orth.reserve(nwords);
    for(list<string>::const_iterator it = orths.begin(); it != orths.end(); ++it) {
#ifdef HAVE_ICU
      string lc_str = Conv->convert(Conv->convert(*it).toLower());
      _orth.push_back(lc_str);
#else
      _orth.push_back(it->c_str());
      to_lower(_orth.back());
#endif
    }
  }

  LOG(logGrammar, DEBUG, *this);
}


lex_stem::~lex_stem()
{
}


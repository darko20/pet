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

/* POS tags */

#include "cheap.h"
#include "item.h"
#include "grammar.h"
#include "utility.h"
#include "postags.h"

using std::string;
using std::list;
using std::map;
using std::set;
using std::vector;

postags::postags(const vector<string> &tags, const vector<double> &probs)
{
    assert(tags.size() == probs.size());
    for(vector<string>::const_iterator it = tags.begin(); it != tags.end();
        ++it)
    {
        _tags.insert(*it);
        _probs[*it] = probs[it - tags.begin()];
    }
}

postags::postags(const class lex_stem * ls)
{
  if(ls != NULL) {
    set<string> tags = cheap_settings->smap("type-to-pos", ls->type());
    
    for(set<string>::iterator it = tags.begin(); it != tags.end(); ++it)
    {
        add(*it);
    }
  }
}

postags::postags(const list< class tItem *> &les)
{
    for(list<tItem *>::const_iterator it = les.begin(); it != les.end();
        ++it)
    {
        tLexItem *le = dynamic_cast<tLexItem *>(*it);
        if (le != NULL) 
          add(le->get_supplied_postags());
    }
}

void
postags::add(string s) 
{
    _tags.insert(s);
}

void
postags::add(string s, double prob) 
{
    _tags.insert(s);
    _probs[s] = prob;
}

void
postags::add(const postags &s)
{
    for(set<string>::const_iterator iter = s._tags.begin();
        iter != s._tags.end(); ++iter)
    {
        add(*iter);
    }
}


bool
postags::operator==(const postags &b) const
{
    return _tags == b._tags;
}


/** \todo Test the new implementation of this method for case insensitive
 *  search.
 */
bool
postags::contains(const string &s) const
{
  /*
   for(set<string>::const_iterator iter = _tags.begin();
        iter != _tags.end(); ++iter)
    {
        if(strcasecmp(iter->c_str(), s.c_str()) == 0)
            return true;
    }
    return false;
  */
  return (_tags.find(s) != _tags.end());
}

void
postags::remove(string s) 
{
    _tags.erase(s);
}

void
postags::remove(const postags &s)
{
    for(set<string>::const_iterator iter = s._tags.begin();
        iter != s._tags.end(); ++iter)
    {        
        remove(*iter);
    }
}

void
postags::print(FILE *f) const
{
    for(set<string>::const_iterator iter = _tags.begin(); iter != _tags.end();
        ++iter)
    {
        fprintf(f, " %s", iter->c_str());
        map<string, double>::const_iterator p = _probs.find(*iter);
        if(p != _probs.end())
            fprintf(f, " %.2g", p->second);
    }
}

// similar to print(FIle *f) above, but returns a string
string
postags::getPrintString() const
{
  string out = "";
  bool start=true;
  for(set<string>::const_iterator iter = _tags.begin(); iter != _tags.end();
      ++iter)
    {
      if (start) 
        start=false;
      else
        out += " ";
      out += iter->c_str();
    }
  return out;
}

// Determine if given type is licensed under posmapping
// Find first matching tuple in mapping.
bool
postags::contains(type_t t, const class setting *set) const
{
  if(_tags.empty())
    return false;
  
  for(int i = 0; i < set->n; i+=2)
    {
      if(i+2 > set->n)
        {
          fprintf(ferr, "warning: incomplete last entry "
                  "in POS mapping - ignored\n");
          break;
        }
            
      char *lhs = set->values[i], *rhs = set->values[i+1];
            
      int type = lookup_type(rhs);
      if(type == -1)
        {
          fprintf(ferr, "warning: unknown type `%s' in POS mapping\n", rhs);
        }
      else
        {
          if(subtype(t, type) && contains(lhs))
              return true;
        }
    }
  return false;
}

bool
postags::contains(type_t t) const
{
  setting *set = cheap_settings->lookup("posmapping");
  return ((set != NULL) && contains(t, set));
}  
  
bool
postags::license(type_t t) const
{
  setting *set = cheap_settings->lookup("posmapping");
  return ((set == 0) || contains(t, set));
}

void
postags::tagsnprobs(std::vector<std::string> &tagslist,
                    std::vector<double> &probslist) const
{
  std::map<std::string, double, ltstr>::const_reverse_iterator it;
  for (it = _probs.rbegin(); it != _probs.rend(); it++) {
    tagslist.push_back((*it).first.c_str());
    probslist.push_back((*it).second);
  }
  assert(tagslist.size() == probslist.size());
}

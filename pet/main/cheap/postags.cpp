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

#include "pet-system.h"
#include "cheap.h"
#include "item.h"
#include "grammar.h"
#include "../common/utility.h"
#include "postags.h"

postags::postags(const vector<string> &tags, const vector<double> &probs)
{
    for(vector<string>::const_iterator it = tags.begin(); it != tags.end();
        ++it)
    {
        _tags.insert(*it);
        _probs[*it] = probs[it - tags.begin()];
    }
}

postags::postags(const class full_form ff)
{
    if(!ff.valid())
        return;
    
    set<string> tags = cheap_settings->smap("type-to-pos", ff.stem()->type());
    
    for(set<string>::iterator it = tags.begin(); it != tags.end(); ++it)
    {
        add(*it);
    }
}

postags::postags(const list<lex_item *> &les)
{
    for(list<lex_item *>::const_iterator it = les.begin(); it != les.end();
        ++it)
    {
        add((*it)->get_supplied_postags());
    }
}

void
postags::add(string s) 
{
    _tags.insert(s);
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

bool
postags::contains(string s) const
{
    for(set<string>::const_iterator iter = _tags.begin();
        iter != _tags.end(); ++iter)
    {
        if(strcasecmp(iter->c_str(), s.c_str()) == 0)
            return true;
    }
    return false;
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

bool
postags::contains(type_t t) const
{
    if(_tags.empty())
        return false;
    
    bool contained = false;
    
    setting *set = cheap_settings->lookup("posmapping");
    if(set)
    {
        for(int i = 0; i < set->n; i+=3)
        {
            if(i+3 > set->n)
            {
                fprintf(ferr, "warning: incomplete last entry "
                        "in POS mapping - ignored\n");
                break;
            }
            
            char *lhs = set->values[i], *rhs = set->values[i+2];
            
            int type = lookup_type(rhs);
            if(type == -1)
            {
                fprintf(ferr, "warning: unknown type `%s' "
                        "in POS mapping\n", rhs);
            }
            else
            {
                if(subtype(t, type))
                {
                    if(contains(lhs))
                    {
                        contained = true;
                        break;
                    }
                }
            }
        }
    }
    return contained;
}

// Compute priority of given type under posmapping
// Find first matching tuple in mapping, and return this priority
// If no match found, return initialp
int
postags::priority(const char *settingname, type_t t, int initialp) const
{
    setting *set = cheap_settings->lookup(settingname);
    if(set == 0)
        return initialp;
    
    for(int i = 0; i < set->n; i+=3)
    {
        if(i+3 > set->n)
        {
            fprintf(ferr, "warning: incomplete last entry "
                    "in POS mapping `%s' - ignored\n", settingname);
            break;
        }
        
        char *lhs = set->values[i],
               *p = set->values[i+1],
             *rhs = set->values[i+2];
        
        int prio = strtoint(p, "as priority value in POS mapping");
        int type = lookup_type(rhs);
        
        if(type == -1)
        {
            fprintf(ferr, "warning: unknown type `%s' in POS mapping\n",
                    rhs);
        }
        else
        {
            if(subtype(t, type) && contains(lhs))
                return prio;
        }
    }
    return initialp;
}

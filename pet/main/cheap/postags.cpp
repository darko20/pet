/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* POS tags */

#include "pet-system.h"
#include "cheap.h"
#include "item.h"
#include "grammar.h"
#include "../common/utility.h"
#include "postags.h"

postags::postags(const class full_form ff)
{
  if(!ff.valid())
    return;

  setting *set;
  if((set = cheap_settings->lookup("type-to-pos")) != 0)
    {
      for(int i = 0; i < set->n; i+=2)
        {
          if(i+2 > set->n)
            {
              fprintf(ferr, "warning: incomplete last entry in "
		      "type to POS mapping - ignored\n");
              break;
            }
      
          char *lhs = set->values[i], *rhs = set->values[i+1];
          int id = lookup_type(lhs);
          if(id != -1)
            {
              if(subtype(ff.stem()->type(), id))
                {
                  add(string(rhs));
                }
            }
          else
	    fprintf(ferr, "warning: unknown type `%s' in "
		    "type to POS mapping - ignored\n", lhs);
	  
        }
    }
}

postags::postags(const list<lex_item *> &les)
{
  for(list<lex_item *>::const_iterator it = les.begin(); it != les.end(); ++it)
    add((*it)->get_supplied_postags());
}

void postags::add(string s) 
{
  _tags.insert(s);
}

void postags::add(const postags &s)
{
  for(set<string>::const_iterator iter = s._tags.begin();
      iter != s._tags.end(); ++iter)
    add(*iter);
}

bool postags::operator==(const postags &b) const
{
  return _tags == b._tags;
}

bool postags::contains(string s) const
{
  for(set<string>::const_iterator iter = _tags.begin();
      iter != _tags.end(); ++iter)
    {
      if(strcasecmp(iter->c_str(), s.c_str()) == 0)
	return true;
    }
  return false;
}

void postags::remove(string s) 
{
  _tags.erase(s);
}

void postags::remove(const postags &s)
{
  for(set<string>::const_iterator iter = s._tags.begin();
      iter != s._tags.end(); ++iter)
    remove(*iter);
}

void postags::print(FILE *f) const
{
  for(set<string>::const_iterator iter = _tags.begin(); iter != _tags.end(); ++iter)
    {
      fprintf(f, " %s", iter->c_str());
    }
}

bool postags::contains(type_t t) const
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
// If not match found, return initialp
int
postags::priority(type_t t, int initialp) const
{
  setting *set = cheap_settings->lookup("posmapping");
  if(set == 0)
    return initialp;

  for(int i = 0; i < set->n; i+=3)
    {
      if(i+3 > set->n)
        {
          fprintf(ferr, "warning: incomplete last entry "
		  "in POS mapping - ignored\n");
          break;
        }
      
      char *lhs = set->values[i], *p = set->values[i+1],
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

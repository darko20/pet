/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* POS tags */

#include <strings.h>
#include <algorithm>
#include "postags.h"

bool postags::contains(string s) const
{
  for(set<string>::const_iterator iter = _tags.begin(); iter != _tags.end(); ++iter)
    {
      if(strcasecmp(iter->c_str(), s.c_str()) == 0)
	return true;
    }
  return false;
}

void postags::remove(string s) 
{
  for(set<string>::iterator iter = _tags.begin(); iter != _tags.end(); ++iter)
    {
      if(strcasecmp(iter->c_str(), s.c_str()) == 0)
	_tags.erase(iter);
    }
}

void postags::print(FILE *f) const
{
  for(set<string>::const_iterator iter = _tags.begin(); iter != _tags.end(); ++iter)
    {
      fprintf(f, " %s", iter->c_str());
    }
}

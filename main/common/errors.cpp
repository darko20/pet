/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* defines classes representing errors - used in exceptions */

#include "errors.h"

#ifdef TSDBAPI
#include "tsdb++.h"
#endif

error::error()
  : _msg("unknown error")
{
}

error::error(string s)
  : _msg(s)
{
}
  
error::~error()
{
}

void error::print(FILE *f)
{
  fprintf(f, "%s", _msg.c_str());
}

#ifdef TSDBAPI
void error::tsdb_print()
{
  string s;
  for(string::size_type i = 0; i < _msg.length(); i++)
    {
      if(_msg[i] == '"')
	s += "<quote>";
      else
	s += string(1, _msg[i]);
    }

  capi_printf("%s", s.c_str());
}
#endif

error_ressource_limit::error_ressource_limit(string lim, int val)
  : _lim(lim), _val(val)
{
}

void error_ressource_limit::print(FILE *f)
{
  fprintf(f, "%s exhausted: %d", _lim.c_str(), _val);
}

#ifdef TSDBAPI
void error_ressource_limit::tsdb_print()
{
  capi_printf("%s exhausted: %d", _lim.c_str(), _val);
}
#endif

string error_ressource_limit::msg()
{
  char foo[42];

  sprintf(foo, "%d", _val);
  return _lim + string(" exhausted: ") + string(foo);
}


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

/* defines class representing errors - used in exceptions */

#include "pet-system.h"
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



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

/* class `agenda' */

#include "pet-system.h"
#include "agenda.h"

//#define DEBUG

void agenda::push(basic_task *t)
{
#ifdef DEBUG
  fprintf(ferr, " -> ");
  t->print(ferr);
  fprintf(ferr, "\n");
#endif

  _A.push(t);
}

basic_task *agenda::top()
{
  basic_task *t;
  t = _A.top();
  return t;
}

basic_task *agenda::pop()
{
  basic_task *t = top();

  _A.pop();

  return t;
}

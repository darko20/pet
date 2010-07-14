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

/* internal TDL term representation - constructors and copy constructors */

#include "flop.h"
#include "lex-io.h"
#include "hierarchy.h"
#include "utility.h"
#include <boost/algorithm/string/predicate.hpp>

using std::string;
using boost::iequals;

Term *new_type_term(int id)
{
  Term *T = new Term();
  T ->tag = TYPE;
  T ->value = types.name(id);
  T ->type = id;
  return T;
}

Term *new_avm_term()
{
  Term* T = new Term();
  T->tag = FEAT_TERM;
  T->A = new Avm();
  return T;
}

Term *add_term(Conjunction *C, Term *T)
{
  C->term.push_back(T);
  return T;
}

Conjunction *get_feature(Avm *A, const std::string& feat) /// \todo only used in add_feature
{
  for(int i = 0; i < A -> n(); i ++)
  {
    if (iequals(A->av[i]->attr, feat))
      return A->av[i]->val;
  }
  return NULL;
}

Conjunction *add_feature(Avm *A, const std::string& feat) /// \todo unused function
{
  assert(A != NULL);
  if(attributes.id(feat) == -1) {
    attributes.add(feat);
  }
  Conjunction *C = get_feature(A, feat);
  if (C != NULL) {
    return C;
  }
  Attr_val* av = new Attr_val();
  av->attr = feat;
  av->val = new Conjunction();
  A->add_attr_val(av);
  return av->val;
}

int nr_avm_constraints(Conjunction *C) /// \todo unused
{
  int cnt = 0;

  assert(C != NULL);
  for(int j = 0; j < C -> n(); j++)
  {
    if (C->term[j] ->tag == FEAT_TERM)
      cnt++;
  }
  return cnt;
}


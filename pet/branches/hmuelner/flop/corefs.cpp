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

/* `unify' coreferences in conjunctions
   (eg. [ #1 & #2 ] is converted to [ #1_2 ]) */

#include "flop.h"
#include "logging.h"
#include "utility.h"
#include <cstdlib>
#include <string>
#include <boost/format.hpp>

using std::string;

int add_coref(Coref_table *co, const std::string& name)
{
  int i = 0;

  for( ; i < co->n(); i++)
    {
      if(co->coref[i] == name)
        break;
    }

  if(i < co->n())
    { // already in table
      return i;
    }
  else
    { // add to table
      co->coref.push_back(name);
      return co->n()-1;
    }
}

void new_coref_domain(Coref_table *co) {

  for(int i = 0; i < co->n(); i++)
    {
      if(strchr(co->coref[i].c_str(), '#') == NULL) {
        // old length + hash + four digits + zero char
        string newname = co->coref[i] + (boost::format("#%04d") % i).str();
        co->coref[i] = newname;
      }
    }
}

void find_coref_conjunction(Conjunction *, Coref_table *);

void find_coref_avm(Avm *A, Coref_table *coref)
{

  for (size_t j = 0; j < A -> n(); ++j)
    {
      find_coref_conjunction(A->av[j]->val, coref);
    }
}

void find_coref_list(Tdl_list *L, Coref_table *coref)
{

  for(size_t i = 0; i < L->n(); i++)
    {
      find_coref_conjunction(L->list[i], coref);
    }
  
  if(L->dottedpair)
    find_coref_conjunction(L->rest, coref);

}

void find_coref_term (Term *T, Coref_table *coref)
{
  switch(T->tag)
    {
    case TYPE:
      break;
    case ATOM:
    case STRING:
      break;
    case COREF:
      break;
    case FEAT_TERM:
      find_coref_avm(T->A, coref);
      break;
    case LIST:
    case DIFF_LIST:
      find_coref_list(T->L, coref);
      break;
    case TEMPL_PAR:
    case TEMPL_CALL:
      assert(!"this cannot happen");
      break;
    default:
      LOG(logSyntax, ERROR, "unknown kind of term: " << T->tag);
      assert(false);
      break;
    }
}

void find_coref_conjunction(Conjunction *C, Coref_table *coref)
{
  if(C == NULL) return;

  int first = 1;
  int coidx = -1;
  for(size_t i = 0; i < C -> n(); i++)
    {
      if(C->term[i]->tag == COREF)
        {
          int thisidx;

          thisidx = C->term[i]->coidx;
          if(first)
            coidx = thisidx, first = 0;
          else
            {
              assert(coidx < coref->n());
              assert(thisidx < coref->n());

              string old1 = coref->coref[coidx];
              string old2 = coref->coref[thisidx];
              /* we could use the old name, but i try to be perfect */
              string newname = old1 + "_" +old2;
              
              /* no we have to write this new name to any entries
                 in the coref table equal to  one of the old names */
              for(size_t j = 0; j < coref -> n(); j++)
                if(coref->coref[j] == old1 || coref->coref[j] == old2)
                  coref->coref[j] = newname;
              
              /* now remove coref */
              C->term[i--] = C->term[C->n()-1];
              C->term.resize(C->n()-1);
            }
        }
    }

  for(size_t i = 0; i < C->n(); i++)
    {
      find_coref_term(C->term[i], coref);
    }
}

void find_corefs()
{
  int i;

  for(i = 0; i<types.number(); i++)
    find_coref_conjunction(types[i]->constraint, types[i]->coref);
}

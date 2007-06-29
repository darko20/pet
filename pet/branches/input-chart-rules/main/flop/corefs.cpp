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

int add_coref(struct coref_table *co, char *name)
{
  int i;

  for(i = 0; i < co->n; i++)
    {
      if(strcmp(co->coref[i], name) == 0)
        break;
    }

  if(i < co->n)
    { // already in table
      return i;
    }
  else
    { // add to table
      assert(i < COREF_TABLE_SIZE);
      co->coref[i] = name;
      return co->n++;
    }
}

void find_coref_conjunction(struct conjunction *, struct coref_table *);

void find_coref_avm(struct avm *A, struct coref_table *coref)
{
  int j;

  for (j = 0; j < A -> n; ++j)
    {
      find_coref_conjunction(A -> av[j] -> val, coref);
    }
}

void find_coref_list(struct tdl_list *L, struct coref_table *coref)
{
  int i;

  for(i = 0; i < L -> n; i++)
    {
      find_coref_conjunction(L -> list[i], coref);
    }
  
  if(L -> dottedpair)
    find_coref_conjunction(L -> rest, coref);

}

void find_coref_term (struct term *T, struct coref_table *coref)
{
  switch(T -> tag)
    {
    case TYPE:
      break;
    case ATOM:
    case STRING:
      break;
    case COREF:
      break;
    case FEAT_TERM:
      find_coref_avm(T -> A, coref);
      break;
    case LIST:
    case DIFF_LIST:
      find_coref_list(T -> L, coref);
      break;
    case TEMPL_PAR:
    case TEMPL_CALL:
      assert(!"this cannot happen");
      break;
    default:
      fprintf(ferr, "unknown kind of term: %d\n", T -> tag);
      break;
    }
}

void find_coref_conjunction(struct conjunction *C, struct coref_table *coref)
{
  int i, j;
  int first, coidx;

  if(C == NULL) return;

  first = 1; coidx = -1;
  for(i = 0; i < C -> n; i++)
    {
      if(C -> term[i]->tag == COREF)
        {
          int thisidx;

          thisidx = C -> term[i] -> coidx;
          if(first)
            coidx = thisidx, first = 0;
          else
            {
              char *newname, *old1, *old2;
              assert(coidx < coref -> n);
              assert(thisidx < coref -> n);

              old1 = coref->coref[coidx];
              old2 = coref->coref[thisidx];
              /* we could use the old name, but i try to be perfect */
              newname = (char *) salloc(strlen(old1) + 1 + strlen(old2) + 1);
              strcpy(newname, old1);
              strcat(newname, "_");
              strcat(newname, old2);
              
              /* no we have to write this new name to any entries
                 in the coref table equal to  one of the old names */
              for(j = 0; j < coref -> n; j++)
                if(coref->coref[j] == old1 || coref->coref[j] == old2)
                  coref->coref[j] = newname;
              
              /* now remove coref */
              C->term[i--] = C->term[--C->n];
            }
        }
    }

  for(i = 0; i < C -> n; i++)
    {
      find_coref_term(C -> term[i], coref);
    }
}

void find_corefs()
{
  int i;

  for(i = 0; i<types.number(); i++)
    find_coref_conjunction(types[i] -> constraint, types[i]->coref);
}

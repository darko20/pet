/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* internal TDL term representation - constructors and copy constructors */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "flop.h"

struct avm *new_avm()
{
  struct avm *A;

  A = (struct avm *) salloc(sizeof (struct avm));

  A -> is_special = 0;
  A -> n = 0;
  A -> allocated = AV_TABLE_SIZE;
  A -> av = (struct attr_val **) salloc(A -> allocated * sizeof(struct attr_val*));

  return A;
}

struct tdl_list *new_list()
{
  struct tdl_list *L;

  L = (struct tdl_list *) salloc(sizeof(struct tdl_list));

  L -> difflist = 0;
  L -> openlist = 0;
  L -> dottedpair = 0;

  L -> n = 0;
  L -> list = (struct conjunction **) salloc(LIST_TABLE_SIZE * sizeof(struct conjunction *));
  L -> rest = NULL;

  return L;
}

struct term *new_term()
{
  struct term *T;

  T = (struct term *) salloc(sizeof(struct term));

  T -> tag = NONE;
  T -> value = NULL; T -> type = 0; T -> coidx = 0; T -> L = NULL; T -> A = NULL;
  T -> params = NULL;

  return T;
}

struct attr_val *new_attr_val()
{
  struct attr_val *av;
  
  av = (struct attr_val *) salloc(sizeof(struct attr_val));

  av -> attr = NULL; av -> val = NULL;

  return av;
}

struct term *new_type_term(int id)
{
  struct term *T;

  T = new_term();
  T -> tag = TYPE;

  T -> value = strdup(types.name(id).c_str());
  T -> type = id;
  
  return T;
}

struct term *new_string_term(char *s)
{
  struct term *T;

  T = new_term();
  T -> tag = STRING;
  T -> value = s;
      
  string S = "\"" + string(s) + "\"";
  if(types.id(S) == -1)
    {
      struct type *t;
      t = new_type(S, false);
      t->def = new_location("virtual", 0, 0);
      subtype_constraint(t->id, BI_STRING);
    }

  return T;
}

struct term *new_int_term(int i)
{
  char *s = new char[10];
  sprintf(s, "%d", i);
  
  return new_string_term(s);
}

struct term *new_avm_term()
{
  struct term *T;

  T = new_term();
  T -> tag = FEAT_TERM;

  T -> A = new_avm();

  return T;
}

struct conjunction *new_conjunction()
{
  struct conjunction *C;

  C = (struct conjunction *) salloc(sizeof(struct conjunction));
  C -> n = 0;
  C -> term = (struct term **) salloc(CONJ_TABLE_SIZE * sizeof(struct term *));

  return C;
}

struct term *add_term(struct conjunction *C, struct term *T)
{
  assert(C); assert(C -> n < CONJ_TABLE_SIZE);

  C -> term[C -> n++] = T;
  return T;
}

struct attr_val *add_attr_val(struct avm *A, struct attr_val *av)
{
  assert(A);

  if(A -> n >= A -> allocated)
    {
      A -> allocated += AV_TABLE_SIZE;
      A -> av = (struct attr_val **) realloc(A -> av, A -> allocated * sizeof(struct attr_val *));
    }

  A -> av[A -> n++] = av;

  return av;
}

struct conjunction *add_conjunction(struct tdl_list *L, struct conjunction *C)
{
  assert(L); assert(L -> n < LIST_TABLE_SIZE);
  L -> list [L -> n++] = C;

  return C;
}

struct conjunction *get_feature(struct avm *A, char *feat)
{
  int i = 0;

  for(i = 0; i < A -> n; i ++)
    {
      if(strcasecmp(A -> av[i] -> attr, feat) == 0)
        return A -> av[i] -> val;
    }

  return NULL;
}

struct conjunction *add_feature(struct avm *A, char *feat)
{
  struct attr_val *av;
  struct conjunction *C;

  assert(A != NULL); assert(feat != NULL);

  if(attributes.id(feat) == -1)
    attributes.add(feat);

  if((C = get_feature(A, feat)) != NULL) return C;

  av = new_attr_val();
  av -> attr = feat;
  av -> val = new_conjunction();

  add_attr_val(A, av);

  return av -> val;
}

int nr_avm_constraints(struct conjunction *C)
{
  int j, cnt = 0;

  assert(C != NULL);
  for(j = 0; j < C -> n; j++)
    {
      if (C -> term[j] -> tag == FEAT_TERM)
	cnt++;
    }
  return cnt;
}

struct term *single_avm_constraint(struct conjunction *C)
{
  int j;

  assert(C != NULL);

  if(nr_avm_constraints(C) > 1) return NULL;

  for(j = 0; j < C -> n; j++)
    {
      if (C -> term[j] -> tag == FEAT_TERM)
	return C -> term[j];
    }

  return NULL;
}

struct conjunction *copy_conjunction(struct conjunction *C);

struct avm* copy_avm(struct avm *A)
{
  int i;
  struct avm *A1;

  A1 = (struct avm *) salloc(sizeof(struct avm));

  A1 -> n = A -> n;
  A1 -> is_special = A -> is_special;
  A1 -> allocated = A -> n;
  A1 -> av = (struct attr_val **) salloc(A1 -> allocated * sizeof(struct attr_val*));

  for(i = 0; i < A -> n; i++)
    {
      struct attr_val *av;

      av = (struct attr_val *) salloc(sizeof(struct attr_val));
      av -> attr = (char *) salloc(strlen(A -> av[i] -> attr) + 1);
      strcpy(av -> attr, A -> av[i] -> attr);

      av -> val = copy_conjunction(A -> av[i] -> val);

      A1 -> av[i] = av;
    }
  
  return A1;
}

struct tdl_list *copy_list(struct tdl_list *L)
{
  int i;
  struct tdl_list *L1;

  L1 = (struct tdl_list *) salloc(sizeof(struct tdl_list));

  L1 -> difflist = L -> difflist;
  L1 -> openlist = L -> openlist;
  L1 -> dottedpair = L -> dottedpair;

  if(L -> dottedpair)
    L1 -> rest = copy_conjunction(L -> rest);
  else
    L1 -> rest = NULL;

  L1 -> n = L -> n;
  L1 -> list = (struct conjunction **) salloc(LIST_TABLE_SIZE * sizeof(struct conjunction *));

  for(i = 0; i < L -> n; i++)
    {
      L1 -> list[i] = copy_conjunction(L -> list[i]);
    }

  return L1;
}

struct term *copy_term(struct term *T)
{
  struct term *T1;

  T1 = (struct term *) salloc(sizeof(struct term));

  T1 -> tag = T -> tag;

  if(T -> value != NULL)
    {
      T1 -> value = (char *) salloc(strlen(T -> value) + 1);
      strcpy(T1 -> value, T -> value);
    }
  else
    T1 -> value = NULL;

  T1 -> type = T -> type;
  T1 -> coidx = T -> coidx;

  if(T -> tag == LIST || T -> tag == DIFF_LIST)
    T1 -> L = copy_list(T -> L);
  else
    T1 -> L = NULL;

  if(T -> tag == FEAT_TERM)
    T1 -> A = copy_avm(T -> A);
  else
    T1 -> A = NULL;

  T1 -> params = T -> params; /* this is _NOT_ copied - that's ok the way templates are
				 implemented now, but must be kept in mind when making
				 changes to the template mechanism */

  return T1;
}

struct conjunction *copy_conjunction(struct conjunction *C)
{
  int i;
  struct conjunction *C1;

  if(C == 0) return C;

  C1 = (struct conjunction *) salloc(sizeof(struct conjunction));

  C1 -> n = C -> n;
  C1 -> term = (struct term **) salloc(CONJ_TABLE_SIZE * sizeof(struct term*));

  for(i = 0; i < C -> n; i++)
    {
      C1 -> term[i] = copy_term(C -> term[i]);
    }

  return C1;
}

/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class to represent bitvectors of fixed size */

#include "bitcode.h"
#include "errors.h"

bitcode::bitcode(int n)
{
  sz = n; 
  register int i = 1 + sz/SIZE_OF_WORD;

  V = new CODEWORD[i];
  stop = V + i;

  first_set = last_set = 0; // no bit is set

  while (i--) V[i]=0;
}

bitcode::bitcode(const bitcode& b)
{
  sz = b.sz;
  register int n = 1 + sz/SIZE_OF_WORD;

  V = new CODEWORD[n];
  stop = V + n;
  first_set = b.first_set; last_set = b.last_set;

  while (n--) V[n] = b.V[n];
}

bitcode& bitcode::operator=(const bitcode& b)
{
  if (this == &b) return *this;
  register int n = 1 + b.sz/SIZE_OF_WORD;
  if(sz != b.sz)
    {
      delete[] V;
      sz = b.sz;
      V = new CODEWORD[n];
      stop = V + n;
    }

  first_set = b.first_set; last_set = b.last_set;
  for(CODEWORD *p = V, *q = b.V; p < stop; p++, q++) *p = *q;

  return *this;
}

void bitcode::find_relevant_parts()
{
  first_set = last_set = -1;
  for(CODEWORD *p = V; p < stop && last_set == -1; p++)
  {
    if(first_set == -1)
    {
      if(*p != 0)
        first_set = p - V;
    }
    else if(last_set == -1)
    {
       if(*p == 0)
         last_set = p - V;
    }
  }

  if(first_set == -1)
    first_set = last_set = 0; // no bit set
  if(last_set == -1)
    last_set = stop - V;
}

void bitcode::clear()
{
  register CODEWORD *p = V;
  while (p < stop) *p = 0;
  first_set = last_set = 0; // no bit set
}

list_int *bitcode::get_elements()
{
  CODEWORD *p, w;
  int i;
  list_int *l = 0;

  for(p = V, i = 0; p < stop; p++, i++)
    if(*p)
      {
	w = *p;

	for(int j = 0; j < SIZE_OF_WORD; j++)
	  {
	    if((w & (CODEWORD)1) == 1)
	      l = cons(i*SIZE_OF_WORD + j, l);
	    w >>= 1;
	  }
      }

  return l;
}

bitcode& bitcode::join(const bitcode& b)
{
  CODEWORD *p, *q;

  for(p = V, q = b.V; p<stop; p++, q++) *p |= *q;

  return *this;
}

bitcode& bitcode::intersect(const bitcode& b)
{
  CODEWORD *p, *q;

  for(p = V, q = b.V; p<stop; p++, q++) *p &= *q;

  return *this;
}

bool intersect_empty(const bitcode &A, const bitcode &B, bitcode *C)
{
  CODEWORD *p, *q, *s;
  bool empty;

  for(p = A.V, q = B.V, s = C -> V, empty = true; p < A.stop; p++, q++, s++)
    if((*s = *p & *q) != 0) empty = false;

  return empty;
}

bool bitcode::subset(const bitcode &supposed_superset)
{
  CODEWORD *sub, *super;

  for(sub = V, super = supposed_superset.V; sub < stop; sub++, super++)
    if((*sub & *super) != *sub) return false;

  return true;
}

bool bitcode::subset_fast(const bitcode &supposed_superset)
{
  // assumes first_set and last_set is set correctly
  CODEWORD *sub, *super;

  if(first_set < supposed_superset.first_set ||
     last_set > supposed_superset.last_set)
    return false;

  for(sub = V + supposed_superset.first_set,
      super = supposed_superset.V + supposed_superset.first_set;
      sub < V + supposed_superset.last_set; sub++, super++)
    if((*sub & *super) != *sub) return false;

  return true;
}


bitcode& bitcode::complement()
{
  for(CODEWORD *p = V; p<stop; p++) *p = ~(*p);

  return *this;
}

bitcode bitcode::operator|(const bitcode& b)
{
  bitcode res(*this);

  return res.join(b);
}

bitcode bitcode::operator&(const bitcode& b)
{
  bitcode res(*this);

  return res.intersect(b);
}

bitcode bitcode::operator~()
{
  bitcode res(*this);

  return res.complement();
}

int bitcode::empty() const
{
  for(CODEWORD *p = V; p < stop; p++)
    if(*p != 0) return 0;

  return 1;
}

void bitcode::print(FILE *f) const
{
  for(CODEWORD *p = V; p < stop; p++)
    fprintf(f, "%.8X", *p);
}

#ifdef NAIVE_BITCODE_DUMP

void bitcode::dump(dumper *f)
{
  short int s = 1 + sz/SIZE_OF_WORD;

  f->dump_short(s);

  for(CODEWORD *p = V; p < stop; p++)
    f->dump_int(*p);
}

void bitcode::undump(dumper *f)
{
  short int s;

  s = f->undump_short();

  if(s != (1 + sz/SIZE_OF_WORD))
  {
    fprintf(ferr, "mismatch %d vs %d\n", s, 1+sz/SIZE_OF_WORD);
  }

  for(CODEWORD *p = V; p < stop; p++)
    *p = f->undump_int();
}

#else

void bitcode::dump(dumper *f)
{
  int n = 1 + sz/SIZE_OF_WORD;
  int i = 0;

  while(i < n)
    {
      f->dump_int(V[i]);
      if(V[i] == 0)
	{
	  int j = i + 1;

	  while(j < n && V[j] == 0)
	    j++;

	  f->dump_short((short int) (j - i));

	  i = j;
	}
      else
	i++;
    }

  f->dump_int(0);
  f->dump_short(0);
}

void bitcode::undump(dumper *f)
{
  int n = 1 + sz/SIZE_OF_WORD;
  int i = 0;

  while(i < n)
    {
      V[i] = f->undump_int();
      if(V[i] == 0)
	{
	  int l = f->undump_short();

	  while(--l > 0)
	    {
	      i++;
	      if(i >= n)
		throw error("invalid compressed bitcode (too long)");
	      V[i] = 0;
	    }
	}
      i++;
    }

  if( f->undump_int() != 0 || f->undump_short() != 0)
    throw error("invalid compressed bitcode (no end marker)");

  find_relevant_parts();
}

#endif

int Hash(const bitcode &C)
{
  
  for(CODEWORD *p = C.V; p < C.stop; p++)
    if(*p != 0)
      {
	for(int j = 0; j < C.SIZE_OF_WORD ; j++)
	  {
	    if(*p & (1 << j))
	      return (p - C.V) * C.sz + j;
	  }
      }

  //  CODEWORD *stop = C.V + C.sz/C.SIZE_OF_WORD + 1;
  //  for(CODEWORD *p = C.V; p < stop; p++)
  //    if(*p != 0) return *p;
  
  return 0;
}

ostream& operator<<(ostream& O, const bitcode& C)
{
  for(int i=0; i < C.sz; i++)
    {
      if(C.member(i)) O << i << " "; 
    }
  O << endl;
  return O;
}

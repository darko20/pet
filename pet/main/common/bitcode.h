/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

// class to represent bitvectors of fixed size
// represents set of integers in the interval [ 0 .. sz [
// performance of some operations is critical for efficient glb computation

#ifndef _BITCODE_H_
#define _BITCODE_H_

#include <stdio.h>
#include <iostream.h>
#ifdef HASH_MAP_AVAIL
#include <hash_map>
#endif
#include <list-int.h>
#include <dumper.h>

typedef unsigned int CODEWORD;

class bitcode {

  static const int SIZE_OF_WORD = (8*sizeof(CODEWORD));

  CODEWORD *V, *stop;
  int sz;

 public:

  bitcode(int n); 
  
  bitcode(const bitcode&);
  ~bitcode() { delete[] V; } 

  void insert(int x);
  void del(int x);

  int  member(int x) const;

  list_int *get_elements();

  int max() const;
  void clear();
  int empty() const;

  void print(FILE *f) const;

  void  dump(dumper *f);
  void undump(dumper *f);

  bitcode& join(const bitcode& );
  bitcode& intersect(const bitcode&);
  bitcode& complement();

  bool subset(const bitcode&);

  bitcode& operator=(const bitcode& S1);

  bitcode& operator|=(const bitcode&);
  bitcode& operator&=(const bitcode&);

  bitcode  operator|(const bitcode& S1);
  bitcode  operator&(const bitcode& S1);

  bitcode  operator~();

  bool operator==(const bitcode& T) const;

  friend int Hash(const bitcode& C);
  friend int compare(const bitcode &S1, const bitcode &S2);
  friend bool intersect_empty(const bitcode&, const bitcode&, bitcode *);

  friend ostream& operator<<(ostream& O, const bitcode& C); 
  friend bool operator<(const bitcode &, const bitcode&);
  friend bool operator>(const bitcode &, const bitcode&);
};

inline int bitcode::max() const { return sz - 1; }

inline int  bitcode::member(int x)  const
{
  return V[ x / SIZE_OF_WORD ] & (1 << (x % SIZE_OF_WORD)); 
}

inline void bitcode::insert(int x) 
{ 
  V[ x / SIZE_OF_WORD ] |= (1 << (x % SIZE_OF_WORD)); 
}

inline void bitcode::del(int x)    
{
  V[ x / SIZE_OF_WORD ] &= ~(1 << (x % SIZE_OF_WORD)); 
}

inline bitcode& bitcode::operator|=(const bitcode& s) { return join(s); }

inline bitcode& bitcode::operator&=(const bitcode& s) { return intersect(s); }

inline bool bitcode::operator==(const bitcode &T) const
{
  CODEWORD *p, *q;
  
  for(p = V + sz/SIZE_OF_WORD, q = T.V + sz/SIZE_OF_WORD; p >= V; p--, q--)
    if(*p != *q) return 0;
  
  return 1;
}

inline int compare(const bitcode &S1, const bitcode &S2)
{
  CODEWORD *p, *q;
  
  for(p = S1.V + S1.sz/S1.SIZE_OF_WORD, q = S2.V + S2.sz/S2.SIZE_OF_WORD; p >= S1.V; p--, q--)
    if(*p != *q)
      {
	if (*p < *q) return -1; else return 1;
      }
  return 0;
}

inline bool operator<(const bitcode& a, const bitcode &b)
{ return compare(a, b) == -1; }

inline bool operator>(const bitcode& a, const bitcode &b)
{ return compare(a, b) ==  1; }

#ifdef HASH_MAP_AVAIL
template<> struct hash<bitcode>
{
  inline size_t operator()(const bitcode &key) const
  {
    return Hash(key);
  }
};
#endif

#endif

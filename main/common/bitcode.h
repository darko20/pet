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

// class to represent bitvectors of fixed size
// represents set of integers in the interval [ 0 .. sz [
// performance of some operations is critical for efficient glb computation

#ifndef _BITCODE_H_
#define _BITCODE_H_

#include <list-int.h>
#include <dumper.h>

typedef unsigned int CODEWORD;

class bitcode {

  static const int SIZE_OF_WORD = (8*sizeof(CODEWORD));

  CODEWORD *V, *stop;
  int first_set, last_set; // index to first/last word != 0, only kept up
                           // to date where possible without added cost!!
  int sz;

  void find_relevant_parts();

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
  void find_relevant_parts() const; // update first_set/last_set

  void print(FILE *f) const;

  void  dump(dumper *f);
  void undump(dumper *f);

  bitcode& join(const bitcode& );
  bitcode& intersect(const bitcode&);
  bitcode& complement();

  bool subset_fast(const bitcode&);
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
  {
    if(*p != *q)
      {
	if (*p < *q) return -1; else return 1;
      }
#ifdef __BORLANDC__
    if(p == S1.V) return 0;
#endif
  }
  return 0;
}

inline bool operator<(const bitcode& a, const bitcode &b)
{ return compare(a, b) == -1; }

inline bool operator>(const bitcode& a, const bitcode &b)
{ return compare(a, b) ==  1; }

#ifdef HASH_MAP_AVAIL
namespace std {
template<> struct hash<bitcode>
{
  inline size_t operator()(const bitcode &key) const
  {
    return Hash(key);
  }
};
}
#endif

#endif

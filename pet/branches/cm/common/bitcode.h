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

/** \file bitcode.h
 * Class to represent bitvectors of fixed size
 * represents set of integers in the interval [ 0 .. sz [
 * performance of some operations is critical for efficient glb computation
 *
 * \todo
 * Could try a `zoning' approach here: since most bits are 0, restrict 
 * operations like subtype to just the region containing 1. Need to 
 * keep track of left and right limit for this. See #ifdef ZONING 
 */

#ifndef _BITCODE_H_
#define _BITCODE_H_

#include <cassert>
#include <iosfwd>

/** \c CODEWORD should be an unsigned numeric type that fits best into a CPU
 *  register.
 */
typedef unsigned int CODEWORD;

/** Implementation of efficient fixed size bit vectors.
 * \attention Most functions assume that all bit vectors are of the same size.
 */ 
class bitcode {

  static const int SIZE_OF_WORD = (8*sizeof(CODEWORD));

  inline CODEWORD *end() const { return stop ; }

  inline int wordindex(int pos) const { return pos / SIZE_OF_WORD ; }

  inline CODEWORD bitmask(int pos) const { return (1 << (pos % SIZE_OF_WORD));}

  /** compare this bitcode and \a S2 in lexicographic order 
   *  \return 0 if \a this == \a S2, -1 if \a this < \a S2
   *  , +1 if \a this < \a S2
   */
  int compare(const bitcode &S2) const {
    CODEWORD *p, *q;
    
    for(p = V, q = S2.V; p < end() ; p++, q++) {
      if(*p != *q) {
        if (*p < *q) return -1; else return 1;
      }
    }
    return 0;
  }

  /** Destructive bitwise OR */
  bitcode& join(const bitcode& b) {
    assert(sz == b.sz);
    CODEWORD *p, *q;
    for(p = V, q = b.V; p < end(); p++, q++) *p |= *q;
    return *this;
  }

  /** Destructive bitwise AND */
  bitcode& intersect(const bitcode& b) {
    assert(sz == b.sz);
    CODEWORD *p, *q;
    for(p = V, q = b.V; p < end(); p++, q++) *p &= *q;
    return *this;
  }

  CODEWORD *V, *stop;
  int sz;

#ifdef ZONING
  /** Zoning is not tested. Does it work? */
  int first_set, last_set;
#endif

 public:

  /** Create a bit vector of \a n bits */
  bitcode(int n);

  /** Clone a bit vector */
  bitcode(const bitcode&);

  /** Destroy bit vector */
  ~bitcode() { delete[] V; }

  /** Set bit at position \a x to one */
  void insert(int x) { V[ wordindex(x) ] |= bitmask(x); }

  /** Set bit at position \a x to zero */
  void del(int x){ V[ wordindex(x) ] &= ~ bitmask(x); }

  /** Return true if the bit at position \a x is set to one */
  bool member(int x) const { return ((V[ wordindex(x) ] & bitmask(x)) != 0); }

  /** Collect the positions of all bits that are 1 into the result list */
  struct list_int *get_elements();

  /** Return the index of the last bit in this bitvector (== size() - 1) */
  int max() const { return sz - 1; }
  /** Return the number of bits in this bitvector */
  int size() const { return sz; }

  /** Set all bits to zero */
  void clear() {
    register CODEWORD *p = V;
    while (p < end()) *p++ = 0;
  }
  
  /** Test if the bitvector only contains zeros */
  bool empty() const {
    for(CODEWORD *p = V; p < end(); p++) if(*p != 0) return false;
    return true;
  }

#ifdef ZONING
  void find_relevant_parts() const; // update first_set/last_set
#endif

  /** Print bitcode for debugging purposes */
  void print(FILE *f) const {
    for(CODEWORD *p = V; p < end(); p++)
      fprintf(f, "%.8X", *p);
  }

  /** @name Serialize/Deserialize bitvector.
   * The Serialization is optimized for bitvectors containing sequences of
   * empty codewords, which are stored using a run-length encoding.
   *
   * Plain (naive) bitcode dumping can be used by defining the symbol
   * \c NAIVE_BITCODE_DUMP, but the same method has to be used in the undumping
   * application then.
   */
  /*@{*/
  void dump(class dumper *f);
  void undump(class dumper *f);
  /*@}*/

  /** Return \c true if the bitvector \f$ \mbox{this}\wedge a = \mbox{this}\f$,
   *  i.e., the bits set in this bitvector are a subset of the bits set in \a a
   *  \pre The bitvectors have to be of equal size for this function to work
   *  correctly.
   */
  bool subset(const bitcode& a);

  /** Assignment. Works also for bit vectors of different size. */
  bitcode& operator=(const bitcode& S1);

  /** Destructive  bitwise OR
   *  \pre The bitvectors have to be of equal size.
   */
  bitcode& operator|=(const bitcode& s) { return join(s); }
  /** Destructive bitwise AND
   *  \pre The bitvectors have to be of equal size.
   */
  bitcode& operator&=(const bitcode& s) { return intersect(s); }
  /** Destructive bitwise NOT */
  bitcode& complement(){
    for(CODEWORD *p = V; p<end(); p++) *p = ~(*p);
    /* Delete the bits that are not used because they are beyond max()
    --p; 
    *p &= (((CODEWORD) -1) >> SIZE_OF_WORD - (sz % SIZE_OF_WORD))
    */
    return *this;
  }

  /** Non-destructive bitwise OR
   *  \pre The bitvectors have to be of equal size.
   */
  bitcode& operator|(const bitcode& b){
    bitcode res(*this);
    return res.intersect(b);
  }
  /** Non-destructive bitwise AND
   *  \pre The bitvectors have to be of equal size.
   */
  bitcode& operator&(const bitcode& b){
    bitcode res(*this);
    return res.join(b);
  }
  /** Non-destructive bitwise NOT.
   *  \pre The bitvectors have to be of equal size.
   */
  bitcode operator~(){
    bitcode res(*this);
    return res.complement();
  }
  
  /** Return \c true if this bitvector is bitwise equal to \a T
   *  \pre The bitvectors have to be of equal size for this function to work
   *  correctly.
   */
  bool operator==(const bitcode& T) const {
    CODEWORD *p, *q;
    assert(sz == T.sz);
    for(p = V, q = T.V; p < end(); p++, q++)
      if(*p != *q) return 0;
    
    return 1;
  }

  /** A more efficient hash function for bitcodes in type hierarchies: return
   *  the number of the first nonzero bit.
   */
  friend int Hash(const bitcode& C);

  /** Check subset relations between \a A and \a B in parallel and store result
   *  in \a a and \a b.
   *  \return \a a is true iff \f$ A \subseteq B \f$ and \a b is true iff \f$ B
   *  \subseteq A \f$ 
   *  \pre The bitvectors have to be of equal size for this function to work
   *  correctly.
   */
  friend void subset_bidir(const bitcode&A, const bitcode &B, bool &a, bool &b);
  /** Return true if the bitwise \c and of \a A and \a B is empty, and return
   *  the result of the \c and operation in \a C.
   *  \pre All bitvectors have to be of equal size for this function to work
   *  correctly.
   */
  friend bool intersect_empty(const bitcode&A, const bitcode&B, bitcode *C);
  
  /** Print bitcode for debugging purposes */
  friend std::ostream& operator<<(std::ostream& O, const bitcode& C);

  /** @name Comparison
   * Compare bitvectors in reverse lexicographic order
   */
  /*@{*/
  friend bool operator<(const bitcode &, const bitcode&);
  friend bool operator>(const bitcode &, const bitcode&);
  /*@}*/
};

/** Print bitcode for debugging purposes */
std::ostream& operator<<(std::ostream& O, const bitcode& C);

/** @name Comparison
 * Compare bitvectors in reverse lexicographic order
 */
/*@{*/
inline bool operator<(const bitcode& a, const bitcode &b)
{ return a.compare(b) == -1; }

inline bool operator>(const bitcode& a, const bitcode &b)
{ return a.compare(b) ==  1; }
/*@}*/

#include "hashing.h"

#ifdef HASH_SPACE
namespace HASH_SPACE {
  template<> struct hash<bitcode> {
    inline size_t operator()(const bitcode &key) const {
      return Hash(key);
    }
  };
}
#endif

#endif

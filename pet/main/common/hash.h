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

/* This is based on lookup2.c by Bob Jenkins, see
 *  http://burtleburtle.net/bob/hash/index.html
 */

#ifndef _HASH_H_
#define _HASH_H_

#include <vector>

typedef  unsigned long  int  ub4;   /* unsigned 4-byte quantities */
typedef  unsigned       char ub1;

#define hashsize(n) ((ub4)1<<(n))
#define hashmask(n) (hashsize(n)-1)

/**
 * hash() -- hash a variable-length key into a 32-bit value
 *   k     : the key (the unaligned variable-length array of bytes)
 *   len   : the length of the key, counting by bytes
 *   level : can be any 4-byte value
 * Returns a 32-bit value.  Every bit of the key affects every bit of
 * the return value.  Every 1-bit and 2-bit delta achieves avalanche.
 * About 36+6len instructions.
 * 
 * The best hash table sizes are powers of 2.  There is no need to do
 * mod a prime (mod is sooo slow!).  If you need less than 32 bits,
 * use a bitmask.  For example, if you need only 10 bits, do
 *   h = (h & hashmask(10));
 * In which case, the hash table should have hashsize(10) elements.
 * 
 * If you are hashing n strings (ub1 **)k, do it like this:
 *   for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);
 * 
 * By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use this
 * code any way you wish, private, educational, or commercial.  It's free.
 * 
 * See http://burlteburtle.net/bob/hash/evahash.html
 * Use for hash table lookup, or anything where one collision in 2^32 is
 * acceptable.  Do NOT use for cryptographic purposes.
 **/
ub4 hash(ub1 *k, ub4 length, ub4 initval);

/**
 * This works on all machines.  hash2() is identical to hash() on 
 * little-endian machines, except that the length has to be measured
 * in ub4s instead of bytes.  It is much faster than hash().  It 
 * requires
 * -- that the key be an array of ub4's, and
 * -- that all your machines have the same endianness, and
 * -- that the length be the number of ub4's in the key
 **/
ub4 hash2(ub4 *k, ub4 length, ub4 initval);

/** Same as hash2 above, but on a vector<int>
 */
ub4 hash2(const std::vector<int> &k, ub4 initval);

/**
 * This is identical to hash() on little-endian machines (like Intel 
 * x86s or VAXen).  It gives nondeterministic results on big-endian
 * machines.  It is faster than hash(), but a little slower than 
 * hash2(), and it requires
 * -- that all your machines be little-endian
 **/
ub4 hash3(ub1 *k, ub4 length, ub4 initval);


#endif

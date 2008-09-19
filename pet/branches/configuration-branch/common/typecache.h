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

/** \file typecache.h
 * Cache for type operations:
 * a hashtable implementation optimized for this purpose, especially glb
 * computation.
 */

#ifndef _TYPECACHE_H_
#define _TYPECACHE_H_

// #define CACHESTATS
// #define PRUNING

#include "chunk-alloc.h"

/** The key type for the type cache */
typedef long long typecachekey_t;

/** @name Compile Time Parameters
 * Note that the initial cache has to fit in one chunk, thus
 * GLB_CACHE_SIZE*sizeof(typecachebucket) must be <= GLB_CHUNK_SIZE
 */
/*@{*/
#define GLB_CACHE_SIZE (12289 /* 49157 */)
#define GLB_CHUNK_SIZE ((int(GLB_CACHE_SIZE*sizeof(typecachebucket)/4096)+1)*4096)
/*@}*/

/** A single linked list of buckets */
struct typecachebucket
{
  /** The key of this bucket */
  typecachekey_t key;
  /** The value of this bucket */
  int value;

#ifdef CACHESTATS
  /** how often was this bucket referenced */
  unsigned short refs;
#endif

  /** list successor (or NULL) */
  struct typecachebucket *next;
};

/** cache for type operations:
 * a hashtable implementation optimized for this purpose, especially glb
 * computation
 */
class typecache
{
 private:
  static const typecachekey_t _no_key = -1;

  chunk_allocator _cache_alloc;
  chunk_alloc_state _initial_alloc;

  int _no_value;

  int _nbuckets;
  typecachebucket *_buckets;

  int _size, _overflows;

#ifdef CACHESTATS
  long int _totalsearches, _totalcl, _totalfp;
  int _maxcl;
#endif

  inline void init_bucket(typecachebucket *b)
    {
      b->key = _no_key; // assumption: legal keys are positive
      b->next = 0;
#ifdef CACHESTATS
      b->refs = 0;
#endif
    }
  
 public:
  
  /** Create a type cache.
   * \param no_value Return this value if the type is not in the cache
   * \param nbuckets The initial number of buckets
   */
  inline typecache(int no_value = 0, int nbuckets = GLB_CACHE_SIZE ) :
    _cache_alloc(GLB_CHUNK_SIZE, true),
    _no_value(no_value), _nbuckets(nbuckets), _buckets(0), _size(0),
    _overflows(0)
    {
      _buckets = (typecachebucket *)
        _cache_alloc.allocate(sizeof(typecachebucket) * _nbuckets);

      _cache_alloc.mark(_initial_alloc);
      // _cache_alloc.print_check();

      for(int i = 0; i < _nbuckets; i++)
        init_bucket(_buckets + i);
#ifdef CACHESTATS
      _totalcl = _totalsearches = _totalfp = _maxcl = 0;
#endif
    }

  inline ~typecache()
    {
      _cache_alloc.reset();
    }

  /** Hash function for the cache. We assume the types are distributed
   *  uniformly.
   */
  inline int hash(typecachekey_t key)
    {
      return key % _nbuckets;
    }

  /** Get the type corresponding to \a key, or the preset \c no_value of the
   *  cache, if not in the cache.
   */
  inline int& operator[](const typecachekey_t key)
    {
      typecachebucket *b = _buckets + hash(key);

#ifdef CACHESTATS
      int cl = 0;
      _totalsearches++;
#endif

      // fast path for standard case
      if(b->key == key)
        {
#ifdef CACHESTATS
          _totalfp++;
          b->refs++;
#endif
          return b->value;
        }

      // is the bucket empty? then insert here
      if(b->key == _no_key)
        {
          _size++;
          b->key = key;
          b->value = _no_value;
          return b->value;
        }

      // search in bucket chain, return if found
      while(b->next != 0)
        {
#ifdef CACHESTATS
          cl++;
#endif
          b = b->next;
          if(b->key == key)
            {
#ifdef CACHESTATS
              _totalcl += cl;
              if(cl > _maxcl) _maxcl = cl;
              b->refs++;
#endif
              return b->value;
            }
        }
#ifdef CACHESTATS
      if(cl > _maxcl) _maxcl = cl;
      _totalcl += cl;
#endif
      // not found, append to end of bucket chain
      _size++; _overflows++;

      b -> next = (typecachebucket *) _cache_alloc.allocate(sizeof(typecachebucket));
      b = b->next;
      init_bucket(b);
      //_cache_alloc.print_check();

      b -> key = key;
      b -> value = _no_value;
      return b->value;
    }

  /** Return the allocator of this cache */
  inline chunk_allocator& alloc() { return _cache_alloc; }

  /** Clear all internal data structures */
  inline void clear()
    {
      _cache_alloc.release(_initial_alloc);
      _cache_alloc.may_shrink();

      for(int i = 0; i < _nbuckets; i++)
        init_bucket(_buckets + i);

      _size = 0;
    }

  /** Try to make the cache smaller by removing infrequently used items */
  inline void prune()
    {
#ifdef CACHESTATS
      LOG(logAppl, DEBUG, "typecache: " << _size << " entries, "
          << _overflows << " overflows, " << _nbuckets << " buckets, "
          << _totalsearches << " searches, " << _totalfp 
          << " fast path, avg chain " << double(_totalcl) / _totalsearches
          << ", max chain " << _maxcl);

#ifdef PRUNING
      int _prune_poss = 0, _prune_done = 0;

      for(int i = 0; i < _nbuckets; i++)
        {
          if(_buckets[i].key != _no_key && _buckets[i].refs == 0)
            {
              _prune_poss++;
              if(_buckets[i].next == 0)
                {
                  _prune_done++;
                  _size--;
                  _buckets[i].key = _no_key; 
                }
            }
        }
      LOG(logAppl, DEBUG,
          "pruning: poss " << _prune_poss << ", done " << _prune_done << "");
#endif

      map<int, int> distr;
      for(int i = 0; i < _nbuckets; i++)
        for(typecachebucket *b = _buckets + i; b != 0; b = b->next)
          if(b->key != _no_key)
            distr[b->refs]++;


      int sum = 0;
      for(map<int,int>::iterator it = distr.begin(); it != distr.end(); ++it)
        {
          sum += it->second;
          if(it->second > 50)
            LOG(logAppl, DEBUG, "  " << it->first << " : " << it->second);
        }
      LOG(logAppl, DEBUG, "  total : " << sum);
#endif
    }
};

#endif

/*
(2362) `if you, can live with a short meeting then maybe we can do it then, otherwise , I think we are going to have to wind up doing it in the morning or, early morning like eight o'clock some day or, in the early evening.' [20000] --- 0 (-0.0|2.5s) <163:5548> (13629.0K) [-1514.1s]
typecache: 33696 entries, 22135 overflows, 12289 buckets, 1348587441 searches, avg chain 0.0797327, max chain 10

(2362) `if you, can live with a short meeting then maybe we can do it then, otherwise , I think we are going to have to wind up doing it in the morning or, early morning like eight o'clock some day or, in the early evening.' [20000] --- 0 (-0.0|2.5s) <163:5548> (13629.0K) [-1507.1s]
typecache: 33696 entries, 9069 overflows, 49157 buckets, 1348587441 searches, avg chain 0.0223881, max chain 5
*/

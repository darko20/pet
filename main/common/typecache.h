/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* cache for type operations:
 * a hashtable implementation optimized for this purpose, especially glb
 * computation
 */

#ifndef _TYPECACHE_H_
#define _TYPECACHE_H_

// #define CACHESTATS
// #define PRUNING

#include "chunk-alloc.h"

typedef long typecachekey_t;

// note that the initial cache has to fit in one chunk, thus
// GLB_CACHE_SIZE*sizeof(typecachebucket) must be <= GLB_CHUNK_SIZE
#define GLB_CACHE_SIZE (12289 /* 49157 */)
#define GLB_CHUNK_SIZE ((int(GLB_CACHE_SIZE*sizeof(typecachebucket)/4096)+1)*4096)

struct typecachebucket
{
  typecachekey_t key; int value;

#ifdef CACHESTATS
  unsigned short refs;
#endif

  struct typecachebucket *next;
};

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
  
  inline typecache(int no_value = 0, int nbuckets = GLB_CACHE_SIZE ) :
    _cache_alloc(GLB_CHUNK_SIZE, true),
    _no_value(no_value), _nbuckets(nbuckets), _buckets(0), _size(0),
    _overflows(0)
    {
      _buckets = (typecachebucket *)
	_cache_alloc.allocate(sizeof(typecachebucket) * _nbuckets);

      _cache_alloc.mark(_initial_alloc);

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

  inline int hash(typecachekey_t key)
    {
      return key % _nbuckets;
    }

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

      b -> key = key;
      b -> value = 0;
      return b->value;
    }

  inline chunk_allocator& alloc() { return _cache_alloc; }

  inline void clear()
    {
      _cache_alloc.release(_initial_alloc);
      _cache_alloc.may_shrink();

      for(int i = 0; i < _nbuckets; i++)
	init_bucket(_buckets + i);

      _size = 0;
    }

  inline void prune()
    {
#ifdef CACHESTATS
      fprintf(ferr, "typecache: %d entries, %d overflows, %d buckets, %ld searches, %ld fast path, avg chain %g, max chain %d\n",
	      _size, _overflows, _nbuckets, _totalsearches, _totalfp, double(_totalcl) / _totalsearches, _maxcl);

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
      fprintf(ferr, "pruning: poss %d, done %d\n", _prune_poss, _prune_done);
#endif

      map<int, int> distr;
      for(int i = 0; i < _nbuckets; i++)
	for(typecachebucket *b = _buckets + i; b != 0; b = b->next)
	  if(b->key != _no_key)
	    distr[b->refs]++;


      int sum = 0;
      for(map<int,int>::iterator iter = distr.begin(); iter != distr.end(); ++iter)
	{
	  sum += iter->second;
	  if(iter->second > 50)
	    fprintf(ferr, "  %d : %d\n", iter->first, iter->second);
	}
      fprintf(ferr, "  total : %d\n", sum);
#endif
    }
};

#endif

#if 0
(2362) `if you, can live with a short meeting then maybe we can do it then, otherwise , I think we are going to have to wind up doing it in the morning or, early morning like eight o'clock some day or, in the early evening.' [20000] --- 0 (-0.0|2.5s) <163:5548> (13629.0K) [-1514.1s]
typecache: 33696 entries, 22135 overflows, 12289 buckets, 1348587441 searches, avg chain 0.0797327, max chain 10

(2362) `if you, can live with a short meeting then maybe we can do it then, otherwise , I think we are going to have to wind up doing it in the morning or, early morning like eight o'clock some day or, in the early evening.' [20000] --- 0 (-0.0|2.5s) <163:5548> (13629.0K) [-1507.1s]
typecache: 33696 entries, 9069 overflows, 49157 buckets, 1348587441 searches, avg chain 0.0223881, max chain 5
#endif

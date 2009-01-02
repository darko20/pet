/* -*- Mode: C++ -*- */
/* \file hashing.h
 * Check if hash maps are available, set \c HASH_SPACE to the namespace
 * containing hash maps, or provide a them using maps.
 *
 * If hashing data structures are not available, \c HASH_SPACE is not defined
 */
#ifndef _HASHING_H
#define _HASHING_H

#include "pet-config.h"
#include "hash.h"

// TODO switch to <tr1/unordered_map> and <tr1/unordered_set>
// cf. http://gcc.gnu.org/gcc-4.3/changes.html
#ifdef HAVE_HASH_MAP
#include <hash_map>
#include <hash_set>

// gcc 4.3 declared ext/hash_map and ext/hash_set as obsolete, moving the
// header files to the default include path backwards
#if ((__GNUC__ == 4) && ( __GNUC_MINOR__ >= 3)) || (__GNUC__ > 4)
#define HASH_SPACE __gnu_cxx
#else
#define HASH_SPACE std
#endif
#endif

#ifdef HAVE_EXT_HASH_MAP
#include <ext/hash_map>
#include <ext/hash_set>

// hopefully a gnu compiler with
#if ((__GNUC__ == 3) && ( __GNUC_MINOR__ > 1)) || (__GNUC__ > 3)
//g++ version 3.2.x or later
#define HASH_SPACE __gnu_cxx
#else
// g++ version 3.0.x or 3.1.x, not recommended
#define HASH_SPACE std
#endif
#endif 

namespace HASH_SPACE {
  /**
   * A standard hash implementation for strings.
   */
  template<> struct hash< std::string >
  {
    inline size_t operator()(const std::string &s) const
    {
      return HASH_SPACE::hash<const char* >()( s.c_str() );
    }
  };
}

/* function object: hash function for pointers */
struct pointer_hash {
  inline size_t operator() (void* p) const {
    return reinterpret_cast<size_t>(p);
  }
};

/* function object: standard hash function for std::string */
struct standard_string_hash {
  inline size_t operator() (const std::string& s) const
  {
    return HASH_SPACE::hash<const char* >()( s.c_str() );
  }
};

/* function object: simple hash function for std::string */
struct simple_string_hash {
  inline size_t operator()(const std::string &key) const {
    size_t v = 0;
    for(unsigned int i = 0; i < key.length(); i++)
      v += key[i];
    return v;
  }
};

/* function object: hash function for std::string by Bob Jenkins */
struct bj_string_hash
{
  inline size_t operator()(const std::string &key) const
  {
    return bjhash((const ub1 *) key.data(), key.size(), 0);
  }
};

#endif

/* -*- Mode: C++ -*- */
/* \file hashing.h
 * Check if hash maps are available, set \c HASH_SPACE to the namespace
 * containing hash maps, or provide a them using maps.
 *
 * If hashing data structures are not available, \c HASH_SPACE is not defined
 */
#ifndef _HASHING_H
#define _HASHING_H

#include "config.h"

#ifdef HAVE_HASH_MAP
#define HASH_SPACE std
#include <hash_map>
#include <hash_set>
#endif

#ifdef HAVE_EXT_HASH_MAP
// hopefully a gnu compiler with
#if (__GNUC__ > 2) && ( __GNUC_MINOR__ > 1)
//g++ version 3.2.x or later
#define HASH_SPACE __gnu_cxx
using namespace HASH_SPACE;
#else
// g++ version 3.0.x or 3.1.x, not recommended
#define HASH_SPACE std
#endif

#include <ext/hash_map>
#include <ext/hash_set>
#endif 

#if ! defined(HAVE_HASH_MAP) && ! defined(HAVE_EXT_HASH_MAP)
// no hash_map available
#undef HASH_SPACE
#include <map>
#include <set>
#define hash_map map
#define hash_set set
#endif

#endif

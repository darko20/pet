/* -*- Mode: C++ -*- */
/* \file hashing.h
 * Check if hash maps are available, set \c HASH_SPACE to the namespace
 * containing hash maps, or provide a them using maps.
 */
#ifndef _HASHING_H
#define _HASHING_H

#ifdef HAVE_HASH_MAP
#if __GNUC__ > 2
#if __GNUC_MINOR__ > 1
#define HASH_SPACE __gnu_cxx
using namespace HASH_SPACE;
#else
#define HASH_SPACE std
#endif
#include <ext/hash_map>
#include <ext/hash_set>
#else
#define HASH_SPACE std
#include <hash_map>
#include <hash_set>
#endif
#else
#define HASH_SPACE std
#define hash_map map
#define hash_set set
#endif

#endif

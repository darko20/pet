/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

// system/platform specific global stuff - this file should be included
// in every file right after the system header files

#ifndef _PET_SYSTEM_H_
#define _PET_SYSTEM_H_

#include <stdlib.h>
#include <stdio.h>
#include <new>
#include <memory>

#ifdef SMARTHEAP
#include "smrtheap.hpp"
#include "yymemory.h"
#undef LOCAL_POOL
#define LOCAL_POOL yy_l2_pool
#else
#define New new
#endif

// include system header files that we need everywhere

// standard C library

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <locale.h>

#ifndef __BORLANDC__
#include <unistd.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <sys\stat.h>
#include <io.h>
#endif

// C++

#include <iostream>
#include <strstream>

// STL

#include <utility>
#include <algorithm>
#include <iterator>
#include <vector>
#include <string>
#include <list>
#include <set>
#include <map>
#include <queue>
#include <stack>

// ICU
#ifdef ICU
#include "unicode/unistr.h"
#include "unicode/schriter.h"
#include "unicode/convert.h"
#endif

using namespace std;

// platform specifics

#ifdef HASH_MAP_AVAIL
#if __GNUC__ > 2
#include <ext/hash_map>
#include <ext/hash_set>
#else
#include <hash_map>
#include <hash_set>
#endif
#else
#define hash_map map
#define hash_set set
#endif

#ifdef __BORLANDC__
#define strcasecmp stricmp
#define R_OK 0
#endif

#endif

#ifdef _RWSTD_MSC22_STATIC_INIT_BUG
#define STRING_NPOS -1
#else
#define STRING_NPOS string::npos
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif

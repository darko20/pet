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

// system/platform specific global stuff - this file should be included
// in every file right after the system header files

#ifndef _PET_SYSTEM_H_
#define _PET_SYSTEM_H_

#ifdef __BORLANDC__
#define _RWSTD_CONTAINER_BUFFER_SIZE 16
#endif

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
#include <sstream>

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
#include "unicode/ucnv.h"
#endif

#ifndef FLOP
using namespace std;
#endif

// platform specifics

#ifdef HASH_MAP_AVAIL
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

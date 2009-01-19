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

/** class to manage chunks of memory for efficient allocation of objects

   this class has several implementations of its low-level memory allocation,
   one that is unix dependend, supporting two heaps with disjunct and
   easy to distinguish address rooms to allow zero-overhead seperation
   of permanent and non-permant dags (colored pointers); another windows-
   specific implementation that uses VirtualAlloc; and a third one that
   relies on standard C++ memory management only */

#include "pet-config.h"
#ifdef HAVE_MMAP
#include <unistd.h>
#endif

#include "chunk-alloc.h"
#include "errors.h"

chunk_allocator t_alloc(CHUNK_SIZE, false);
chunk_allocator p_alloc(CHUNK_SIZE, true);

chunk_allocator::chunk_allocator(int chunk_size, bool down) {
  _curr_chunk = 0;
  _chunk_pos = 0;
  _nchunks = 0;

  _chunk = new char* [MAX_CHUNKS];
  if(_chunk == 0)
    throw tError("alloc: out of memory"); 

  _init_core(down, chunk_size);

  _chunk[_nchunks++] = (char *) _core_alloc(_chunk_size);
  if(_chunk[_curr_chunk] == 0)
    throw tError("alloc: out of memory"); 

  _stats_chunk_sum = _stats_chunk_n = 0;

  _max = 0;
}

chunk_allocator::~chunk_allocator() {
  for(int i = 0; i < _nchunks; i++)
    _core_free(_chunk[i], _chunk_size);

  delete[] _chunk;
}

#ifdef __borlandc__
#pragma argsused
#endif
void chunk_allocator::_overflow(int n) {
  if(n > _chunk_size)
    throw tError("alloc: chunk_size too small");

  if(++_curr_chunk >= MAX_CHUNKS)
    throw tError("alloc: out of chunks");
  
  if(_curr_chunk >= _nchunks) {
    _chunk[_nchunks++] = (char *) _core_alloc(_chunk_size);
    if(_chunk[_curr_chunk] == 0) { 
      _nchunks--;
      reset(); 
      throw tError("alloc: out of memory"); 
    }
  }
  _chunk_pos = 0;
}

void chunk_allocator::may_shrink() {
  _stats_chunk_n++;
  _stats_chunk_sum+=_curr_chunk;

  int avg_chunks = _stats_chunk_sum / _stats_chunk_n;

  // we want to shrink if _nchunks is way above average & current usage is
  // not above average

  if(_nchunks > avg_chunks * 2 && _curr_chunk <= avg_chunks + 1) {
#ifdef DEBUG
    fprintf(stderr, "shrink (avg: %d, used: %d, allocated: %d) -> ",
            avg_chunks, _curr_chunk, _nchunks);
#endif

    while(_nchunks > (_curr_chunk * 2 + 1) && _nchunks > avg_chunks) {
      _core_free(_chunk[--_nchunks], _chunk_size);
    }

#ifdef DEBUG
    fprintf(stderr, "%d\n", _nchunks);
#endif
  }
#ifdef DEBUG
  else {
    fprintf(stderr, "noshrink (avg: %d, used: %d, allocated: %d)\n",
            avg_chunks, _curr_chunk, _nchunks);
  }
#endif
}

void chunk_allocator::reset() {
  _stats_chunk_sum = _stats_chunk_n = 0;
  _curr_chunk = _chunk_pos = 0;
  _max = 0;
  may_shrink();
}

void chunk_allocator::print_check() {
  for (int i=0; i < _nchunks; i++) {
    printf("alloc'ed: [%x %x]\n", (ptr2uint_t) _chunk[i]
           , (ptr2uint_t) (_chunk[i] + _chunk_size));
  }
  for (int i=0; i < _nchunks; i++) {
    for(char *p = _chunk[i]; p < _chunk[i] + _chunk_size; p++) {
      char val = *p ; *p = val;
    }
  }
}

#ifdef HAVE_MMAP

//
// core memory allocation for UNIX using mmap
//

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/fcntl.h>

// we want two heaps, one extending from low to high adresses, the
// other from high to low adresses
// linux mmap returns low to high, linux malloc low to high;
// linux mmap respects an addr argument, even without MAP_FIXED
// solaris mmap return high to low, solaris malloc low to high;
// solaris mmap doesn't respect an addr argument without MAP_FIXED

// --> strategy:
// (linux):   use mmap(0) for low to high, mmap(addr) for high to low
// (solaris): use mmap(addr) for low to high, mmap(0) for high to low

// 2.6 series linux kernels have problems with mmap. Values proposed by Stephan
// Oepen in the developer's mailing list post at
// http://lists.delph-in.net/archive/developers/2005/000062.html seem to solve
// the problems of cheap and flop crashing with alloc errors and flop not being
// able to compile grammars under kernel 2.6. There is probably a better
// solution, but for now let's check the kernel version in configure.ac and set
// the mmap values appropriately ... 
// Eric Nichols <eric-n@is.naist.jp> -- Jun. 19, 2005

#if defined(linux)
#define _MMAP_ANONYMOUS
#ifdef KERNEL_2_6
#define _CORE_LOW  0x10000000
#define _CORE_HIGH 0xb6000000
//#define _MMAP_DOWN
#endif

#ifdef KERNEL_2_4
#define _CORE_LOW  0x50000000
#define _CORE_HIGH 0xb6000000
#endif
#else
// settings for Solaris. Not known to work for other systems
#define _CORE_LOW  0x30000000
#define _CORE_HIGH 0xd0000000
#define _MMAP_DOWN
#endif

#ifndef _MMAP_ANONYMOUS
static int dev_zero_fd = -1;
#endif

char *chunk_allocator::_mmap_up_mark = (char *) _CORE_LOW;
char *chunk_allocator::_mmap_down_mark = (char *) _CORE_HIGH;
size_t chunk_allocator::_pagesize = 0;

inline void *mmap_mmap(void *addr, int size) {
#ifndef _MMAP_ANONYMOUS
  return mmap((char *) addr, size
              , PROT_READ|PROT_WRITE, MAP_PRIVATE|(addr == 0 ? 0 : MAP_FIXED)
              , dev_zero_fd, 0);
#else
  return mmap(addr, size
              , PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS
              , -1, 0);
#endif
}

#ifdef DEBUG
void mmap_scan(int pagesize) {
  char *p_mem;
  bool success, last_success = false;
  for (p_mem = (char *)0x10000000; p_mem < (char *) 0xFFFF0000
         ; p_mem += pagesize) {
    char *new_mem = (char *) mmap_mmap(p_mem, pagesize);
    success = (new_mem == p_mem);
    if (success != last_success) {
      printf("%s in mapping %d bytes at %x\n"
             , (success ? "succeeded" : "failed")
             , pagesize, (ptr_to_uint_t) p_mem);
      last_success = success;
    }
    munmap(new_mem, pagesize);
  }
}
#endif

void chunk_allocator::_init_core(bool down, int chunk_size) {

  _core_down = down;
  
  if (_pagesize == 0) { 
#ifndef _MMAP_ANONYMOUS
    if(dev_zero_fd == -1) { dev_zero_fd = open("/dev/zero", O_RDWR); }
#endif

#ifdef _SC_PAGESIZE
    _pagesize = sysconf(_SC_PAGESIZE);     /* SVR4 */
#else
# ifdef _SC_PAGE_SIZE
    _pagesize = sysconf(_SC_PAGE_SIZE);    /* HPUX */
# else
    _pagesize = getpagesize();
# endif
#endif

  }
  
  _chunk_size = (((chunk_size-1)/_pagesize)+1)*_pagesize;
#ifdef DEBUG
  printf("Up: %x   Down: %x   Chunks: %x[%x]  %s\n"
         , (ptr_to_uint_t) _mmap_up_mark, (ptr_to_uint_t) _mmap_down_mark
         , _chunk_size, chunk_size
         , (_core_down ? "Down" : "Up")
         );

  printf("Scan run 1");
  mmap_scan(_pagesize);
#endif
}


void *chunk_allocator::_core_alloc(int size) {
  void *p = MAP_FAILED;

  while((p == MAP_FAILED) && (_mmap_up_mark < _mmap_down_mark)) {
#ifdef _MMAP_DOWN
    if(_core_down)
      p = mmap_mmap(0, size);
    else
      p = mmap_mmap(_mmap_up_mark, size);
#else
    if(!_core_down)
      p = mmap_mmap(_mmap_up_mark, size);
    else
      p = mmap_mmap(_mmap_down_mark - size, size);
#endif

    if (p == MAP_FAILED) { _mmap_down_mark -= size ; }
  }

  if(p == MAP_FAILED) {
    perror("chunk_allocator::_core_alloc");
    fprintf(stderr,
            "couldn't mmap %d bytes %s (up = %xd, down = %xd)\n",
            size,
            _core_down ? "down" : "up",
            (size_t) _mmap_up_mark, (size_t) _mmap_down_mark);
    throw tError("alloc: mmap problem");
  }

  if(_core_down)
    _mmap_down_mark = (char *) p;
  else
    _mmap_up_mark = (char *) p + size;

#ifdef DEBUG
  char *s = (char *)p;
  if(_core_down) {
    printf("New Block Down:   %x-%x\n"
           , (ptr_to_uint_t) s, (ptr_to_uint_t) s+size-1);
  }
  else {
    printf("New Block Up:     %x-%x\n"
           , (ptr_to_uint_t) s, (ptr_to_uint_t) s+size-1);
  }
  for(int i=0; i < size; i++) { *s = 0xFF; s++; }
  for(int i=0; i < size; i++) { s-- ; *s = 0x00; }
#endif

  if(_mmap_up_mark >= _mmap_down_mark) {
    fprintf(stderr, "alloc: no space (up = %xd, down = %xd)\n",
            (size_t) _mmap_up_mark, (size_t) _mmap_down_mark);
    throw tError("alloc: out of mmap space");
  }

  return p;
}

int chunk_allocator::_core_free(void *p, int size) {
  int res = munmap((char *) p, size);

  if(res == -1)
    throw tError("alloc: munmap error");

  if(_core_down)
    _mmap_down_mark += size;
  else
    _mmap_up_mark -= size;

  return res;
}

#elif defined(__BORLANDC__)
// Use WinAPI VirtualAlloc and VirtualFree

#include <windows.h>
#include "winbase.h"

#pragma argsused
int chunk_allocator::_init_core(bool down, int chunksize) {
  _chunk_size = chunksize;
}

void *chunk_allocator::_core_alloc(int size) {
  void *p = (void *)
    VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
  return p;
}

#pragma argsused
int chunk_allocator::_core_free(void *p, int size) {
  VirtualFree(p, 0, MEM_RELEASE);
  return 1;
}

#else

//
// generic core memory allocation for Windows et al
//

#ifndef __GNUG__
#pragma argsused
#endif
void chunk_allocator::_init_core(bool down, int chunksize) {
  _chunk_size = chunksize;
}

void *chunk_allocator::_core_alloc(int size) {
  void *p = (void *) new char[size];
  return p;
}

#ifndef __GNUG__
#pragma argsused
#endif
int chunk_allocator::_core_free(void *p, int size) {
  delete[] (char *) p;
  return 1;
}

#endif  // core memory allocation

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

/** \file chunk-alloc.h
 * Manage chunks of memory for efficient allocation of objects.
 */

#ifndef _CHUNK_ALLOC_H_
#define _CHUNK_ALLOC_H_

/** The maximum number of chunks to allocate */
#define MAX_CHUNKS 16000
/** The desired chunk size */
#define CHUNK_SIZE (1024 * 1024)

/** Class to support stack-like allocation/deallocation by keeping a pointer
 *  into a growing allocated heap of memory.
 */
typedef struct chunk_alloc_state
{
  int c;
  int p;
} chunk_alloc_state;

/** Class to manage chunks of memory for efficient allocation of objects

   This class has several implementations of its low-level memory allocation,
   one that is unix dependend, supporting two heaps with disjunct and
   easy to distinguish address rooms to allow zero-overhead seperation
   of permanent and non-permant dags (colored pointers); another windows-
   specific implementation that uses VirtualAlloc; and a third one that
   relies on standard C++ memory management only
   (somewhat like obstacks in gcc) */
class chunk_allocator
{
 public:
  /** Create a chunk allocator with (approximate) chunk size \a chunk_size.
   *  \param chunk_size The (approximate) size of one allocated chunk.
   *  \param down (only useful if mmap is used, i.e., on *nix platforms:
   *         if \c true, allocate chunks down from the upper end of the address
   *         space, otherwise, start at the lower end and grow upwards.
   */
  chunk_allocator(int chunk_size, bool down = false);
  ~chunk_allocator();

  /** Get another \a n bytes from the chunk allocator */
  inline void* allocate(int n)
    {
      if((_chunk_pos + n) > _chunk_size) _overflow(n);
      void *p = (void *) (_chunk[_curr_chunk] + _chunk_pos);
      _chunk_pos += n;
      return p;
    }

  /** The number of chunks currently allocated */
  inline int nchunks() { return _nchunks; }
  /** The size of every single chunk */
  inline int chunksize() { return _chunk_size; }

  /** The amount of memory currently allocated */
  inline long int allocated()
    { return _curr_chunk * _chunk_size + _chunk_pos; }

  /** Pointer to the next free memory address */
  inline void *current()
    { return (void *) (_chunk[_curr_chunk] + _chunk_pos); }
  /** Pointer to the base of the current chunk still containing free memory */
  inline void *current_base()
    { return (void *) (_chunk[_curr_chunk]); }

  /** @name State Functions
   *  These functions, together with class chunk_alloc_state, support a
   *  stack-like allocation/deallocation strategy for situations where a lot of
   *  memory allocated between to points in computation may be released in
   *  one step (no memory has to be preserved in between these points)
   */
  /*@{*/
  /** Mark the current state of the chunk allocator */
  inline void mark(chunk_alloc_state &s)
    { s.c = _curr_chunk; s.p = _chunk_pos; }
  /** Release all memory allocated since this alloc state was marked */
  inline void release(chunk_alloc_state &s)
    { 
      if(allocated() > _max) _max = allocated();
      _curr_chunk = s.c; _chunk_pos = s.p;
    }
  /*@}*/

  /** Call this function when there is a good opportunity to release
   *  dispensable memory.
   */
  void may_shrink();

  /** Release all memory and reset all statistics */
  void reset();

  /** Maximum size of allocated memory */
  inline long int max_usage()
    { return _max; }
  
  /** Reset maximum allocated size counter */
  inline void reset_max_usage()
    { _max = 0; }

  void print_check() ;

 private:
  int _chunk_pos;  /** number of bytes allocated in current chunk */
  int _chunk_size; /** size of each chunk */

  int _curr_chunk; /** index of chunk currently allocated from */
  int _nchunks;    /** nr of currently allocated chunks */

  char **_chunk;   /** vector of chunks of length MAX_CHUNKS */

  long int _max;   /** max nr of bytes allocated so far */


  void _overflow(int n);

  /** @name Statistics 
   * Collect statistics to enable shrinking
   */
  /*@{*/
  int _stats_chunk_sum;
  int _stats_chunk_n;
  /*@}*/

  /** @name Core Memory Allocation */
  /*@{*/
  void _init_core(bool down, int chunksize);
  void *_core_alloc(int size);
  int _core_free(void *p, int size);
  /*@}*/

  /** Does the chunks in this allocator grow up or down ? */
  bool _core_down;

#ifdef HAVE_MMAP
  /** Pointers to the upper and lower allocation boundary of mmap(2) based core
   *  allocator.
   */
  static char *_mmap_up_mark, *_mmap_down_mark;
  /** Size of a memory page allocated by mmap(2) */
  static size_t _pagesize;

  friend bool is_p_addr(void *p);
#endif
};

#define t_alloc _t_alloc

extern chunk_allocator t_alloc, p_alloc;

#if defined(HAVE_MMAP)
inline bool is_p_addr(void *p)
{
  return p >= chunk_allocator::_mmap_down_mark; 
}
#endif

#endif

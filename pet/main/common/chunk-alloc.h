/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class to manage chunks of memory for objects */
/* (somewhat like obstacks in gcc) */

#ifndef _CHUNK_ALLOC_H_
#define _CHUNK_ALLOC_H_

#define MAX_CHUNKS 16000
#define CHUNK_SIZE (1024 * 512)

typedef struct chunk_alloc_state
{
  int c;
  int p;
} chunk_alloc_state;

class chunk_allocator
{
 public: 
  chunk_allocator(int chunk_size, bool down = false);
  ~chunk_allocator();

  inline void* allocate(int n)
    {
      if((_chunk_pos + n) > _chunk_size) _overflow(n);
      void *p = (void *) (_chunk[_curr_chunk] + _chunk_pos);
      _chunk_pos += n;
      return p;
    }

  inline int nchunks() { return _nchunks; }
  inline int chunksize() { return _chunk_size; }

  inline long int allocated()
    { return _curr_chunk * _chunk_size + _chunk_pos; }

  inline void *current()
    { return (void *) (_chunk[_curr_chunk] + _chunk_pos); }
  inline void *current_base()
    { return (void *) (_chunk[_curr_chunk]); }
  inline void mark(chunk_alloc_state &s)
    { s.c = _curr_chunk; s.p = _chunk_pos; }
  inline void release(chunk_alloc_state &s)
    { if(allocated() > _max) _max = allocated(); _curr_chunk = s.c; _chunk_pos = s.p; }

  void may_shrink();
  void reset();

  inline long int max_usage()
    { return _max; }
  
  inline void reset_max_usage()
    { _max = 0; }

 private:
  int _chunk_pos;  // number of bytes allocated in current chunk
  int _chunk_size; // size of each chunk

  int _curr_chunk; // index of chunk currently allocated from
  int _nchunks;    // nr of currently allocated chunks

  char **_chunk;

  long int _max;   // max nr of bytes allocated so far

  void _overflow(int n);

  // keep statistics to enable shrinking
  int _stats_chunk_sum;
  int _stats_chunk_n;

  // core memory allocation

  void _init_core(bool down);
  void *_core_alloc(int size);
  int _core_free(void *p, int size);

  bool _core_down;
};

#define t_alloc _t_alloc

extern chunk_allocator t_alloc, p_alloc;

#if defined(USEMMAP)
extern char *_mmap_down_mark;
inline bool is_p_addr(void *p)
{
  return p >= _mmap_down_mark; 
}
#endif

#endif

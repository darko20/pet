/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* interface to the [incr tsdb()] system */

#ifndef _ITSDB_H_
#define _ITSDB_H_

#define MICROSECS_PER_SEC 1000000

#ifdef TSDBAPI
extern "C" {
#include "itsdb.h"
}

void capi_putstr(const char *, bool strctx = false);

void tsdb_mode();
void cheap_tsdb_summarize_run();
void cheap_tsdb_summarize_item(class chart &, class agenda *, int, int, int,
			       char * = NULL);
void cheap_tsdb_summarize_error(error &, int treal);
#endif

class statistics
{
 public:
  int id;                     /* item id */
  int readings;               /* nr of readings */
  int words;                  /* nr of words */
  int words_pruned;           /* nr of words pruned by chart manipulation */
  int first;                  /* time for first reading */
  int tcpu;                   /* total cpu time */
  int ftasks_fi;              /* filtered tasks (by rule filter) */
  int ftasks_qc;              /* filtered tasks (by quickcheck) */
  int etasks;                 /* executed tasks */
  int stasks;                 /* suceeding tasks */
  int aedges;                 /* active items in chart */
  int pedges;                 /* passive items in chart */
  int raedges;                /* active items contributing to result */
  int rpedges;                /* passive items contributing to result */
  int medges;                 /* inflr items */
  int unifications_succ;      /* nr of successfull unifications */
  int unifications_fail;      /* nr of failed unifications */
  int copies;                 /* nr of copies */
  long dyn_bytes;             /* total dynamic memory in bytes */
  long stat_bytes;            /* total static memory in bytes */
  int cycles;                 /* cycles found */
  int fssize;                 /* avg size of all passive edges */
  int nmeanings;              /* number of well-formed semantic formulae; typically <= readings */
  int unify_cost_succ;
  int unify_cost_fail;

  void reset();
  void print(FILE *f);
};

extern statistics stats;

void initialize_version();

// this class provides a clock that starts running on construction
// elapsed() returns elapsed time since start in some unknown unit
// provides functions to convert this to milliseconds, and to get resolution
// timer can be stopped and restarted
// this implementation will fail when clock() wraps over, which happens after
// about 36 minutes on solaris/linux on 32 bit machines
// a timer with lower resolution (1 s) is also maintained, it will not
// wrap over as quickly

class timer
{
 public:
  inline timer(bool running = true) : _start(0), _elapsed(0), _saved(0),
    _elapsed_ts(0), _saved_ts(0), _running(false)
    { if(running) start(); }

  inline ~timer() {};

  bool running() { return _running; }

  void start()
    { if(!_running) { _running = true; _start = clock(); } }

  void stop()
    { if(_running) { _running = false; _elapsed += clock() - _start; _elapsed_ts += convert2ms(clock() - _start) / 100; } }

  void save() { _saved = _elapsed; _saved_ts = _elapsed_ts; }
  void restore() { _elapsed = _saved; _elapsed_ts = _saved_ts; }

  inline clock_t elapsed() { return _elapsed + (_running ? clock() - _start : 0); }
  // returns elapsed time since start in some unknown unit

  inline int elapsed_ts() { return _elapsed_ts; }
  // returns elapsed time since start in tenth of seconds
  
  inline clock_t convert2ms(clock_t t) { return t / (CLOCKS_PER_SEC / 1000); }
  // converts time in internal unit to milliseconds

  inline clock_t resolution() { return 1000 / CLK_TCK; }
  // returns `stepsize' of the clock in milliseconds
 private:
  clock_t _start;
  clock_t _elapsed;
  clock_t _saved;
  
  unsigned int _elapsed_ts;
  unsigned int _saved_ts;

  bool _running;
};

#endif


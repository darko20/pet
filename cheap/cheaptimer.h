/* -*- Mode: C++ -*- */

#ifndef _TIMER_H
#define _TIMER_H

#include <time.h>
#define MICROSECS_PER_SEC 1000000

/** this class provides a clock that starts running on construction
 *  elapsed() returns elapsed time since start in some unknown unit
 *  provides functions to convert this to milliseconds, and to get resolution
 *  timer can be stopped and restarted
 *  this implementation will fail when clock() wraps over, which happens after
 *  about 36 minutes on solaris/linux on 32 bit machines
 */
class timer
{
 public:
  inline timer(bool running = true) : _start(0), _elapsed(0), _saved(0),
      _running(false)
    { if(running) start(); }

  inline ~timer() {};

  bool running() { return _running; }

  void reset() {
    _running = false; _start = 0; _elapsed = 0; _saved = 0;
  }

  void start()
    { if(!_running) { _running = true; _start = clock(); } }

  void stop()
    { if(_running) { _running = false; _elapsed += clock() - _start; } }

  void save() { _saved = _elapsed; }
  void restore() { _elapsed = _saved; }

  /** returns elapsed time since start in some unknown unit */
  inline long long
  elapsed() { return _elapsed + (_running ? clock() - _start : 0); }

  /** returns elapsed time since start in tenth of seconds */
  inline int elapsed_ts() { return convert2ms(elapsed()) / 100; }

  
  /** converts time in internal unit to milliseconds */
  inline long long
  convert2ms(long long t) { return t * 1000 / CLOCKS_PER_SEC ; }

  /** returns `stepsize' of the clock in milliseconds */
  inline clock_t resolution() { return 1000 / CLOCKS_PER_SEC; }
 private:
  clock_t _start;

  long long _elapsed;
  long long _saved;
  
  bool _running;
};


#endif

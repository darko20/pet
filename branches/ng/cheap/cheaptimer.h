/* -*- Mode: C++ -*- */

#ifndef _TIMER_H
#define _TIMER_H

#include <time.h>
#include "errors.h"

/** this class provides a clock that starts running on construction, unless
 *  the constructor is passed \c false as argument.
 *  elapsed() returns elapsed time since start in nanoseconds
 *  provides functions to convert this to milliseconds
 */
class timer {

private:

  struct timespec _start;

  long long _elapsed;
  long long _saved;

  bool _running;

  long long delta() {
    struct timespec stop;
    int success = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &stop);
    if (success != 0)
      throw new tError("CLOCK_THREAD_CPUTIME_ID not accessible");
    long long result = 1000000000LL * (stop.tv_sec - _start.tv_sec)
      + (stop.tv_nsec - _start.tv_nsec);
    return result;
  }

public:
  inline timer(bool running = true) : _start(), _elapsed(0), _saved(0),
      _running(false)
    { if(running) start(); }

  inline ~timer() {};

  bool running() { return _running; }

  void reset() {
    _running = false; _elapsed = 0; _saved = 0;
  }

  void start() {
    if(!_running) {
      _running = true;
      int success = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &_start);
      if (success != 0)
        throw new tError("CLOCK_THREAD_CPUTIME_ID not accessible");
    }
  }

  void stop() {
    if(_running) {
      _running = false;
      _elapsed += delta();
    }
  }

  void save() { _saved = _elapsed; }
  void restore() { _elapsed = _saved; }

  /** returns elapsed time since start in some unknown unit */
  inline long long elapsed() { return _elapsed + (_running ? delta() : 0); }

  /** returns elapsed time since start in tenth of seconds */
  inline int elapsed_ms() { return convert2ms(elapsed()); }

  /** converts time in internal unit to milliseconds */
  inline float convert2ms(long long t) { return t / 1000000 ; }
};


#endif

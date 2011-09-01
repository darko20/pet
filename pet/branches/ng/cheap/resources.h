
/* -*- Mode: C++ -*- */

#ifndef _RESOURCES_H
#define _RESOURCES_H

#include "chart.h"
#include "cheaptimer.h"
#include "configs.h"
#include "tsdb++.h"

#include <sys/times.h>
#include <sstream>

class Resources {

private:

  static const int STAGES = 4;

  /** Names for the different stages of processing, all of which get a certain
   *  amount of the globally restricted resources.
   *
   *  TODO: make names and percentages configurable from the outside
   */
  static const std::string stage_name[] ;

  float _stage_percentage[STAGES];

  float _current_percentage;
  int _current_stage;

  /** Global and per-stage timer, do not start at creation time */
  timer _local_timer;
  timer _global_timer;

  long long _sum_timeused;
  long _sum_memused;
  long _sum_edges;

  long long _last_timeused;
  long _last_memused;
  long _last_edges;

  long long _global_timelimit;
  long _global_memlimit;
  int _global_edgelimit;

  long long _local_timelimit;
  long _local_memlimit;
  int _local_edgelimit;

private:

  long getmem() { return t_alloc.max_usage(); }

  /** local helper for exhaustion_message */
  void print_prefix(std::ostream &s, std::string prefix,
                               std::string what);

  /** fill the local limits with appropriate values.
   *
   *  If local percentage is < 0, take the next percentage value that is > 0
   *  as local percentage.
   *  Then, the local limit is:
   *     global_limit * local_percentage - sum_of_resource_used
   */
  void compute_local_bounds();

  /** Compute the resources that were used during the last stage and put that
   *  into the _last fields.
   */
  void compute_last_used();

  void debug_print(bool endl) {
    printf("%s %f %lld %d %ld (%lld %d %ld)",
           stage_name[_current_stage].c_str(), _current_percentage,
           _local_timer.elapsed(), pedges, getmem(),
           _local_timelimit, _local_edgelimit, _local_memlimit);
    if (endl) printf("\n");
  }

public:
  /** A counter for passive edges */
  int pedges;

  /** Create a new Resources object. */
  Resources();

  inline bool exhausted() {
    // debug_print(false);
    bool local =
      ((_local_edgelimit != 0 && pedges > _local_edgelimit)
       || (_local_memlimit != 0 && getmem() > _local_memlimit)
       || (_local_timelimit != 0 && _local_timer.elapsed() > _local_timelimit));
    // printf(" %s\n", (local ? "ex" : "no"));
    return local;
  }

  /** Supposed to be called at the very beginning of a run */
  void stop_run();

  /** Supposed to be called at the very end of a run */
  void stop_run();

  long get_stage_time_ms() {
    return _local_timer.elapsed_ms();
  }

  /** may only be called when the resources have been exhausted. Returns an
   * explanation of the concrete resource failure
   */
  std::string exhaustion_message();

  /** Call this method when a stage starts: use this ONLY in exceptional
   *  cases, normally, start_next_stage should be called
   */
  void start_stage(int stage);

  /** Call this method to end a stage.
   *  Stops the local (per-stage) timer and computes the resources used by the
   *  stage as well as the sub-totals (in the _sum fields).
   */
  void end_stage();

  /** Call this method when the next stage starts */
  void start_next_stage() {
    start_stage(_current_stage + 1);
  }
};


#endif

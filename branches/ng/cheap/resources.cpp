#include "resources.h"

const std::string Resources::stage_name[] = {
  "preprocessing", "tree creation", "unpacking", "robust"
};

#define min(a, b) ((a > b) ? b : a)
#define max(a, b) ((a > b) ? a : b)

/** fill the local limits with appropriate values.
 *
 *  If local percentage is < 0, take the next percentage value that is > 0
 *  as local percentage.
 *  Then, the (relative) local limit is:
 *     min(global_limit * local_percentage,
 *         global_limit - sum_of_resource_used)
 *  For values where there is only access to the totals (memory, edges), the
 *  current value has to be added.
 */
void Resources::compute_local_bounds() {
  _local_memlimit = 0;
  _local_edgelimit = 0;
  _local_timelimit = 0;

  int stage = _current_stage;
  _current_percentage = -1.0;
  do {
    _current_percentage = _stage_percentage[stage];
    ++ stage;
  } while (_current_percentage < 0 && stage < STAGES);
  // beyond the last stage: this stage is given the rest of the resources.
  if (_current_percentage < 0) {
    _local_memlimit = _global_memlimit;
    _local_edgelimit = _global_edgelimit;
    _local_timelimit = (_global_timelimit > 0
                        ? _global_timelimit - _global_timer.elapsed()
                        : 0);
  } else {
    // add the memory used up to now because we get only total usage
    if (_global_memlimit > 0) {
      _local_memlimit = min(_global_memlimit * _current_percentage,
                            _global_memlimit - _sum_memused)
        + getmem();
    }
    // add edges consumed up to now because we use total nr. of edges
    if (_global_edgelimit > 0) {
      _local_edgelimit =
        min(_global_edgelimit * _current_percentage,
            _global_edgelimit - _sum_edges)
        + pedges;
    }
    if (_global_timelimit > 0) {
      _local_timelimit = min(_global_timelimit * _current_percentage,
                             _global_timelimit - _sum_timeused);
    }
  }
}

void Resources::compute_last_used() {
  _last_edges = pedges - _sum_edges;
  _last_memused = getmem() - _sum_memused;
  _last_timeused = _local_timer.elapsed();
}


/** Create a new Resources object
 */
Resources::Resources() : _current_stage(0),
                         _local_timer(false), _global_timer(false),
                         _sum_timeused(0), _sum_memused(0), _sum_edges(0),
                         _global_timelimit(1000000000LL
                                           * get_opt_int("opt_timeout")),
                         _global_memlimit(get_opt_int("opt_memlimit")
                                          * 1024 * 1024),
                         _global_edgelimit(get_opt_int("opt_pedgelimit")),
                         pedges(0) {

  // printf(">>> %lld %d\n", _global_timelimit, get_opt_int("opt_timeout"));

  _stage_percentage[0] = -1;  // this takes the percentage of the next stage
  _stage_percentage[1] = (get_opt_int("opt_packing") == 0) ? 1 : 0.9;
  _stage_percentage[2] = -1;  // this takes the remaining resources
  _stage_percentage[3] = -1;  // this takes the remaining resources

}

/** Supposed to be called at the very beginning of a run. */
void Resources::start_run() {
  _global_timer.start();
  start_stage(_current_stage);
}

/** Supposed to be called at the very end of a run */
void Resources::stop_run() {
  end_stage();
  _global_timer.stop();
  // printf("%f", _global_timer.elapsed_ms() / 1000.);
}

void Resources::print_prefix(std::ostream &s, std::string prefix,
                             std::string what) {
  s << prefix << " " << what;
  if (_current_percentage > 0)
    s << (int) (100. * _current_percentage) << "% of ";
}

/** may only be called when the resources have been exhausted. Returns an
 * explanation of the concrete resource failure
 */
std::string Resources::exhaustion_message() {
  // TODO: We'll see what and when we want to report. Maybe only the global
  // failure is of interest
  compute_last_used();
  const std::string &prefix =
    (_current_stage < STAGES) ? stage_name[_current_stage] : "FINAL";
  std::ostringstream s;
  if (_local_memlimit != 0 && getmem() >= _local_memlimit) {
    print_prefix(s, prefix, "memory limit exhausted: ");
    s << _global_memlimit / (1024 * 1024) << " MB";
  }
  else if (_local_edgelimit != 0 && pedges >= _local_edgelimit) {
    print_prefix(s, prefix, "edge limit exhausted: ");
    if (_current_percentage > 0)
      s << (int) (100. * _current_percentage) << "% of ";
    s << _global_edgelimit << " pedges";
  }
  else {
    print_prefix(s, prefix, "timed out: ");
    if (_current_percentage > 0)
      s << (int) (100. * _current_percentage) << "% of ";
    s << get_opt_int("opt_timeout") << " s";
  }
  return s.str();
}

/** Call this method when a stage starts: use this ONLY in exceptional
 *  cases, normally, start_next_stage should be called
 */
void Resources::start_stage(int stage) {
  end_stage();
  _current_stage = stage;
  compute_local_bounds();
  // debug_print(true);
  _local_timer.reset();
  _local_timer.start();
}

/** Call this method to end a stage. */
void Resources::end_stage() {
  _local_timer.stop();
  compute_last_used();    // also computes _last_timeused
  _sum_edges = pedges;
  _sum_memused = getmem();
  _sum_timeused += _last_timeused;
}

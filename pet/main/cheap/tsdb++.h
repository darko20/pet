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

/* interface to the [incr tsdb()] system */

#ifndef _ITSDB_H_
#define _ITSDB_H_

#define MICROSECS_PER_SEC 1000000

#ifdef TSDBAPI
extern "C" {
#include "itsdb.h"
}

void tsdb_mode();
void cheap_tsdb_summarize_run();
#endif

void cheap_tsdb_summarize_item(class chart &, int, int, int,
                               const char *, class tsdb_parse &);
void cheap_tsdb_summarize_error(error &, int treal, class tsdb_parse &);

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

// Representation of tsdb++ relations

class tsdb_result
{
 public:
  tsdb_result() :
    parse_id(-1), result_id(-1), time(-1), r_ctasks(-1), r_ftasks(-1),
    r_etasks(-1), r_stasks(-1), size(-1), r_aedges(-1), r_pedges(-1),
    derivation(), tree(), mrs()
    {
    }    

  void file_print(FILE *f);

#ifdef TSDBAPI
  void capi_print();
#endif

  int parse_id;
  int result_id;                    // unique result identifier
  int time;                         // time to find this result (msec)
  int r_ctasks;                     // parser contemplated tasks
  int r_ftasks;                     // parser filtered tasks
  int r_etasks;                     // parser executed tasks
  int r_stasks;                     // parser succeeding tasks
  int size;                         // size of feature structure
  int r_aedges;                     // active items for this result
  int r_pedges;                     // passive items in this result
  string derivation;                // derivation tree for this reading
  string tree;                      // phrase structure tree (CSLI labels)
  string mrs;                       // mrs for this reading
};

class tsdb_rule_stat
{
 public:
  tsdb_rule_stat()
    : rule(), actives(-1), passives(-1)
    {
    }

#ifdef TSDBAPI
  void capi_print();
#endif  

  string rule;
  int actives;
  int passives;
};

class tsdb_parse
{
 public:
  tsdb_parse() :
    parse_id(-1), run_id(-1), i_id(-1), readings(-1), first(-1), total(-1), tcpu(-1), tgc(-1), treal(-1), words(-1), l_stasks(-1), p_ctasks(-1), p_ftasks(-1), p_etasks(-1), p_stasks(-1), aedges(-1), pedges(-1), raedges(-1), rpedges(-1), unifications(-1), copies(-1), conses(-1), symbols(-1), others(-1), gcs(-1), i_load(-1), a_load(-1), date(), err(), nk2ys(-1), nmeanings(-1), k2ystatus(-1), failures(-1), pruned(-1), results(), rule_stats(), i_input(), i_length(-1)
    {
    }

  void push_result(class tsdb_result &r)
    {
      results.push_back(r);
    }

  void set_rt(const string &rt);

  void push_rule_stat(class tsdb_rule_stat &r)
    {
      rule_stats.push_back(r);
    }

  void set_input(const string &s)
    {
      i_input = s;
    }

  void set_i_length(int l)
    {
      i_length = l;
    }

  void file_print(FILE *f_parse, FILE *f_result, FILE *f_item);

#ifdef TSDBAPI
  void capi_print();
#endif

  int parse_id;                     // unique parse identifier
  int run_id;                       // test run for this parse
  int i_id;                         // item parsed
  int readings;                     // number of readings obtained
  int first;                        // time to find first reading (msec)
  int total;                        // total time for parsing (msec)
  int tcpu;                         // total (cpu) processing time (msec)
  int tgc;                          // gc time used (msec)
  int treal;                        // overall real time (msec)
  int words;                        // lexical entries retrieved
  int l_stasks;                     // successful lexical rule applications
  int p_ctasks;                     // parser contemplated tasks (LKB)
  int p_ftasks;                     // parser filtered tasks
  int p_etasks;                     // parser executed tasks
  int p_stasks;                     // parser succeeding tasks
  int aedges;                       // active items in chart (PAGE)
  int pedges;                       // passive items in chart
  int raedges;                      // active items contributing to result
  int rpedges;                      // passive items contributing to result
  int unifications;                 // number of (node) unifications
  int copies;                       // number of (node) copy operations
  int conses;                       // cons() cells allocated
  int symbols;                      // symbols allocated
  long int others;                  // bytes of memory allocated
  int gcs;                          // number of garbage collections
  int i_load;                       // initial load (start of parse)
  int a_load;                       // average load
  string date;                      // date and time of parse
  string err;                       // error string (if applicable |:-)
  int nk2ys;
  int nmeanings;
  int k2ystatus;
  int failures;
  int pruned;

 private:

  list<tsdb_result> results;
  list<tsdb_rule_stat> rule_stats;
  string i_input;
  int i_length; 
};

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

  inline clock_t resolution() { return 1000 / CLOCKS_PER_SEC; }
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

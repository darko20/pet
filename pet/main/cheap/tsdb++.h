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

/** \file tsdb++.h
 * Interface to the [incr tsdb()] system.
 */

#ifndef _ITSDB_H_
#define _ITSDB_H_

#include "errors.h"

#define MICROSECS_PER_SEC 1000000

#ifdef TSDBAPI
extern "C" {
#include "itsdb.h"
}

/** Attach to a (hopefully) running pvm server and initialize [incr tsdb()]
 *  functionality.
 */
void tsdb_mode();
/** Deliver the tsdb run statistics to the server */
void cheap_tsdb_summarize_run();
#endif

/** Create a summary for a successful parse.
 * \param Chart the chart of the last parse
 * \param length the length of the chart 
 * \param treal the real time used for the parse
 * \param nderivations i've got no clue
 * \param tp the data structure where the data is collected (the result) 
 */
void cheap_tsdb_summarize_item(class chart &Chart, int length, int treal,
                               int nderivations, class tsdb_parse &tp);

/** Create a summary for an unsuccessful parse.
 * \param treal the real time used for the parse
 * \param tp the data structure where the data is collected (the result) 
 */
void cheap_tsdb_summarize_error(list<tError> &, int treal,
                                class tsdb_parse &tp);

/** Statistics for one parse */
class statistics
{
 public:
  /** item id */
  int id;
  /** nr of trees (packed readings) */
  int trees;
  /** nr of readings */
  int readings;
  /** nr of words */
  int words;
  /** nr of words pruned by chart manipulation */
  int words_pruned;
  /** time for first reading */
  int first;
  /** total cpu time */
  int tcpu;
  /** filtered tasks (by rule filter) */
  int ftasks_fi;
  /** filtered tasks (by quickcheck) */
  int ftasks_qc;
  /** filtered subsumptions (by rule filter) */
  int fsubs_fi;
  /** filtered subsumptions (by quickcheck) */
  int fsubs_qc;
  /** executed tasks */
  int etasks;
  /** suceeding tasks */
  int stasks;
  /** active items in chart */
  int aedges;
  /** passive items in chart */
  int pedges;
  /** active items contributing to result */
  int raedges;
  /** passive items contributing to result */
  int rpedges;
  /** inflr items */
  int medges;
  /** nr of successfull unifications */
  int unifications_succ;
  /** nr of failed unifications */
  int unifications_fail;
  /** nr of successfull subsumptions */
  int subsumptions_succ;
  /** nr of failed subsumptions */
  int subsumptions_fail;
  /** nr of copies */
  int copies;
  /** total dynamic memory in bytes */
  long dyn_bytes;
  /** total static memory in bytes */
  long stat_bytes;
  /** cycles found */
  int cycles;
  /** avg size of all passive edges */
  int fssize;
  /** number of well-formed semantic formulae; typically <= readings */
  int nmeanings;
  /** costs for all successful unifications */
  int unify_cost_succ;
  /** costs for all failing unifications */
  int unify_cost_fail;

  /** @name Slots For Packing */
  /*@{*/
  /** equivalent edges */
  int p_equivalent;
  /** proactive packed edges: */
  int p_proactive;
  /** retroactive packed edges: */
  int p_retroactive;
  /** frozen edges: */
  int p_frozen;
  /** cpu time for unpacking */
  int p_utcpu;
  /** passive items constructed in unpacking */
  int p_upedges;
  /** failures in unpacking */
  int p_failures;
  /** total dynamic memory in bytes in unpacking */
  long p_dyn_bytes;
  /** total static memory in bytes in unpacking */
  long p_stat_bytes;
  /*@}*/

  void reset();
  void print(FILE *f);
};

extern statistics stats;

extern char CHEAP_VERSION[];
void initialize_version();

// Representation of tsdb++ relations

/** tsdb statistics for one parse result */
class tsdb_result
{
 public:
  tsdb_result() :
    parse_id(-1), result_id(-1), time(-1), r_ctasks(-1), r_ftasks(-1),
    r_etasks(-1), r_stasks(-1), size(-1), r_aedges(-1), r_pedges(-1),
    derivation(), edge_id(-1), tree(), mrs(), scored(false)
    {
    }    

  void file_print(FILE *f);

#ifdef TSDBAPI
  void capi_print();
#endif

  int parse_id;
  /** unique result identifier */
  int result_id;
  /** time to find this result (msec) */
  int time;
  /** parser contemplated tasks */
  int r_ctasks;
  /** parser filtered tasks */
  int r_ftasks;
  /** parser executed tasks */
  int r_etasks;
  /** parser succeeding tasks */
  int r_stasks;
  /** size of feature structure */
  int size;
  /** active items for this result */
  int r_aedges;
  /** passive items in this result */
  int r_pedges;

  /** derivation tree for this reading (v1) */
  string derivation;

  /** edge id for derivation (v2) */
  int edge_id;

  /** phrase structure tree (CSLI labels) */
  string tree;
  /** mrs for this reading */
  string mrs;
  /** has a score been assigned? */
  bool scored;
  /** score assigned by stochastic model */
  double score;
};

/** The tsdb representation of a chart edge */
class tsdb_edge
{
 public:
    tsdb_edge()
        : id(-1), label(), score(-1.0), start(-1), end(-1), status(0), 
        daughters()
    {
    }
#ifdef TSDBAPI
    void capi_print();
#endif  

    int id;
    string label;
    double score;
    int start, end;
    int status;
    string daughters;
};

/** Statistics for a single rule */
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

/** tsdb statistics for one parse */
class tsdb_parse
{
 public:
  tsdb_parse()
    : parse_id(-1), run_id(-1), i_id(-1), trees(-1), readings(-1),
    first(-1), total(-1), tcpu(-1), tgc(-1), treal(-1), words(-1),
    l_stasks(-1), p_ctasks(-1), p_ftasks(-1), p_etasks(-1), p_stasks(-1),
    aedges(-1), pedges(-1), raedges(-1), rpedges(-1),
    unifications(-1), copies(-1), conses(-1), symbols(-1), others(-1),
    gcs(-1), i_load(-1), a_load(-1),
    date(), err(), nmeanings(-1), clashes(-1), pruned(-1), 
    subsumptions(-1), p_equivalent(-1), p_proactive(-1),
    p_retroactive(-1), p_frozen(-1), p_utcpu(-1), p_failures(-1),
    p_upedges(-1),
    results(), edges(), rule_stats(), i_input(), i_length(-1)
    {
    }

  void push_result(class tsdb_result &r)
    {
      results.push_back(r);
    }

  void push_edge(class tsdb_edge &e)
    {
      edges.push_back(e);
    }

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

  /** unique parse identifier */
  int parse_id;
  /** test run for this parse */
  int run_id;
  /** item parsed */
  int i_id;
  /** number of trees (packed readings) */
  int trees;
  /** number of readings obtained */
  int readings;
  /** time to find first reading (msec) */
  int first;
  /** total time for parsing (msec) */
  int total;
  /** total (cpu) processing time (msec) */
  int tcpu;
  /** gc time used (msec) */
  int tgc;
  /** overall real time (msec) */
  int treal;
  /** lexical entries retrieved */
  int words;
  /** successful lexical rule applications */
  int l_stasks;
  /** parser contemplated tasks (LKB) */
  int p_ctasks;
  /** parser filtered tasks */
  int p_ftasks;
  /** parser executed tasks */
  int p_etasks;
  /** parser succeeding tasks */
  int p_stasks;
  /** active items in chart (PAGE) */
  int aedges;
  /** passive items in chart */
  int pedges;
  /** active items contributing to result */
  int raedges;
  /** passive items contributing to result */
  int rpedges;
  /** number of (node) unifications */
  int unifications;
  /** number of (node) copy operations */
  int copies;
  /** cons() cells allocated */
  int conses;
  /** symbols allocated */
  int symbols;
  /** bytes of memory allocated */
  long int others;
  /** number of garbage collections */
  int gcs;
  /** initial load (start of parse) */
  int i_load;
  /** average load */
  int a_load;
  /** date and time of parse */
  string date;
  /** error string (if applicable |:-) */
  string err;
  int nmeanings;
  /** number of failed unifications */
  int clashes;
  /** number of pruned lexical entries */
  int pruned;
    
  int subsumptions;
  int p_equivalent;
  int p_proactive;
  int p_retroactive;
  int p_frozen;
  int p_utcpu;
  int p_failures;
  /** passive items in unpacking  */
  int p_upedges;
    
 private:
    
  list<tsdb_result> results;
  list<tsdb_edge> edges;
  list<tsdb_rule_stat> rule_stats;
  string i_input;
  int i_length; 
};

/** A class to dump incr[tsdb] data directly to database files without
 *  connection to the server.
 */
class tTsdbDump {
public:
  /** Create the tsdb dump database in \a directory.
   * If \a directory is the empty string, this dumper remains inactive.
   */
  tTsdbDump(string directory);

  ~tTsdbDump();

  /** Is this dumper active? */
  bool active() { return _item_file != NULL; }

  /** Call this method at the start of a parse */
  void start(string input);

  /** Call this method at the end of a successful parse */
  void finish(class chart *Chart);

  /** Call this method at the end of a parse that produced an error */
  void error(class chart *Chart, const class tError &e);

private:
  void dump_current();

  FILE *_parse_file, *_result_file , *_item_file;
  tsdb_parse *_current;
};


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
  convert2ms(long long t) { return t / (CLOCKS_PER_SEC / 1000); }

  /** returns `stepsize' of the clock in milliseconds */
  inline clock_t resolution() { return 1000 / CLOCKS_PER_SEC; }
 private:
  clock_t _start;

  long long _elapsed;
  long long _saved;
  
  bool _running;
};

#endif

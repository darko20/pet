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

/* parser control */

#include "parse.h"
#include "cheap.h"
#include "pcfg.h"
#include "fs.h"
#include "item.h"
#include "item-printer.h"
#include "grammar.h"
#include "chart.h"
#include "lexparser.h"
#include "task.h"
#include "tsdb++.h"
#include "configs.h"
#include "settings.h"
#include "logging.h"

#include <sstream>
#include <iostream>
#include <sys/times.h>
#include <unistd.h>

using namespace std;

#ifdef YY
#include "yy.h"
#endif

// output for excessive subsumption failure debugging in the German grammar
//#define PETDEBUG_SUBSFAILS

//
// global variables for parsing
//

chart *Chart = NULL;
tAbstractAgenda *Agenda = NULL;

timer ParseTime;
timer TotalParseTime(false);

static bool parser_init();
//options managed by configuration subsystem
bool opt_hyper = parser_init();
int  opt_nsolutions, opt_packing;

#ifdef YY
int opt_nth_meaning;
#endif

/** Initialize global variables and options for the parser */
static bool parser_init() {
  reference_opt("opt_hyper",
     "use hyperactive parsing (see Oepen & Carroll 2000b)", opt_hyper);
  opt_hyper = true;
  reference_opt("opt_nsolutions",
                "The number of solutions until the parser is"
                "stopped, if not in packing mode", opt_nsolutions);
  opt_nsolutions = 0;
  reference_opt("opt_packing",
                "choose ambiguity packing mode. (see Oepen & Carroll 2000a,b) "
                "a bit vector of flags: 1:equivalence "
                "2:proactive 4:retroactive packing; "
                "8:selective 128:no unpacking", opt_packing);
  opt_packing = 0;
  managed_opt("opt_pedgelimit", "maximum number of passive edges",
              (int) 0);
  managed_opt("opt_memlimit", "memory limit (in MB) for parsing and unpacking",
              (int) 0);
  managed_opt("opt_timeout", "timeout limit (in s) for parsing and unpacking",
              (int) 0);
  managed_opt("opt_shrink_mem", "allow process to shrink after huge items",
              true);
  return opt_hyper;
}



/**
 * @name timeout control for parsing
 * timer class is not suitable because:
 *  i) we do not need microsecond-level accuracy
 *  ii) timer based on CLOCK(3) can wrap around after 36 minutes
 * therefore it is more suitable to use TIMES(2)
 */
//@{
clock_t timeout;
clock_t timestamp;
//@}

//
// filtering
//

bool
filter_rule_task(grammar_rule *R, tItem *passive)
{

#ifdef PETDEBUG
    LOG(logParse, DEBUG, "trying " << R << " & passive " << passive << " ==> ");
#endif

    if(!Grammar->filter_compatible(R, R->nextarg(), passive->rule()))
    {
        stats.ftasks_fi++;

#ifdef PETDEBUG
        LOG(logParse, DEBUG, "filtered (rf)");
#endif

        return false;
    }

    if(!fs::qc_compatible_unif(R->qc_vector_unif(R->nextarg()),
                               passive->qc_vector_unif()))
    {
        stats.ftasks_qc++;

#ifdef PETDEBUG
        LOG(logParse, DEBUG, "filtered (qc)");
#endif

        return false;
    }

#ifdef PETDEBUG
    LOG(logParse, DEBUG, "passed filters");
#endif

    return true;
}

bool
filter_combine_task(tItem *active, tItem *passive)
{
#ifdef PETDEBUG
    LOG(logParse, DEBUG, "trying active " << *active
        << " & passive " << *passive << " ==> ");
#endif

    if(!Grammar->filter_compatible(active->rule(), active->nextarg(),
                                   passive->rule()))
    {
#ifdef PETDEBUG
        LOG(logParse, DEBUG, "filtered (rf)");
#endif

        stats.ftasks_fi++;
        return false;
    }

    if(!fs::qc_compatible_unif(active->qc_vector_unif(),
                               passive->qc_vector_unif()))
    {
#ifdef PETDEBUG
        LOG(logParse, DEBUG, "filtered (qc)");
#endif

        stats.ftasks_qc++;
        return false;
    }

#ifdef PETDEBUG
    LOG(logParse, DEBUG, "passed filters");
#endif

    return true;
}

//
// parser control
//

/** Add all tasks to the agenda that try to combine the specified (passive)
 *  item with a suitable rule.
 */
void
postulate(tItem *passive) {
  assert(!passive->blocked());
  // iterate over all the rules in the grammar
  for(ruleiter rule = Grammar->rules().begin(); rule != Grammar->rules().end();
      ++rule) {
    grammar_rule *R = *rule;

    if(passive->compatible(R, Chart->rightmost()))
      if(filter_rule_task(R, passive))
        Agenda->push(new rule_and_passive_task(Chart, Agenda, R, passive));
  }
}

void
fundamental_for_passive(tItem *passive)
{
    // iterate over all active items adjacent to passive and try combination
    for(chart_iter_adj_active it(Chart, passive); it.valid(); ++it)
    {
        tItem *active = it.current();
        if(active->adjacent(passive))
            if(passive->compatible(active, Chart->rightmost()))
                if(filter_combine_task(active, passive))
                    Agenda->push(new active_and_passive_task(Chart, Agenda,
                                                             active, passive));
    }
}

void
fundamental_for_active(tPhrasalItem *active) {
  // iterate over all passive items adjacent to active and try combination

  for(chart_iter_adj_passive it(Chart, active); it.valid(); ++it)
    if(!it.current()->blocked())
      if(it.current()->compatible(active, Chart->rightmost()))
        if(filter_combine_task(active, it.current()))
          Agenda->push(new
                       active_and_passive_task(Chart, Agenda,
                                               active, it.current()));
}

bool
packed_edge(tItem *newitem) {
  if(! newitem->inflrs_complete_p()) return false;

  for(chart_iter_span_passive iter(Chart, newitem->start(), newitem->end());
      iter.valid(); ++iter) {
    bool forward, backward;
    tItem *olditem = iter.current();

    if(!olditem->inflrs_complete_p() || (olditem->trait() == INPUT_TRAIT))
      continue;

    // YZ 2007-07-25: avoid packing item with its offspring edges
    // (both forward and backward)
    if (newitem->contains_p(olditem))
      continue;

    // sets forward and backward correctly in every case
    Grammar->subsumption_filter_compatible(olditem->rule(), newitem->rule(),
                                           forward, backward);

#ifdef PETDEBUG_SUBSFAILS
    failure *uf = NULL;
#endif

    if(forward ==false && backward == false) {
      stats.fsubs_fi++;
    }
    else {
      bool f1 = true, b1 = true;
      fs::qc_compatible_subs(olditem->qc_vector_subs(),
                             newitem->qc_vector_subs(),
                             f1, b1);

#ifdef PETDEBUG_SUBSFAILS
      start_recording_failures();
#endif

      if(forward ==false && backward == false)
        stats.fsubs_qc++;
      else
        subsumes(olditem->get_fs(), newitem->get_fs(),
                 forward, backward);

#ifdef PETDEBUG_SUBSFAILS
      uf = stop_recording_failures();
#endif

#if 0
      //
      // according to ulrich (sometime mid-2004), we sometimes hit this
      // condition (for the ERG), hence subsumption quick check remains
      // off for the time being.                         (11-jan-05; oe)
      //
      if(f1 == false && forward || b1==false && backward)
        {
          fprintf(stderr, "S | > %c vs %c | < %c vs %c\n",
                  f1 ? 't' : 'f',
                  forward ? 't' : 'f',
                  b1 ? 't' : 'f',
                  backward ? 't' : 'f');
        }
#endif
    }

#ifdef PETDEBUG_SUBSFAILS
    if ((! ((forward && !olditem->blocked()) &&
            ((!backward && (opt_packing & PACKING_PRO))
             || (backward && (opt_packing & PACKING_EQUI)))))
        &&
        (! (backward && (opt_packing & PACKING_RETRO) && !olditem->frosted())))
      {
        const char *id1 = (newitem->rule() != NULL)
          ? newitem->rule()->printname() : newitem->printname() ;
        const char *id2 = (olditem->rule() != NULL)
          ? olditem->rule()->printname() : olditem->printname() ;
        if (uf != NULL)
          LOG(logParse, DEBUG, "SF: " << id1 << " <-> " << id2 << " " << *uf);
        else
          LOG(logParse, DEBUG, "SF: " << id1 << " <-> " << id2);
      }
#endif

    if(forward && !olditem->blocked()) {
      if((!backward && (opt_packing & PACKING_PRO))
         || (backward && (opt_packing & PACKING_EQUI))) {
        LOG(logPack, DEBUG, "proactive (" << (backward ? "equi" : "subs")
            << ") packing:" << endl << *newitem << endl
            << " --> " << endl << *olditem << endl);

        if(backward)
          stats.p_equivalent++;
        else
          stats.p_proactive++;

        olditem->packed.push_back(newitem);
        return true;
      }
    }

    if(backward && (opt_packing & PACKING_RETRO) && !olditem->frosted()) {
      LOG(logPack, DEBUG,  "retroactive packing:" << endl
          << *newitem << " <- " << *olditem << endl);

      newitem->packed.splice(newitem->packed.begin(), olditem->packed);

      if(!olditem->blocked()) {
        stats.p_retroactive++;
        newitem->packed.push_back(olditem);
      }

      olditem->frost();

      // delete (old, chart)
    }
  }
  return false;
}

/** return \c true if parsing should be stopped because enough results have
 *  been found
 */
inline bool result_limits() {
  // in no-unpacking mode (aiming to determine parseability only), have we
  // found at least one tree?
  if ((opt_packing & PACKING_NOUNPACK) != 0 && stats.trees > 0) return true;

  // in (non-packing) best-first mode, is the number of trees found equal to
  // the number of requested solutions?
  // opt_packing w/unpacking implies exhaustive parsing
  if ((! opt_packing
       && opt_nsolutions != 0 && stats.trees >= opt_nsolutions)
#ifdef YY
      || (opt_nth_meaning != 0 && stats.nmeanings >= opt_nth_meaning)
#endif
      ) return true;
  return false;
}


bool add_item(tItem *it) {
  assert(!it->blocked());

#ifdef PETDEBUG
  LOG(logParse, DEBUG, "add_item " << *it);
#endif

  if(it->passive()) {
    // \todo how could there be a packed edge if packing is not on??
    if(opt_packing && packed_edge(it))
      return false;

    Chart->add(it);

    type_t rule;
    if(it->root(Grammar, Chart->rightmost(), rule)) {
      it->set_result_root(rule);
      // Add the tree to the complete results
      Chart->trees().push_back(it);
      // Count all trees for now, some of these may still be blocked in
      // the packing parser.
      stats.trees++;
      if(stats.first == -1) {
        stats.first = ParseTime.convert2ms(ParseTime.elapsed());
      }
      if (result_limits()) return true;
    }

    postulate(it);
    fundamental_for_passive(it);
  }
  else {
    Chart->add(it);
    fundamental_for_active(dynamic_cast<tPhrasalItem *> (it));
  }
  return false;
}

inline bool
resources_exhausted(int pedgelimit, long memlimit, 
                    int timeout, int timestamp)
{
  return (pedgelimit > 0 && Chart->pedges() >= pedgelimit) ||
    (memlimit > 0 && (p_alloc.max_usage_mb() + t_alloc.max_usage_mb()) >= memlimit) ||
    (timeout > 0 && timestamp >= timeout );
}

//
// it appears .timeout. and .timestamp. are global values, also used in code
// outside of this file.  to (at least) avoid further proliferation of global
// variables, encapsulate everything in a function, which records the reason
// for resource exhaustion in a string reference.
// _fix_me_
// we should probably push this a little further, for example (a) provide a way
// of (re-)initializing limits and counters, e.g. by passing in a non-empty
// .message. argument; (b) recording the first .message. (i.e. first cause of
// resource exhaustion) in a static variable, to avoid accumulating the same
// error multiple times (as currently can happen in parsing and unpacking); and
// (c) provide optional arguments to allow a certain percentage of overrunning,
// e.g. a margin of ten percent for at least a shot at unpacking.  this calls
// for a little more thinking and consultation with PET co-developers.
//                                                              (28-aug-11; oe)
bool
test_resource_limits(std::string &message)
{

  static int pedges = -1;
  if(pedges == -1) pedges = get_opt_int("opt_pedgelimit");
  if(pedges > 0 && Chart != NULL && Chart->pedges() >= pedges) {
    ostringstream buffer;
    buffer << "resource limit exhausted (" 
           << Chart->pedges() << " passive edges)";
    message = buffer.str();
    return true;
  } //if

  static long int memory = -1;
  if(memory == -1) memory = get_opt_int("opt_memlimit");
  if(memory > 0 && (p_alloc.max_usage_mb() + t_alloc.max_usage_mb()) >= memory) {
    ostringstream buffer;
    buffer << "resource limit exhausted (" 
           << memory << " MB)";
    message = buffer.str();
    return true;
  } // if

  if(timeout > 0) {
    if(times(NULL) >= timeout) {
      ostringstream buffer;
      buffer << "resource limit exhausted (" 
             << get_opt_int("opt_timeout") / sysconf(_SC_CLK_TCK) 
             << " seconds)";
      message = buffer.str();
      return true;
    } // if
  } //if

  return false;
  
} // test_resource_limits()


void
parse_loop(fs_alloc_state &FSAS, list<tError> &errors, clock_t timeout) {
  long memlimit = get_opt_int("opt_memlimit");
  int pedgelimit = get_opt_int("opt_pedgelimit");

  //
  // run the core parser loop until either (a) we empty out the agenda, (b) we
  // hit a resource limit, or (c) in (non-packing) best-first mode, the number
  // of trees found equals the number of requested solutions.
  //
  while(! Agenda->empty() &&
        ! resources_exhausted(pedgelimit, memlimit, timeout, timestamp)) {

    basic_task* t = Agenda->pop();
#ifdef PETDEBUG
    LOG(logParse, DEBUG, t);
#endif
    tItem *it = t->execute();
    delete t;
    if (timeout > 0) timestamp = times(NULL);
    // add_item checks all limits that have to do with the number of
    // analyses. If it returns true that means that one of these limits has
    // been reached
    if ((it != 0) && add_item(it)) break;
  }
}

/**
 * Unpacks the parse forest selectively (using the max-ent model).
 * \return number of unpacked trees
 */
int unpack_selectively(std::vector<tItem*> &trees, int upedgelimit,
                       long memlimit, int nsolutions,
                       timer *UnpackTime , vector<tItem *> &readings) {
  int nres = 0;
  if (memlimit > 0 && (p_alloc.max_usage_mb() + t_alloc.max_usage_mb()) >= memlimit)
    //
    // _fix_me_
    // for all i can tell, the actual selective unpacking code does not always
    // respect resource limitations in terms of memory; in case we are out of
    // memory at this point already, return immediately.        (12-nov-11; oe)
    //
    return nres;

  if (get_opt_int("opt_timeout") > 0)
    timestamp = times(NULL); // FIXME passing NULL is not defined in POSIX

  // selectively unpacking
  list<tItem*> uroots;
  for (vector<tItem*>::iterator tree = trees.begin();
       // TODO: this should be checked beforehand, because it does not change
       // in the loop, and will either lead to no or all trees ending up in
       // uroots, or am i wrong??
       (upedgelimit == 0 || stats.p_upedges <= upedgelimit)
       // END TODO
         && tree != trees.end(); ++tree) {
    if (! (*tree)->blocked()) {
      stats.trees ++;
      uroots.push_back(*tree);
    }
  }
  list<tItem*> results
    = tItem::selectively_unpack(uroots, nsolutions
                                , Chart->rightmost(), upedgelimit, memlimit);

  static tCompactDerivationPrinter cdp;
  for (list<tItem*>::iterator res = results.begin();
       res != results.end(); ++res) {
    readings.push_back(*res);
    LOG(logParse, DEBUG,
        "unpacked[" << nres << "] (" << setprecision(1)
        << (float) (UnpackTime->convert2ms(UnpackTime->elapsed()) / 1000.0)
        << "): " << endl
        << (std::pair<tAbstractItemPrinter *, const tItem *>(&cdp, *res))
        << endl);
    nres++;
  }
  return nres;
}


/**
 * Unpacks the parse forest exhaustively.
 * \return number of unpacked trees
 * \todo why is opt_nsolutions not honored here
 */
int unpack_exhaustively(vector<tItem*> &trees, int upedgelimit,
                        timer *UnpackTime, vector<tItem *> &readings) {
  int nres = 0;
  if (get_opt_int("opt_timeout") > 0)
    timestamp = times(NULL);
  for(vector<tItem *>::iterator tree = trees.begin();
      (upedgelimit == 0 || stats.p_upedges <= upedgelimit)
        && tree != trees.end(); ++tree) {
    if (get_opt_int("opt_timeout") > 0 && timestamp >= timeout)
      break;
    if(! (*tree)->blocked()) {

      stats.trees++;

      list<tItem *> results;

      results = (*tree)->unpack(upedgelimit);

      static tCompactDerivationPrinter cdp;
      for(list<tItem *>::iterator res = results.begin();
          res != results.end(); ++res) {
        type_t rule;
        if((*res)->root(Grammar, Chart->rightmost(), rule)) {
          (*res)->set_result_root(rule);
          readings.push_back(*res);
          LOG(logParse, DEBUG,
              "unpacked[" << nres << "] (" << setprecision(1)
              << (float)(UnpackTime->convert2ms(UnpackTime->elapsed()) / 1000.0)
              << "): " << endl
              << (std::pair<tAbstractItemPrinter *, const tItem *>(&cdp, *res))
              << endl);
          nres++;
        }
      }
    }
  }
  return nres;
}

vector<tItem*>
collect_readings(fs_alloc_state &FSAS, list<tError> &errors,
                 int pedgelimit, long memlimit, int nsolutions,
                 vector<tItem*> &trees) {
  vector<tItem *> readings;

  if(opt_packing && !(opt_packing & PACKING_NOUNPACK)) {
    // \todo What if there are already valid solutions but the edge limit has
    // been hit? Why is there no unpacking at all
    if (pedgelimit == 0 || Chart->pedges() < pedgelimit) {
      timer *UnpackTime = new timer();
      stats.trees = 0; // We want to recount the trees in case some
                       // are blocked or don't unpack.
      int upedgelimit = pedgelimit ? pedgelimit - Chart->pedges() : 0;

      if ((opt_packing & PACKING_SELUNPACK)
          && nsolutions > 0
          && Grammar->sm()) {
        try {
          unpack_selectively(trees, upedgelimit, memlimit, nsolutions,
                             UnpackTime, readings);
        } catch(tError e) {
          errors.push_back(e);
        }
      } else { // unpack exhaustively
        try {
          unpack_exhaustively(trees, upedgelimit, UnpackTime, readings);
        } catch(tError e) {
          errors.push_back(e);
        }
      }

      if(upedgelimit > 0 && stats.p_upedges > upedgelimit) {
        ostringstream s;
        s << "unpack edge limit exhausted (" << upedgelimit
          << " pedges)";
        errors.push_back(s.str());
      }

      if (get_opt_int("opt_timeout") > 0 && timestamp >= timeout) {
        ostringstream s;
        s << "timed out ("
          << get_opt_int("opt_timeout") / sysconf(_SC_CLK_TCK)
          << " s)";
        errors.push_back(s.str());
      }

      stats.p_utcpu = UnpackTime->convert2ms(UnpackTime->elapsed());
      stats.p_dyn_bytes = FSAS.dynamic_usage() - stats.dyn_bytes;
      stats.p_stat_bytes = FSAS.static_usage() - stats.stat_bytes;
      FSAS.clear_stats();
      delete UnpackTime;
    }
  } else {
    readings = trees;
  }

  // This is not the ideal solution if parsing could be restarted
  if(Grammar->sm())
    sort(readings.begin(), readings.end(), item_greater_than_score());

  return readings;
}


void
parse_finish(fs_alloc_state &FSAS, list<tError> &errors, clock_t timeout) {
  long memlimit = get_opt_int("opt_memlimit");
  int pedgelimit = get_opt_int("opt_pedgelimit");
  clock_t timestamp = (timeout > 0 ? times(NULL) : 0);

  stats.tcpu = ParseTime.convert2ms(ParseTime.elapsed());

  stats.dyn_bytes = FSAS.dynamic_usage();
  stats.stat_bytes = FSAS.static_usage();

  get_unifier_stats();
  Chart->get_statistics();

  if(resources_exhausted(pedgelimit, memlimit, timeout, timestamp)) {
    ostringstream s;
    if (memlimit > 0 
        && (p_alloc.max_usage_mb() + t_alloc.max_usage_mb()) >= memlimit) {
      s << "memory limit exhausted (" << memlimit << " MB)";
    }
    else if (pedgelimit > 0 && Chart->pedges() >= pedgelimit) {
      s << "edge limit exhausted (" << pedgelimit << " pedges)";
    }
    else {
      s << "timed out (" << get_opt_int("opt_timeout") / sysconf(_SC_CLK_TCK)
          << " s)";
    }
    errors.push_back(s.str());
  }

  LOG(logParse, DEBUG, *Chart);

  if(get_opt_int("opt_tsdb") & 32) {
    //
    // in lexical-only mode, the parser has halted after lexical parsing, and
    // what remains to be done is a mimicry of finding actual results; in this
    // mode, all edges that could have fed into syntactic rules count as valid
    // results, i.e. in effect we output a lexical lattice.     (2-jul-11; oe)
    //
    for(chart_iter item(Chart); item.valid(); ++item) {
      if(passive_unblocked_non_input(item.current())
         && item.current()->inflrs_complete_p())
        Chart->readings().push_back(item.current());
    } // for
    Chart->trees() = Chart->readings();
    stats.trees = Chart->trees().size();
  }
  else
    Chart->readings() 
      = collect_readings(FSAS, errors, pedgelimit, memlimit, 
                         opt_nsolutions, Chart->trees());

  stats.readings = Chart->readings().size();

}




void
analyze(string input, chart *&C, fs_alloc_state &FSAS
        , list<tError> &errors, int id)
{
  // optionally clearing memory before rather than after analyzing since
  // we want to allow for lazy output of parse results in a server mode
  if (get_opt_bool("opt_shrink_mem")) {
    FSAS.may_shrink();
    prune_glbcache();
  }

  FSAS.clear_stats();
  stats.reset();
  stats.id = id;

  Chart = C;
  auto_ptr<item_owner> owner(new item_owner);
  tItem::default_owner(owner.get());

  unify_wellformed = true;

  //
  // really, start timing here.  for JaCY (as of jan-05), input processing
  // takes significant time.                               (10-feb-05; oe)
  // even more so, now that PET sports native support for REPP and calling out
  // to external taggers.                                   (5-aug-11; oe)
  //
  TotalParseTime.start();
  ParseTime.reset(); ParseTime.start();
  if (get_opt_int("opt_timeout") > 0) {
    timestamp = times(NULL);
    timeout = timestamp + (clock_t)get_opt_int("opt_timeout");
  }

  int max_pos = 0;
  inp_list input_items;
  bool chart_mapping = get_opt_int("opt_chart_mapping") != 0;
  try {
    max_pos = Lexparser.process_input(input, input_items, chart_mapping);
  } // try
  catch(tError e) {
    input_items.clear();
    errors.push_back(e);
  } // catch

  if (get_opt_int("opt_chart_pruning") != 0) {
    Agenda = new tLocalCapAgenda (get_opt_int ("opt_chart_pruning"), max_pos);
  } else {
    Agenda = new tExhaustiveAgenda;
  }

  C = Chart = new chart(max_pos, owner);

  if(input_items.size()) {
    Lexparser.lexical_processing(input_items, chart_mapping,
                                 (chart_mapping
                                  || cheap_settings->lookup("lex-exhaustive")),
                                 FSAS, errors);

    // during lexical processing, the appropriate tasks for the syntactic stage
    // are already created
    if(!(get_opt_int("opt_tsdb") & 32)) parse_loop(FSAS, errors, timeout);
  } //if

  ParseTime.stop();
  TotalParseTime.stop();

  parse_finish(FSAS, errors, timeout);

  if(get_opt_int("opt_robust") != 0 && (Chart->readings().empty()))
    analyze_pcfg(Chart, FSAS, errors);

  //
  // _fix_me_
  // with .max_pos. == 0, lexparser::reset() will SEGV; maybe contact bernd?
  //                                                            (27-aug-11; oe)
  if(input_items.size()) Lexparser.reset();
  // clear_dynamic_types(); // too early
  delete Agenda;
}

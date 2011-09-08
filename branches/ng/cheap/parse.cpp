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

#include "resources.h"
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

// output for excessive subsumption failure debugging in the German grammar
//#define PETDEBUG_SUBSFAILS

//
// global variables for parsing
//

chart *Chart = NULL;
tAbstractAgenda *Agenda = NULL;

static bool parser_init();
//options managed by configuration subsystem
bool opt_hyper = parser_init();
int  opt_nsolutions, opt_packing;

timer TotalParseTime(false);

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
//clock_t timeout;
//clock_t timestamp;
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
       && opt_nsolutions != 0 && stats.trees >= opt_nsolutions))
    return true;
  return false;
}


bool add_item(tItem *it, Resources &resources) {
  assert(!it->blocked());

#ifdef PETDEBUG
  LOG(logParse, DEBUG, "add_item " << *it);
#endif

  if(it->passive()) {
    ++ resources.pedges;
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
        stats.first = resources.get_stage_time_ms();
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


void
parse_loop(fs_alloc_state &FSAS, list<tError> &errors, Resources &resources) {

  //
  // run the core parser loop until either (a) we empty out the agenda, (b) we
  // hit a resource limit, or (c) in (non-packing) best-first mode, the number
  // of trees found equals the number of requested solutions.
  //
  while(! Agenda->empty() && ! resources.exhausted()) {

    basic_task* t = Agenda->pop();
#ifdef PETDEBUG
    LOG(logParse, DEBUG, t);
#endif
    tItem *it = t->execute();
    delete t;

    // add_item checks all limits that have to do with the number of
    // analyses. If it returns true that means that one of these limits has
    // been reached
    if (it != 0 && add_item(it, resources))
      break;
  }
}


extern int unpack_exhaustively(vector<tItem*> &trees, int chart_length,
                               Resources& resources, vector<tItem *> &readings);

extern int unpack_selectively(std::vector<tItem*> &trees, int nsolutions,
                              int chart_length, Resources &resources,
                              vector<tItem *> &readings,  list<tError> &errors);

void
collect_readings(fs_alloc_state &FSAS, list<tError> &errors,
                 Resources &resources, int nsolutions,
                 vector<tItem*> &trees) {
  vector<tItem *> readings;

  int pedges = resources.pedges;

  if(opt_packing && !(opt_packing & PACKING_NOUNPACK)) {
    int nres = 0;
    // We want to recount the trees in case some are blocked or don't unpack.
    stats.trees = 0;

    if ((opt_packing & PACKING_SELUNPACK)
        && nsolutions > 0
        && Grammar->sm()) {
      nres = unpack_selectively(trees, nsolutions, Chart->rightmost(),
                                resources, readings, errors);
    } else {
      nres = unpack_exhaustively(trees, Chart->rightmost(), resources,
                                 readings);
    }
    if (resources.exhausted()) {
      errors.push_back(tExhaustedError(resources.exhaustion_message()));
    }

    stats.p_upedges = resources.pedges - pedges;

    stats.p_utcpu = resources.get_stage_time_ms();
    stats.p_dyn_bytes = FSAS.dynamic_usage();
    stats.p_stat_bytes = FSAS.static_usage();
    FSAS.clear_stats();
  } else {
    readings = trees;
  }

  // This is not the ideal solution if parsing could be restarted
  if(Grammar->sm())
    sort(readings.begin(), readings.end(), item_greater_than_score());

  Chart->readings() = readings;
}


void
analyze(string input, chart *&C, fs_alloc_state &FSAS
        , list<tError> &errors, int id) {
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

  // really, start timing here.  for JaCY (as of jan-05), input processing
  // takes significant time.                               (10-feb-05; oe)
  // even more so, now that PET sports native support for REPP and calling out
  // to external taggers.                                   (5-aug-11; oe)
  Resources resources;

  TotalParseTime.start();
  resources.start_run();

  int max_pos = 0;
  inp_list input_items;
  try {
    max_pos = Lexparser.process_input(input, input_items, resources);
  } // try
  catch(tError e) {
    input_items.clear();
    Lexparser.reset();
    errors.push_back(e);
    // eventually rethrow
    if (e.severe())
      throw e;
  } // catch

  if(input_items.size() > 0) {
    if (get_opt_int("opt_chart_pruning") != 0) {
      Agenda = new tLocalCapAgenda (get_opt_int ("opt_chart_pruning"), max_pos);
    } else {
      Agenda = new tExhaustiveAgenda;
    }

    C = Chart = new chart(max_pos, owner);
    Lexparser.lexical_processing(input_items, FSAS, errors, resources);
    // TODO: THIS IS A BAD HACK TO SIMULATE THAT EDGES IN THE PREPROCESSING
    // STAGE ARE NOT COUNTED. IN FACT THEY ARE, SINCE THE CODE USES
    // add_item (SEE ABOVE)
    resources.pedges = 0;

    resources.start_next_stage();
    // during lexical processing, the appropriate tasks for the syntactic stage
    // are already created
    if(!(get_opt_int("opt_tsdb") & 32)) {
      parse_loop(FSAS, errors, resources);
      if (resources.exhausted()) {
        errors.push_back(tExhaustedError(resources.exhaustion_message()));
      }

      // post-parsing (tree creation) statistics
      stats.tcpu = resources.get_stage_time_ms();

      stats.dyn_bytes = FSAS.dynamic_usage();
      stats.stat_bytes = FSAS.static_usage();

      get_unifier_stats(stats);
      Chart->get_statistics(stats);

      LOG(logParse, DEBUG, *Chart);

      // start unpacking (eventually)
      resources.start_next_stage();
      collect_readings(FSAS, errors, resources, opt_nsolutions, Chart->trees());
    } else {
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

    stats.readings = Chart->readings().size();

    if(get_opt_int("opt_robust") != 0 && (Chart->readings().empty())) {
      resources.start_next_stage();
      analyze_pcfg(Chart, FSAS, errors, resources);
    }

    Lexparser.reset();
    // clear_dynamic_types(); // too early
    delete Agenda;
  } //if input_items.size() > 0

  resources.stop_run();
  TotalParseTime.stop();

}

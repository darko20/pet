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
#include "fs.h"
#include "item.h"
#include "item-printer.h"
#include "grammar.h"
#include "chart.h"
#include "lexparser.h"
#include "task.h"
#include "tsdb++.h"

#include <sstream>
#include <iostream>
#include <sys/times.h>
#include <unistd.h>

using namespace std;

#ifdef YY
#include "yy.h"
#endif

// output for excessive subsumption failure debugging in the German grammar
//#define DEBUG_SUBSFAILS

//
// global variables for parsing
//

chart *Chart;
tAgenda *Agenda;

timer ParseTime;
timer TotalParseTime(false);

static bool parser_init();
//options managed by configuration subsystem
bool opt_filter, opt_hyper = parser_init();
int  opt_nsolutions, opt_packing;

#ifdef YY
int opt_nth_meaning;
#endif

/** Initialize global variables and options for the parser */
static bool parser_init() {
  reference_opt("opt_filter", "Use the static rule filter", opt_filter);
  opt_filter = true;
  reference_opt("opt_hyper", "use hyperactive parsing", opt_hyper);
  opt_hyper = true;
  reference_opt("opt_nsolutions",
                       "The number of solutions until the parser is"
                       "stopped, if not in packing mode", opt_nsolutions);
  opt_nsolutions = 0;
  reference_opt("opt_packing",
                       "a bit vector of flags: 1:equivalence "
                       "2:proactive 4:retroactive packing "
                       "8:selective 128:no unpacking", opt_packing);
  opt_packing = 0;
  managed_opt("opt_pedgelimit", "maximum number of passive edges",
                    (int) 0);
  managed_opt("opt_shrink_mem", "shrink process size after huge items",
                    true);
  return opt_hyper;
}




// timeout control for parsing 
// timer class is not suitable because: 
//  i) we do not need microsecond-level accuracy
//  ii) timer based on CLOCK(3) can wrap around after 36 minutes
// therefore it is more suitable to use TIMES(2)
clock_t timeout;
clock_t timestamp;
//
// filtering
//

bool
filter_rule_task(grammar_rule *R, tItem *passive)
{
    LOG_ONLY(PrintfBuffer pb);
    LOG_ONLY(pbprintf(pb, "trying "));
    LOG_ONLY(R->print(pb));
    LOG_ONLY(pbprintf(pb, " & passive "));
    LOG_ONLY(passive->print(pb));
    LOG_ONLY(pbprintf(pb, " ==> "));

    if(opt_filter && !Grammar->filter_compatible(R, R->nextarg(),
                                                 passive->rule()))
    {
        stats.ftasks_fi++;
        LOG_ONLY(pbprintf(pb, "filtered (rf)"));
        return false;
    }

    if(opt_nqc_unif != 0
       && !fs::qc_compatible_unif(R->qc_vector_unif(R->nextarg()),
                                  passive->qc_vector_unif()))
    {
        stats.ftasks_qc++;
        LOG_ONLY(pbprintf(pb, "filtered (qc)"));
        return false;
    }

    LOG_ONLY(pbprintf(pb, "passed filters"));

    LOG(loggerParse, Level::DEBUG, "%s", pb.getContents());
    
    return true;
}

bool
filter_combine_task(tItem *active, tItem *passive)
{
    LOG_ONLY(PrintfBuffer pb);
    LOG_ONLY(pbprintf(pb, "trying active "));
    LOG_ONLY(active->print(pb));
    LOG_ONLY(pbprintf(pb, " & passive "));
    LOG_ONLY(passive->print(pb));
    LOG_ONLY(pbprintf(pb, " ==> "));

    if(opt_filter && !Grammar->filter_compatible(active->rule(),
                                                 active->nextarg(),
                                                 passive->rule()))
    {
        LOG_ONLY(pbprintf(pb, "filtered (rf)\n"));

        stats.ftasks_fi++;
        return false;
    }

    if(opt_nqc_unif != 0
       && !fs::qc_compatible_unif(active->qc_vector_unif(),
                                  passive->qc_vector_unif()))
    {
        LOG_ONLY(pbprintf(pb, "filtered (qc)\n"));

        stats.ftasks_qc++;
        return false;
    }

    LOG_ONLY(pbprintf(pb, "passed filters\n"));
    LOG(loggerParse, Level::DEBUG, "%s", pb.getContents());

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
  // iterate over all the rules in the grammar
  for(ruleiter rule = Grammar->rules().begin(); rule != Grammar->rules().end();
      rule++) {
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
    for(chart_iter_adj_active it(Chart, passive); it.valid(); it++)
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

  for(chart_iter_adj_passive it(Chart, active); it.valid(); it++)
    if(!it.current()->blocked())
      if(it.current()->compatible(active, Chart->rightmost()))
        if(filter_combine_task(active, it.current()))
          Agenda->push(new
                       active_and_passive_task(Chart, Agenda,
                                               active, it.current()));
}

extern void start_recording_failures();
extern class failure * stop_recording_failures();

bool
packed_edge(tItem *newitem) {
  if(! newitem->inflrs_complete_p()) return false;

  for(chart_iter_span_passive iter(Chart, newitem->start(), newitem->end());
      iter.valid(); iter++) {
    bool forward, backward;
    tItem *olditem = iter.current();

    if(!olditem->inflrs_complete_p() || (olditem->trait() == INPUT_TRAIT))
      continue;

    forward = backward = true;

    // YZ 2007-07-25: avoid packing item with its offspring edges
    // (both forward and backward)
    if (newitem->contains_p(olditem))
          continue;

    if(opt_filter)
      Grammar->subsumption_filter_compatible(olditem->rule(),
                                             newitem->rule(),
                                             forward, backward);

#ifdef DEBUG_SUBSFAILS
    failure *uf = NULL;
#endif

    if(forward ==false && backward == false) {
      stats.fsubs_fi++;
    }
    else {
      bool f1 = true, b1 = true;
      if(opt_nqc_subs != 0)
        fs::qc_compatible_subs(olditem->qc_vector_subs(),
                               newitem->qc_vector_subs(),
                               f1, b1);

#ifdef DEBUG_SUBSFAILS
      start_recording_failures();
#endif

      if(forward ==false && backward == false)
        stats.fsubs_qc++;
      else
        subsumes(olditem->get_fs(), newitem->get_fs(),
                 forward, backward);

#ifdef DEBUG_SUBSFAILS
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

#ifdef DEBUG_SUBSFAILS
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
        fprintf(ferr, "SF: %s <-> %s ", id1, id2);
        if (uf != NULL) uf->print(ferr);
        fprintf(ferr, "\n");
      }
#endif

    if(forward && !olditem->blocked()) {
      if((!backward && (opt_packing & PACKING_PRO))
         || (backward && (opt_packing & PACKING_EQUI))) {
        
        LOG_ONLY(PrintfBuffer pb);
        LOG_ONLY(pbprintf(pb, "proactive (%s) packing:\n", backward
                  ? "equi" : "subs"));
        LOG_ONLY(newitem->print(pb));
        LOG_ONLY(pbprintf(pb, "\n --> \n"));
        LOG_ONLY(olditem->print(pb));
        LOG_ONLY(pbprintf(pb, "\n"));
        LOG(loggerParse, Level::DEBUG, "%s", pb.getContents());
                
        if(backward)
          stats.p_equivalent++;
        else
          stats.p_proactive++;
                
        olditem->packed.push_back(newitem);
        return true;
      }
    }
      
    if(backward && (opt_packing & PACKING_RETRO) && !olditem->frosted()) {
      LOG_ONLY(PrintfBuffer pb);
      LOG_ONLY(pbprintf(pb, "retroactive packing:\n"));
      LOG_ONLY(newitem->print(pb));
      LOG_ONLY(pbprintf(pb, " <- "));
      LOG_ONLY(olditem->print(pb));
      LOG_ONLY(pbprintf(pb, "\n"));
      LOG(loggerParse, Level::DEBUG, "%s", pb.getContents());

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

inline bool result_limits() {
  // in no-unpacking mode (aiming to determine parseability only), have we
  // found at least one tree?
  if ((opt_packing & PACKING_NOUNPACK) != 0 && stats.trees > 0) return true;
  
  // in (non-packing) best-first mode, is the number of trees found equal to
  // the number of requested solutions?
  // opt_packing w/unpacking implies exhaustive parsing
  if (! opt_packing
      || (opt_nsolutions == 0 || stats.trees < opt_nsolutions)
#ifdef YY
      || (opt_nth_meaning == 0 || stats.nmeanings < opt_nth_meaning)
#endif
      ) return true;
  return false;
}


bool add_item(tItem *it) {
  assert(!it->blocked());
  
  LOG_ONLY(PrintfBuffer pb);
  LOG_ONLY(pbprintf(pb, "add_item "));
  LOG_ONLY(it->print(pb));
  LOG(loggerParse, Level::DEBUG, "%s", pb.getContents());

  if(it->passive()) {
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

void
parse_loop(fs_alloc_state &FSAS, list<tError> &errors, int pedgelimit) {
  long memlimit = get_opt_int("memlimit") * 1024 * 1024; 

  //
  // run the core parser loop until either (a) we empty out the agenda, (b) we
  // hit a resource limit, or (c) in (non-packing) best-first mode, the number
  // of trees found equals the number of requested solutions.
  //
  while(! Agenda->empty()) {
    // edge limit hit? 
    if (pedgelimit > 0 && Chart->pedges() >= pedgelimit) {
      ostringstream s;
      s << "edge limit exhausted (" << pedgelimit << " pedges)";
      errors.push_back(s.str());
      break;
    }
    // memory limit hit?
    if (memlimit > 0 && t_alloc.max_usage() >= memlimit) {
      ostringstream s;
      s << "memory limit exhausted (" << memlimit / (1024 * 1024) << " MB)";
      errors.push_back(s.str());
      break;
    }
    // test auf timeout fehlt hier

    basic_task* t = Agenda->pop();
#ifdef DEBUG
    t->print(stderr); fprintf(stderr, "\n");
#endif
    tItem *it = t->execute();
    delete t;
    if (get_opt_int("opt_timeout") > 0)
      timestamp = times(NULL);
    // add_item checks all limits that have to do with the number of
    // analyses. If it returns true that means that one of these limits has
    // been reached
    if ((it != 0) && add_item(it)) break;
  }
}

int unpack_selectively(vector<tItem*> &trees, int upedgelimit, int nsolutions
                       , timer *UnpackTime , vector<tItem *> &readings) {
  int nres = 0;
  if (get_opt_int("opt_timeout") > 0)
    timestamp = times(NULL);

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
                                , Chart->rightmost(), upedgelimit);

  tCompactDerivationPrinter cdp(cerr, false);
  for (list<tItem*>::iterator res = results.begin();
       res != results.end(); res++) {
    //type_t rule;
    //if((*res)->root(Grammar, Chart->rightmost(), rule)) {
    // the checking is moved into selectively_unpack()
    readings.push_back(*res);

    LOG_ONLY(PrintfBuffer pb);
    LOG_ONLY(pbprintf(pb, "unpacked[%d] (%.1f): ", nres++,
                      UnpackTime->convert2ms(UnpackTime->elapsed())
                      / 1000.));
    LOG_ONLY((*res)->print_derivation(pb, false));
    LOG(loggerParse, Level::DEBUG, "%s", pb.getContents());
    //} 
  }
  return nres;
}


// \todo why is opt_nsolutions not honored here
int unpack_exhaustively(vector<tItem*> &trees, int upedgelimit
                        , timer *UnpackTime, vector<tItem *> &readings) {
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
      
      // this has to be changed
      tCompactDerivationPrinter cdp(cerr, false);
      for(list<tItem *>::iterator res = results.begin();
          res != results.end(); ++res) {
        type_t rule;
        if((*res)->root(Grammar, Chart->rightmost(), rule)) {
          readings.push_back(*res);
          LOG_ONLY(PrintfBuffer pb);
          LOG_ONLY(pbprintf(pb, "unpacked[%d] (%.1f): ", nres++,
                            UnpackTime->convert2ms(UnpackTime->elapsed())
                            / 1000.));
          // should also be logged???
          cdp.print(*res);
          LOG(loggerParse, Level::DEBUG, "%s", pb.getContents());
        }
      }
    }
  }
  return nres;
}

vector<tItem*>
collect_readings(fs_alloc_state &FSAS, list<tError> &errors, 
                 int pedgelimit, int nsolutions, vector<tItem*> &trees) {
  vector<tItem *> readings;

  if(opt_packing && !(opt_packing & PACKING_NOUNPACK)) {
    // \todo What if there are already valid solutions but the edge limit has
    // been hit? Why is there no unpacking at all
    if (pedgelimit == 0 || Chart->pedges() < pedgelimit) {
      timer *UnpackTime = new timer();
      int nres = 0;
      stats.trees = 0; // We want to recount the trees in case some
                       // are blocked or don't unpack.
      int upedgelimit = pedgelimit ? pedgelimit - Chart->pedges() : 0;
      
      if ((opt_packing & PACKING_SELUNPACK)
          && nsolutions > 0
          && Grammar->sm()) {
        nres = unpack_selectively(trees, upedgelimit, nsolutions,
                                  UnpackTime, readings);
      } else { // unpack exhaustively
        nres = unpack_exhaustively(trees, upedgelimit, UnpackTime, readings);
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
      stats.p_dyn_bytes = FSAS.dynamic_usage();
      stats.p_stat_bytes = FSAS.static_usage();
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
parse_finish(fs_alloc_state &FSAS, list<tError> &errors, int pedgelimit) {
  stats.tcpu = ParseTime.convert2ms(ParseTime.elapsed());

  stats.dyn_bytes = FSAS.dynamic_usage();
  stats.stat_bytes = FSAS.static_usage();
  FSAS.clear_stats();

  get_unifier_stats();
  Chart->get_statistics();

  if(get_opt_bool("opt_shrink_mem")) {
    FSAS.may_shrink();
    prune_glbcache();
  }

    LOG_ONLY(PrintfBuffer pb);  
    LOG_ONLY(Chart->print(pb));   
    LOG(loggerParse, Level::DEBUG, "%s", pb.getContents());if(verbosity > 8)
    if(verbosity > 8)
        Chart->print(cerr);
  
    if(resources_exhausted())
    {
        ostringstream s;

        if (memlimit > 0 && t_alloc.max_usage() >= memlimit)
            s << "memory limit exhausted (" << memlimit / (1024 * 1024) 
              << " MB)";
        else if (pedgelimit > 0 && Chart->pedges() >= pedgelimit)
            s << "edge limit exhausted (" << pedgelimit 
              << " pedges)";
        else 
          s << "timed out (" << opt_timeout / sysconf(_SC_CLK_TCK) 
            << " s)";
        errors.push_back(s.str());
    }

    Chart->readings() = collect_readings(FSAS, errors, 
                                         opt_nsolutions, pedgelimit,
                                         Chart->trees());
    stats.readings = Chart->readings().size();

}




void
analyze(string input, chart *&C, fs_alloc_state &FSAS
        , list<tError> &errors, int id)
{
    FSAS.clear_stats();
    stats.reset();
    stats.id = id;

    auto_ptr<item_owner> owner(new item_owner);
    tItem::default_owner(owner.get());

    unify_wellformed = true;

    inp_list input_items;
    int max_pos = Lexparser.process_input(input, input_items);

    Agenda = new tAgenda;
    C = Chart = new chart(max_pos, owner);
    
    //
    // really, start timing here.  for JaCY (as of jan-05), input processing
    // takes significant time.                               (10-feb-05; oe)
    //

    TotalParseTime.start();
    ParseTime.reset(); ParseTime.start();
    if (get_opt_int("opt_timeout") > 0) {
      timestamp = times(NULL);
      timeout = timestamp + (clock_t)get_opt_int("opt_timeout");
    }

    Lexparser.lexical_processing(input_items
                                 , cheap_settings->lookup("lex-exhaustive")
                                 , FSAS, errors);

    int pedgelimit;
    get_opt("pedgelimit", pedgelimit);
    // during lexical processing, the appropriate tasks for the syntactic stage
    // are already created
    parse_loop(FSAS, errors, pedgelimit);

    ParseTime.stop();
    TotalParseTime.stop();

    parse_finish(FSAS, errors, pedgelimit);

    Lexparser.reset();
    // clear_dynamic_types(); // too early
    delete Agenda;
}
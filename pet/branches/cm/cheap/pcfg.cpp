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

/* robust parsing with PCFG backbone */

#include "pcfg.h"
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
#include "sm.h"
#include "settings.h"
//#include "dag-robust.h"

#include <sstream>
#include <iostream>
#include <sys/times.h>
#include <unistd.h>
#include <map>

using namespace std;

#ifdef YY
#include "yy.h"
#endif


extern tAgenda* Agenda;
extern chart* Chart;
extern int opt_nsolutions, opt_packing;
extern clock_t timeout;

#ifdef YY
extern int opt_nth_meaning;
#endif

inline bool
pcfg_resources_exhausted(int pedgelimit, long memlimit, int timeout, int timestamp)
{
  return (pedgelimit > 0 && Chart->pedges() >= pedgelimit) ||
    (memlimit > 0 && t_alloc.max_usage() >= memlimit) ||
    (timeout > 0 && timestamp >= timeout );
}


/** return \c true if parsing should be stopped because enough results have
 *  been found
 */
inline bool pcfg_result_limits() {
  // in no-unpacking mode (aiming to determine parseability only), have we
  // found at least one tree?
  if ((opt_packing & PACKING_NOUNPACK) != 0 && stats.rtrees > 0) return true;
  
  // in (non-packing) best-first mode, is the number of trees found equal to
  // the number of requested solutions?
  // opt_packing w/unpacking implies exhaustive parsing
  if ((! opt_packing
       && opt_nsolutions != 0 && stats.rtrees >= opt_nsolutions)) 
    return true;
  return false;
}

// std::map<type_t,int> rule_count;

void
postulate_pcfg(tItem *passive) {
  for (ruleiter rule = Grammar->pcfg_rules().begin();
       rule != Grammar->pcfg_rules().end(); rule ++) {
    grammar_rule *R = *rule;
    if (passive->compatible_pcfg(R, Chart->rightmost())) {
//       if (rule_count.find(R->type()) == rule_count.end())
//         rule_count[R->type()] = 1;
//       else
//         rule_count[R->type()] ++;
      Agenda->push(new pcfg_rule_and_passive_task(Chart, Agenda, R, passive));
    }
  }
}


/** Equivalence based packing for PCFG edges
 * Note that they never pack with non-PCFG edges
 */
bool
packed_edge_pcfg(tItem *newitem) {
  //  if (newitem->trait() != PCFG_TRAIT) 
  //    return false;
  // if (! newitem->inflrs_complete_p()) return false;

  for (chart_iter_span_passive iter(Chart, newitem->start(), newitem->end());
       iter.valid(); iter ++) {
    tItem *olditem = iter.current();

    // only pack with other PCFG unblocked items
    if (olditem->trait() != PCFG_TRAIT || olditem->blocked())
      continue;
    
    // never pack with own offsprings // we might need to allow this because of recursive rules
    // if (newitem->contains_p(olditem))
    //   continue;
    
    if (olditem->identity() == newitem->identity()) {
      if (!newitem->cyclic_p(olditem)) // when packing happens for an item into its offspring, pretend to pack, but not put the new item on the packed list
        olditem->packed.push_back(newitem);
      stats.p_equivalent ++;
      return true;
    }
  }
  return false;
}


void fundamental_for_passive_pcfg(tItem *passive) {
  // iterate over all active items adjacent to passive and try combination
  for (chart_iter_adj_active it(Chart, passive); it.valid(); it ++) {
    tItem *active = it.current();
    if (active->adjacent(passive) && active->trait() == PCFG_TRAIT) // we ignore active edges from the first-stage parsing
      if (passive->compatible_pcfg(active, Chart->rightmost())) // still need to filter combine tasks?
        Agenda->push(new pcfg_active_and_passive_task(Chart, Agenda, active, passive));
  }
}

void fundamental_for_active_pcfg(tItem *active) {
  if (active->trait() != PCFG_TRAIT)
    return;
  // iterator over all passive items adjacent to active and try combination
  // no matter whether it is PCFG item or not, for the active one is sure to be PCFG
  for (chart_iter_adj_passive it(Chart, active); it.valid(); it ++) {
    if (opt_packing == 0 || !it.current()->blocked())
      if (it.current()->compatible_pcfg(active, Chart->rightmost()))
        Agenda->push(new pcfg_active_and_passive_task(Chart, Agenda, active, it.current()));
  }
}

inline bool
passive_item_exists(chart *C, int start, int end, type_t type, list<tItem*> dtrs) {
  if (opt_robust == 1) 
    return false;
  chart_iter_span_passive ci(C, start, end);
  while (ci.valid()) {
    tItem * item = ci.current();
    if (item->trait() != INPUT_TRAIT &&
        item->trait() != PCFG_TRAIT &&
        item->inflrs_complete_p() &&
        item->rule()->type() == type) {
      bool equal = true;
      if (opt_robust == 2)
        return equal;
      for (item_citer dtr1 = item->daughters().begin(), dtr2 = dtrs.begin();
           dtr1 != item->daughters().end() && dtr2 != dtrs.end();
           dtr1 ++, dtr2 ++) {
        if ((*dtr1) != (*dtr2)) {
          equal = false;
          break;
        }
      }
      if (equal)
        return true;
    }      
    ci ++;
  }
  return false;
}

tItem *
build_pcfg_rule_item(chart *C, tAgenda *A, grammar_rule *rule, tItem *passive)
{
  //fs_alloc_state FSAS(false);
   //stats.etasks++;
  list<tItem*> dtrs;
  dtrs.push_back(passive);
  if (rule->nextarg_pcfg() == passive->identity() &&
      (rule->restargs() != 0 ||
       !passive_item_exists(C, passive->start(), passive->end(), rule->type(), dtrs))) {
    return new tPhrasalItem(rule, passive);
  }
  return 0; 
}

tItem *
build_pcfg_combined_item(chart *C, tItem *active, tItem *passive) {
  type_t arg = active->nextarg_pcfg();
  if (arg == passive->identity()) {
    list<tItem*> dtrs(active->daughters());
    dtrs.push_back(passive);
    int start = active->left_extending() ? passive->start() : active->start();
    int end = active->left_extending() ? active->end() : passive->end();
    if (active->restargs() != 0 ||
        !passive_item_exists(C, start, end, active->rule()->type(), dtrs))
      return new tPhrasalItem(dynamic_cast<tPhrasalItem *>(active),
                              passive);
  }
  return 0;
}


pcfg_rule_and_passive_task::pcfg_rule_and_passive_task(chart *C, tAgenda *A,
                                                       grammar_rule *rule, tItem *passive)
  : basic_task(C,A), _rule(rule), _passive(passive)
{
  if (Grammar->pcfgsm()) {
    list<tItem *> daughters;
    daughters.push_back(passive);
    //if (! (opt_packing & PACKING_PCFGEQUI))
    priority(Grammar->pcfgsm()->scoreLocalTree(rule, daughters));
  }
}

tItem*
pcfg_rule_and_passive_task::execute()
{
  if (_passive->blocked())
    return 0;
  
  tItem *result = build_pcfg_rule_item(_Chart, _A, _rule, _passive);
  //if ((opt_packing & PACKING_PCFGEQUI) == 0 && result) 
  if (result)
    result->score(priority());
  return result;
}


pcfg_active_and_passive_task::pcfg_active_and_passive_task(chart *C, tAgenda *A,
                                                           tItem *act, tItem *passive)
  : basic_task(C, A), _active(act), _passive(passive) {
  if (Grammar->pcfgsm()) {
    tPhrasalItem *active = dynamic_cast<tPhrasalItem *>(act);
    list<tItem *> daughters(active->daughters());
    daughters.push_back(passive);
    //    if (! (opt_packing & PACKING_PCFGEQUI))
    priority(Grammar->pcfgsm()->scoreLocalTree(active->rule(), daughters));
  }
}

tItem *
pcfg_active_and_passive_task::execute() {
  if (_passive->blocked())
    return 0;

  tItem *result = build_pcfg_combined_item(_Chart, _active, _passive);
  //  if ((opt_packing & PACKING_PCFGEQUI) == 0 && result)
  if (result)
    result->score(priority());
  return result;
}


bool
add_item_pcfg(tItem *it) {
  if (it->passive()) {
    if ((opt_packing & PACKING_PCFGEQUI) && packed_edge_pcfg(it))
      return false;
    Chart->add(it);
    type_t root;
    if (it->root(Grammar, Chart->rightmost(), root)) { 
      //
      // a complete result; because we simulate the actual HPSG start symbols
      // as pseudo PCFG rules, ditch the top node, recording its category as a
      // start symbol instead.
      //
      tItem *result = it->daughters().front();
      result->set_result_root(root);
      Chart->trees().push_back(result);
      stats.rtrees ++;
      if (pcfg_result_limits())
        return true;
    } // if
    else {
      postulate_pcfg(it);
      fundamental_for_passive_pcfg(it);
    } // else
  } else {
    Chart->add(it);
    fundamental_for_active_pcfg(dynamic_cast<tPhrasalItem *> (it));
  }
  return false;
}


void
parse_loop_pcfg() {
  //  long memlimit = get_opt_int("opt_memlimit") * 1024 * 1024; 
  //  int pedgelimit = get_opt_int("opt_pedgelimit");
  while (!Agenda->empty() &&
         ! pcfg_resources_exhausted(opt_pedgelimit, opt_memlimit, timeout, timestamp)) {
    basic_task *t = Agenda->pop();
    tItem *it = t->execute();
    delete t;
    if (timeout > 0)
      timestamp = times(NULL);
    // add_item_pcfg checks all limits that have to do with the number
    // of analyses. If return true that means that one of these limits
    // has been reached
    if ((it != 0) && add_item_pcfg(it))
      break;
  }
}

/* TODO this should be a member of tItem
fs instantiate_robust(tItem* root) {
  if (root->trait() != PCFG_TRAIT) {
    return root->get_fs(true);
  } else {
    type_t ruletype = root->rule()->type();
    grammar_rule *rule;
    for (ruleiter iter = Grammar->rules().begin(); 
         iter != Grammar->rules().end(); iter ++) {
      if ((*iter)->type() == ruletype) {
        rule = *iter;
        break;
      }
    }
    
    fs res = rule->instantiate(true);
    list_int *tofill = rule->allargs();  // FIXME, index the real grammar rule using pcfg rule
    vector<tItem*> dtrs;
    for (list<tItem*>::const_iterator iter = root->daughters().begin();
         iter != root->daughters().end(); iter ++)
      dtrs.push_back(*iter);

    while (tofill) {
      fs arg = res.nth_arg(first(tofill));
      res = unify_robust(res, instantiate_robust(dtrs[first(tofill)-1]), arg);
      tofill = rest(tofill);
    }
    return res;
  }
}
*/

int unpack_selectively_pcfg(vector<tItem*> &trees, int upedgelimit,
                            timer *UnpackTime, vector<tItem*> &readings) {
  int nres = 0;
  if (opt_timeout > 0)
    timestamp = times(NULL);

  int oldgplevel = opt_gplevel;
  tSM* oldsm = Grammar->sm();
  Grammar->sm(Grammar->pcfgsm());
  opt_gplevel = 0; 
  list<tItem*> uroots;
  tItem* maxtree = NULL;
  double maxs = 1;
  for (vector<tItem*>::iterator tree = trees.begin();
       tree != trees.end(); ++ tree) {
    if (! (*tree)->blocked()) {
      if (maxs == 1 || (*tree)->score() > maxs) {
        maxs = (*tree)->score();
        maxtree = *tree;
      }
      stats.rtrees ++;
      uroots.push_back(*tree);
    }
  }
  list<tItem*> results;
  if (opt_nsolutions == 1 && maxtree) {
    results.push_back(maxtree);
  } else {
    results = tItem::selectively_unpack(uroots, opt_nsolutions,
                                                   Chart->rightmost(), upedgelimit);
  }
  tCompactDerivationPrinter cdp(cerr, false);
  for (list<tItem*>::iterator res = results.begin();
       res != results.end(); res ++) {
    readings.push_back(*res);
    if (verbosity > 2) {
      cerr << "unpacked[" << nres << "] (" << setprecision(1)
           << ((float) UnpackTime->convert2ms(UnpackTime->elapsed()) / 1000.0)
           << "): ";
      cdp.print(*res);
      cerr << endl;
    }
    nres ++;
  }

  opt_gplevel = oldgplevel;
  Grammar->sm(oldsm);

  return nres;
}


void parse_finish_pcfg(fs_alloc_state &FSAS, list<tError> &errors) {
  // _todo_ modify so that proper unpacking routines are invoked
  //long memlimit = get_opt_int("opt_memlimit") * 1024 * 1024; 
  //int pedgelimit = get_opt_int("opt_pedgelimit");
  long memlimit = opt_memlimit * 1024 * 1024;
  int pedgelimit = opt_pedgelimit;
  clock_t timestamp = (timeout > 0 ? times(NULL) : 0);

  FSAS.clear_stats();
    
  if(pcfg_resources_exhausted(pedgelimit, memlimit, timeout, timestamp)) {
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

  vector<tItem*> readings;

  if ((opt_packing & PACKING_PCFGEQUI) && !(opt_packing & PACKING_NOUNPACK)
      && Grammar->pcfgsm()
      && (pedgelimit == 0 || Chart->pedges() < pedgelimit)) {
    timer *UnpackTime = new timer();
    int nres = 0;
    stats.rtrees = 0;
    int upedgelimit = pedgelimit ? pedgelimit - Chart->pedges() : 0;

    nres = unpack_selectively_pcfg(Chart->trees(), upedgelimit,
                                   UnpackTime, readings);
      
    if (upedgelimit > 0 && stats.p_upedges > upedgelimit) {
      ostringstream s;
      s << "unpack edge limit exhausted (" << upedgelimit 
        << " pedges)";
      errors.push_back(s.str());
    }

    if (opt_timeout > 0 && timestamp >= timeout) {
      ostringstream s;
      s << "timed out (" << opt_timeout / sysconf(_SC_CLK_TCK) 
        << " s)";
      errors.push_back(s.str());
    }
      
    stats.p_utcpu = UnpackTime->convert2ms(UnpackTime->elapsed());
    stats.p_dyn_bytes = FSAS.dynamic_usage();
    stats.p_stat_bytes = FSAS.static_usage();
    FSAS.clear_stats();
    delete UnpackTime;
  } else {
    readings = Chart->trees();
  }

  if (Grammar->pcfgsm())
    sort(readings.begin(), readings.end(), item_greater_than_score());
  
  Chart->readings() = readings;
  stats.rreadings = stats.readings = Chart->readings().size();
}

void analyze_pcfg(chart *&C, fs_alloc_state &FSAS, list<tError> &errors) {

  int nsolutions = opt_nsolutions;
  if(opt_nsolutions <= 0) opt_nsolutions = 1;

  // initialize agenda
  chart_iter ci1(Chart);
  // invalidate the MEM scores
  while (ci1.valid()) {
      ci1.current()->score(1);
      ci1 ++;
  }
  chart_iter ci2(Chart);
  while (ci2.valid()) {
    if (ci2.current()->trait() != INPUT_TRAIT &&
        ci2.current()->passive() && 
        ci2.current()->inflrs_complete_p()) {
      postulate_pcfg(ci2.current());
    }
    ci2 ++;
  }
  
  // main parse loop
  parse_loop_pcfg();

  parse_finish_pcfg(FSAS, errors);

  opt_nsolutions = nsolutions;
}


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

/* tasks */

#include "task.h"
#include "parse.h"
#include "chart.h"
#include "cheap.h"
#include "tsdb++.h"
#include "sm.h"
#include "logging.h"
#include <iomanip>

using namespace std;

// defined in parse.cpp
extern bool opt_hyper;
extern int  opt_packing;

int basic_task::next_id = 0;

tItem *
build_rule_item(chart *C, tAbstractAgenda *A, grammar_rule *R, tItem *passive)
{
    fs_alloc_state FSAS(false);
    
    stats.etasks++;
    
    fs res;
    
    bool temporary = false;
    
    fs rule = R->instantiate();
    fs arg = R->nextarg(rule);
    
    assert(arg.valid());
    
    if(!opt_hyper || R->hyperactive() == false)
    {
        res = unify_restrict(rule,
                             passive->get_fs(),
                             arg,
                             R->arity() == 1 ?
                             Grammar->deleted_daughters() : 0);
    }
    else
    {
        if(R->arity() > 1)
        {
            res = unify_np(rule, passive->get_fs(), arg);
            temporary = true;
        }
        else
        {
            res = unify_restrict(rule, passive->get_fs(), arg,
                                 Grammar->deleted_daughters());
        }
    }
    
    if(!res.valid())
    {
        FSAS.release();
        return 0;
    }
    else
    {
        stats.stasks++;
        
        tPhrasalItem *it;
        
        if(temporary)
        {
            temporary_generation save(res.temp());
            it = new tPhrasalItem(R, passive, res);
            FSAS.release();
        }
        else
        {
            it = new tPhrasalItem(R, passive, res);
        }
        
        return it;
    }
}

tItem *
build_combined_item(chart *C, tItem *active, tItem *passive)
{
    fs_alloc_state FSAS(false);
    
    stats.etasks++;
    
    fs res;
    
    bool temporary = false;
    
    fs combined = active->get_fs();
    
    if(opt_hyper && combined.temp())
        unify_generation = combined.temp(); // _fix_me_ this might need to be reset
    
    fs arg = active->nextarg(combined);
    
    assert(arg.valid());
    
    if(!opt_hyper || active->rule()->hyperactive() == false)
    {
        res = unify_restrict(combined,
                             passive->get_fs(),
                             arg,
                             active->arity() == 1 ? 
                             Grammar->deleted_daughters() : 0); 
    }
    else
    {
        if(active->arity() > 1)
        {
            res = unify_np(combined, passive->get_fs(), arg);
            temporary = true;
        }
        else
        {
            res = unify_restrict(combined, passive->get_fs(), arg,
                                 Grammar->deleted_daughters());
        }
    }
    
    if(!res.valid())
    {
        FSAS.release();
        return 0;
    }
    else
    {
        stats.stasks++;
        
        tPhrasalItem *it;
        
        if(temporary)
        {
            temporary_generation save(res.temp());
            it = new tPhrasalItem(dynamic_cast<tPhrasalItem *>(active),
                                  passive, res);
            FSAS.release();
        }
        else
        {
            it = new tPhrasalItem(dynamic_cast<tPhrasalItem *>(active),
                                  passive, res);
        }
        
        return it;
    }
}

double packingscore(int start, int end, int n, bool active)
{
  return end - double(start) / n
    - (active ? 0.0 : double(start) / n);
  //return end - double(end - start) / n
  //    - (active ? 0.0 : double(end - start) / n) ;
}

rule_and_passive_task::rule_and_passive_task(chart *C, tAbstractAgenda *A,
                                             grammar_rule *R, tItem *passive)
    : basic_task(C, A), _R(R), _passive(passive)
{

  if (Grammar->gm()) {
    double prior = Grammar->gm()->prior(R);
    if (R->arity() == 1) {
      // Priority(R, X) = P(R) P(R->X) P(X)
      std::vector<class tItem *> l;
      l.push_back(passive);
      double conditional = Grammar->gm()->conditional(R, l);
      priority (prior + conditional + passive->gmscore());
    } else {
      // Priority(R, X, ?) = P(R) P(X)
      priority (prior + passive->gmscore());
    }
  } else {
    priority(packingscore(passive->start(), passive->end(),
                          C->rightmost(), R->arity() > 1));
  }
        
  LOG (logChartPruning, DEBUG, "EX MAKE    rule_and_passive: " << id() << " (" << start() << ", " << end() << ") " << _R->printname() << "  " << _p);

}

tItem *
rule_and_passive_task::execute()
{
    if(_passive->blocked())
        return 0;
    
    tItem *result = build_rule_item(_Chart, _A, _R, _passive);
    _A->feedback (this, result);
    if(result) 
    {
      LOG (logChartPruning, DEBUG, "SUCCESS    rule_and_passive: " << id() << " (" << start() << ", " << end() << ") " << _R->printname() << "  " << _p);
      if (Grammar->gm()) {
        if (_R->arity() == 1) {
          // P(R, X) = P(R->X) P(X)
          std::vector<class tItem *> l;
          l.push_back(_passive);
          double conditional = Grammar->gm()->conditional(_R, l);
          result->gmscore(conditional + _passive->gmscore());
        } else {
          // P(R,X,?) = P(X)
          // Well, this result doesn't matter. If a passive results from this new active edge, its score is re-calculated anyway. 
          result->gmscore(_passive->gmscore());
        }
      } else {
        result->gmscore(priority());
      }
    } else {
      LOG (logChartPruning, DEBUG, "EX FAIL    rule_and_passive: " << id() << " (" << start() << ", " << end() << ") " << _R->printname() << "  " << _p);
    }

    return result;
}

active_and_passive_task::active_and_passive_task(chart *C, tAbstractAgenda *A,
                                                 tItem *act, tItem *passive)
    : basic_task(C, A), _active(act), _passive(passive)
{
  if (Grammar->gm()) {
    // Priority(R, X, Y) = P(R) P(R->XY) P(X) P(Y)
    tPhrasalItem *active = dynamic_cast<tPhrasalItem *>(act); 
    double prior = Grammar->gm()->prior(active->rule());
    tItem* active_daughter;
    
    std::vector<class tItem *> l;
    if (active->left_extending()) {
      l.push_back (passive);
      active_daughter = active->daughters().back();
      l.push_back (active_daughter);
    } else {
      active_daughter = active->daughters().front();
      l.push_back (active_daughter);
      l.push_back (passive);
    }
    double conditional = Grammar->gm()->conditional(active->rule(), l);
    priority (prior + conditional + passive->gmscore() + active_daughter->gmscore());
  } else {        
    tPhrasalItem *active = dynamic_cast<tPhrasalItem *>(act); 
    if(active->left_extending()) {
      priority(packingscore(passive->start(), active->end(),
                            C->rightmost(), false));
    } else {
      priority(packingscore(active->start(), passive->end(),
                            C->rightmost(), false));
    }
  }

  LOG (logChartPruning, DEBUG, "MAKE       active_and_passive: " << id() << " ("
                                                    << _active->start()  << ", " << _active->end()  << ")  (" 
                                                    << _passive->start() << ", " << _passive->end() << ")  "
                                                    << _active->rule()->printname() << "  " << _p );
}

tItem *
active_and_passive_task::execute()
{
    if(_passive->blocked() || _active->blocked())
        return 0;
    
    tItem *result = build_combined_item(_Chart, _active, _passive);
    _A->feedback (this, result);
    if(result) 
    {
      LOG (logChartPruning, DEBUG, "EX SUCCESS active_and_passive: " << id() << " ("
                                                           << _active->start()  << ", " << _active->end()  << ")  (" 
                                                           << _passive->start() << ", " << _passive->end() << ")  "
                                                           << _active->rule()->printname() << "  " << _p );
      if (Grammar->gm()) {
        std::vector<class tItem *> l;
        tItem* active_daughter;
        if (_active->left_extending()) {
          l.push_back (_passive);
          active_daughter = _active->daughters().back();
          l.push_back (active_daughter);
        } else {
          active_daughter = _active->daughters().front();
          l.push_back (active_daughter);
          l.push_back (_passive);
        }
        double conditional = Grammar->gm()->conditional(_active->rule(), l);
        result->gmscore(conditional + active_daughter->gmscore() + _passive->gmscore());
      } else {
        result->gmscore(priority());
      }
    } else {
      LOG (logChartPruning, DEBUG, "EX FAIL    active_and_passive: " << id() << " ("
                                                        << _active->start()  << ", " << _active->end()  << ")  (" 
                                                        << _passive->start() << ", " << _passive->end() << ")  "
                                                        << _active->rule()->printname() << "  " << _p );
    }

    return result;
}

void
basic_task::print(std::ostream &out)
{
  out << "task #" << _id << " (" << std::setprecision(2) << _p << ")";
}

void
rule_and_passive_task::print(std::ostream &out)
{
  out << "task #" << _id << " {" << _R->printname() << " + "
      << _passive->id() << "} (" << std::setprecision(2) << _p << ")";
}

void
active_and_passive_task::print(std::ostream &out)
{
  out << "task #" << _id << " {" << _active->id() << " + " << _passive->id() 
      << "} (" << std::setprecision(2) << _p << ")";
}

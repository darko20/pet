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
#include <fstream>
#include <iostream> 

using namespace std;

// defined in parse.cpp
extern bool opt_hyper;
extern int  opt_packing;

int basic_task::next_id = 0;

/*
vector<int> basic_task::_spans;
ofstream basic_task::_spans_outfile ("spans.txt");
*/

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

  
  // Original setting: 
  //return end - double(start) / n
  //  - (active ? 0.0 : double(start) / n);
    
  // Originally commented out: 
   //return end - double(end - start) / n
   //    - (active ? 0.0 : double(end - start) / n) ;
  
  // Adaptation 1: 
  return double(end) - double(start) + (active ? 0.0 : 0.5);

  // Adaptation 1b: 
  //return double(end) - double(start) + (active ? 1.5 : 0.0);
  
  // Adaptation 2: 
  //return end - start + (double(start) + active ? 0.0 : 0.5) / n;

}

rule_and_passive_task::rule_and_passive_task(chart *C, tAbstractAgenda *A,
                                             grammar_rule *R, tItem *passive)
    : basic_task(C, A), _R(R), _passive(passive)
{
    if(opt_packing) {
        if (Grammar->gm()) {
          double prior = Grammar->gm()->prior(R);
          if (R->arity() == 1) {
            // Priority(R, X) = P(R) P(R->X) P(X)
            std::vector<class tItem *> l;
            l.push_back(passive);
            double conditional = Grammar->gm()->conditional(R, l);
            priority (prior + conditional + passive->score());
          } else {
            // Priority(R, X, ?) = P(R) P(X) * penalty
            // The heavy penalty will make sure that tasks leading to passive items will always be preferred over task leading to active items. 
            priority (prior + passive->score() - 1000.0);
          }
        } else {
          priority(packingscore(passive->start(), passive->end(),
                                C->rightmost(), R->arity() > 1));
        }
    }
    else if(Grammar->sm())
    {
        list<tItem *> daughters;
        daughters.push_back(passive);

        priority(Grammar->sm()->scoreLocalTree(R, daughters));
    }
}

tItem *
rule_and_passive_task::execute()
{
    if(_passive->blocked())
        return 0;

    /*
    if (_R->arity() == 1) {
      basic_task::_spans.push_back (_passive->end() - _passive->start());
    }
    */
    
    tItem *result = build_rule_item(_Chart, _A, _R, _passive);
    if(result) 
    {
      if (Grammar->gm()) {
        if (_R->arity() == 1) {
          // P(R, X) = P(R->X) P(X)
          std::vector<class tItem *> l;
          l.push_back(_passive);
          double conditional = Grammar->gm()->conditional(_R, l);
          result->score(conditional + _passive->score());
        } else {
          // P(R,X,?) = P(X)
          // Well, this result doesn't matter. If a passive results from this new active edge, its score is re-calculated anyway. 
          result->score(_passive->score());
        }
      } else {
        result->score(priority());
      }
      /*
      if (_R->arity() == 1)
      { 
        // Passive unary rule.
        LOG (logGM, DEBUG, fixed << setprecision(2)); 
        LOG (logGM, DEBUG, result->id() << " (" << result->start() << ", " << result->end() << ") " << 
                           result->identity() << "  " << result->printname() << "  " << priority());
        item_list l = result->daughters();
        tItem *it = l.front();
        tLexItem *lit = dynamic_cast<tLexItem*> (it);
        if (lit != 0) {
          LOG (logGM, DEBUG, "    " << it->id() << " (" << it->start() << ", " << it->end() << ") " << 
                             it->identity() << "  " << get_typename(it->identity()) << "  " << it->score());
        } else {
          LOG (logGM, DEBUG, "    " << it->id() << " (" << it->start() << ", " << it->end() << ") " << it->identity() << "  " << it->printname() << "  " << it->score());
        }
      }
      */
    }
    
    return result;
}



active_and_passive_task::active_and_passive_task(chart *C, tAbstractAgenda *A,
                                                 tItem *act, tItem *passive)
    : basic_task(C, A), _active(act), _passive(passive)
{
    if(opt_packing) {
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
          
          priority (prior + conditional + passive->score() + active_daughter->score());
        } else {        
          tPhrasalItem *active = dynamic_cast<tPhrasalItem *>(act); 
          if(active->left_extending())
              priority(packingscore(passive->start(), active->end(),
                                    C->rightmost(), false));
          else
              priority(packingscore(active->start(), passive->end(),
                                    C->rightmost(), false));
        
        }    
    }
    else if(Grammar->sm())
    {
        tPhrasalItem *active = dynamic_cast<tPhrasalItem *>(act); 

        list<tItem *> daughters(active->daughters());

        if(active->left_extending())
            daughters.push_front(passive);
        else
            daughters.push_back(passive);

        priority(Grammar->sm()->scoreLocalTree(active->rule(), daughters));
    }
}

tItem *
active_and_passive_task::execute()
{
    if(_passive->blocked() || _active->blocked())
        return 0;

    /*
    if (_active->left_extending())
    {
      basic_task::_spans.push_back (_active->end() - _passive->start());
    } else {
      basic_task::_spans.push_back (_passive->end() - _active->start());
    }
    */
    
    tItem *result = build_combined_item(_Chart, _active, _passive);
    if(result) 
    {
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
        result->score(conditional + active_daughter->score() + _passive->score());
      } else {
        result->score(priority());
      }

      /*
      if (result->rule()->arity() == 2)
      { 
        // Passive binary rule. 
        LOG (logGM, DEBUG, fixed << setprecision(2));
        LOG (logGM, DEBUG, result->id() << " (" << result->start() << ", " << result->end() << ") " << result->identity() << "  " << result->printname() << "  " << priority());
        item_list l = result->daughters();
        tItem *it = l.front();
        tLexItem *lit = dynamic_cast<tLexItem*> (it);
        if (lit != 0) {
          LOG (logGM, DEBUG, "    " << it->id() << " (" << it->start() << ", " << it->end() << ") " << 
                             it->identity() << "  " << get_typename(it->identity()) << "  " << it->score());
        } else {
          LOG (logGM, DEBUG, "    " << it->id() << " (" << it->start() << ", " << it->end() << ") " << 
                             it->identity() << "  " << it->printname() << "  " << it->score());
        }
        it = l.back();
        lit = dynamic_cast<tLexItem*> (it);
        if (lit != 0) {
          LOG (logGM, DEBUG, "    " << it->id() << " (" << it->start() << ", " << it->end() << ") " << 
                             it->identity() << "  " << get_typename(it->identity()) << "  " << it->score());
        } else {
          LOG (logGM, DEBUG, "    " << it->id() << " (" << it->start() << ", " << it->end() << ") " << 
                             it->identity() << "  " << it->printname() << "  " << it->score());
        }
      }
      */
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

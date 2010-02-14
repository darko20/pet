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
    if(opt_packing)
    {

        double prior = Grammar->pcfgsm()->getPrior(R->type());
        double conditional;
        if (R->arity() == 1) {
          // Unary case: add prior, passive score and conditional 
          std::list<class tItem *> l;
          l.push_back(passive);
          conditional = Grammar->pcfgsm()->scoreLocalTree(R, l);
        } else {
          // Binary case: add prior and passive score (conditional computed when rule is completed. 
          conditional = 0.0;
        }
        priority (passive->score() + prior + conditional);

        // BaC: how it was before non-exhaustive parsing. 
        //priority(packingscore(passive->start(), passive->end(),
        //                      C->rightmost(), R->arity() > 1));
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
      result->score(priority());
      if (_R->arity() == 1)
      { 
        // Passive unary rule. 
        cerr << result->id() << " (" << result->start() << ", " << result->end() << ") " << result->printname() << "  " << priority() << '\n';
        item_list l = result->daughters();
        tItem *it = l.front();
        cerr << "    " << it->id() << " (" << it->start() << ", " << it->end() << ") " << it->printname() << "  " << priority() << '\n';
      }
    }
    
    return result;
}

active_and_passive_task::active_and_passive_task(chart *C, tAbstractAgenda *A,
                                                 tItem *act, tItem *passive)
    : basic_task(C, A), _active(act), _passive(passive)
{
    if(opt_packing)
    {	
        tPhrasalItem *active = dynamic_cast<tPhrasalItem *>(act); 

        std::list<class tItem *> l;
        if (active->left_extending()) {
          l.push_back (passive);
          l.push_back (active);
        } else {
          l.push_back (active);
          l.push_back (passive);
        }
        double conditional = Grammar->pcfgsm()->scoreLocalTree(active->rule(), l);

        // The active item already contains the prior of the rule. 
        // To add: the score of the passive, and the conditional. 
        priority (active->score() + passive->score() + conditional);
        
        
        // BaC: how it was before non-exhaustive parsing. 
        /*
        tPhrasalItem *active = dynamic_cast<tPhrasalItem *>(act); 
        if(active->left_extending())
            priority(packingscore(passive->start(), active->end(),
                                  C->rightmost(), false));
        else
            priority(packingscore(active->start(), passive->end(),
                                  C->rightmost(), false));
        */
        
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
    if(result) {
      result->score(priority());    
      if (result->rule()->arity() == 2)
      { 
        // Passive binary rule. 
        cerr << result->id() << " (" << result->start() << ", " << result->end() << ") " << result->printname() << "  " << priority() << '\n';
        item_list l = result->daughters();
        tItem *it = l.front();
        cerr << "    " << it->id() << " (" << it->start() << ", " << it->end() << ") " << it->printname() << "  " << priority() << '\n';
        it = l.back();
        cerr << "    " << it->id() << " (" << it->start() << ", " << it->end() << ") " << it->printname() << "  " << priority() << '\n';
      }
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

/* -*- Mode: C++ -*- 
 * PET
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

/** \file task.h
 * Task class hierarchy.
 */

#ifndef _TASK_H_
#define _TASK_H_

#include "agenda.h"
#include "item.h"
#include "grammar.h"
#include <functional>
#include <iosfwd>

#include <fstream>
#include <vector>


/** Parser agenda: a queue of prioritized tasks */
typedef abstract_agenda< class basic_task, class task_priority_less > tAbstractAgenda;
typedef exhaustive_agenda< class basic_task, class task_priority_less > tExhaustiveAgenda;
typedef global_cap_agenda< class basic_task, class task_priority_less > tGlobalCapAgenda;
typedef local_cap_agenda< class basic_task, class task_priority_less > tLocalCapAgenda;
typedef striped_cap_agenda< class basic_task, class task_priority_less > tStripedCapAgenda;

/** Pure virtual base class for tasks */
class basic_task {
public:
  virtual ~basic_task() {}

  /** ID counter to produce unique task ids */
  static int next_id;

  /** Base constructor */
  inline basic_task(class chart *C, tAbstractAgenda *A) 
    : _id(next_id++), _Chart(C), _A(A), _p(0.0)
  {}

  /** Execute the task */
  virtual class tItem * execute() = 0;
  
  /* Return ID counter */
  inline int id() {return _id;}
    
  /** Return the priority of this task. 
   *
   * The priority is the sorting criterion for tasks. The task with the
   * highest priority is executed first.
   */
  inline double priority() const
  { return _p; }
  
  /** Set the priority to \a p */
  inline void priority(double p)
  { _p = p; }

  /** Return start and end positions of the possibly resulting edge. */
  virtual int start () = 0;
  virtual int end () = 0;
  virtual bool phrasal () = 0;

  /** Print task readably to \a f for debugging purposes */
  virtual void print(std::ostream &out);
  
 /*
  static std::vector<int> _spans;
  static std::ofstream _spans_outfile;
  static void write_spans () {
    for (std::vector<int>::iterator iter = _spans.begin(); iter != _spans.end(); iter++) {
      _spans_outfile << *iter << " ";
    }
    _spans_outfile << std::endl;
    _spans.clear();
  };
  */


protected:
  /** Unique task id */
  int _id;
    
  /** The chart this task operates on */
  class chart *_Chart;
  /** The agenda this task came from */
  tAbstractAgenda *_A;
  
  /** The priority of this task */
  double _p;
    
  friend class task_priority_less;
};

/** Combination of grammar rule and passive item */
class rule_and_passive_task : public basic_task
{
 public:
    virtual ~rule_and_passive_task() {}

    /** Create a task with a grammar rule and passive item that will be
     *  executed later on.
     */
    rule_and_passive_task(class chart *C, tAbstractAgenda *A,
                          class grammar_rule *R, class tItem *passive);
    
    /** See basic_task::execute() */
    virtual class tItem *execute();

    /** Return start and end positions of the possibly resulting edge. */
    int start () { return _passive->start();}
    int end ()   { return _passive->end();}
    inline bool phrasal () { return _R->trait() == SYNTAX_TRAIT;}
    
    /** See basic_task::print() */
    virtual void print(std::ostream &out);
    
 private:
    grammar_rule *_R;
    class tItem *_passive;
};

/** Combination of active and passive item */
class active_and_passive_task : public basic_task
{
 public:
    virtual ~active_and_passive_task() {}

    /** Create a task with an active and a passive item that will be
     *  executed later on.
     */
    active_and_passive_task(class chart *C, tAbstractAgenda *A,
                            class tItem *active, class tItem *passive);

    /** See basic_task::execute() */
    virtual tItem *execute();

    /** Return start and end positions of the possibly resulting edge. */
    int start () { return std::min(_passive->start(), _active->start());}
    int end ()   { return std::min(_passive->end(), _active->end());}
    inline bool phrasal () { return false;}

    /** See basic_task::print() */
    virtual void print(std::ostream &out);

 private:
    class tItem *_active;
    class tItem *_passive;
};

/** Comparison predicate for tasks based on their priority */
class task_priority_less 
  : public std::binary_function<basic_task *, basic_task *, bool>
{
 public:
    /** \a x is less than \a y if \a x's priority is less than \a y's */
    inline bool
    operator() (const basic_task* x, const basic_task* y) const
    {
        return x->priority() < y->priority();
    }
};

inline std::ostream & operator<<(std::ostream &out, basic_task *t) {
  t->print(out); return out;
}

#endif

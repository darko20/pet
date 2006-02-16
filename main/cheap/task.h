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
#include <functional>

/** Parser agenda: a queue of prioritized tasks */
typedef agenda< class basic_task, class task_priority_less > tAgenda;

/** Pure virtual base class for tasks */
class basic_task {
public:
  virtual ~basic_task() {}

  /** ID counter to produce unique task ids */
  static int next_id;

  /** Base constructor */
  inline basic_task(class chart *C, tAgenda *A) 
    : _id(next_id++), _Chart(C), _A(A), _p(0.0)
  {}

  /** Execute the task */
  virtual class tItem * execute() = 0;
    
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

  /** Print task readably to \a f for debugging purposes */
  virtual void print(FILE *f);

protected:
  /** Unique task id */
  int _id;
    
  /** The chart this task operates on */
  class chart *_Chart;
  /** The agenda this task came from */
  tAgenda *_A;
  
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
    rule_and_passive_task(class chart *C, tAgenda *A,
                          class grammar_rule *R, class tItem *passive);
    
    /** See basic_task::execute() */
    virtual class tItem *execute();
    /** See basic_task::print() */
    virtual void print(FILE *f);
    
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
    active_and_passive_task(class chart *C, tAgenda *A,
                            class tItem *active, class tItem *passive);

    /** See basic_task::execute() */
    virtual tItem *execute();
    /** See basic_task::print() */
    virtual void print(FILE *f);

 private:
    class tItem *_active;
    class tItem *_passive;
};

/** Comparison predicate for tasks based on their priority */
class task_priority_less 
: public binary_function<basic_task *, basic_task *, bool>
{
 public:
    /** \a x is less than \a y if \a x's priority is less than \a y's */
    inline bool
    operator() (const basic_task* x, const basic_task* y) const
    {
        return x->priority() < y->priority();
    }
};

#endif

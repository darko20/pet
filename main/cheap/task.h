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

/* task class hierarchy */

#ifndef _TASK_H_
#define _TASK_H_

#include "grammar.h"
#include "item.h"

// _fix_me_ required for global Grammar only
#include "parse.h"

class basic_task
{
 public:
    static int next_id;

    inline basic_task(class chart *C, class tAgenda *A) 
        : _id(next_id++), _Chart(C), _A(A), _p(0.0)
    {}

    virtual tItem * execute() = 0;
    
    inline double priority() const
    { return _p; }
    
    inline void priority(double p)
    { _p = p; }

    virtual void print(FILE *f);

 protected:
    int _id;
    
    class chart *_Chart;
    class tAgenda *_A;
  
    double _p;
    
    friend class task_priority_less;
};

class item_task : public basic_task
{
 public:
    item_task(class chart *C, class tAgenda *A, tItem *it);
    
    virtual tItem *execute();

 private:
    tItem *_item;
};

class rule_and_passive_task : public basic_task
{
 public:
    rule_and_passive_task(class chart *C, class tAgenda *A,
                          grammar_rule *R, tItem *passive);
    
    virtual tItem *execute();
    virtual void print(FILE *f);
    
 private:
    grammar_rule *_R;
    tItem *_passive;
};

class active_and_passive_task : public basic_task
{
 public:
    active_and_passive_task(class chart *C, class tAgenda *A,
                            tItem *active, tItem *passive);

    virtual tItem *execute();
    virtual void print(FILE *f);

 private:
    tItem *_active;
    tItem *_passive;
};

class task_priority_less
{
 public:
    inline bool
    operator() (const basic_task* x, const basic_task* y) const
    {
        return x->priority() < y->priority();
    }
};

#endif

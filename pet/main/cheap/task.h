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
    basic_task(class chart *C, class agenda *A) 
        : _id(next_id++), _C(C), _A(A), _p(0.0)
    {}

    virtual item *execute() = 0;
    
    double score(grammar_rule *rule, list<item *> &daughters, tSM *model);

    inline double priority() { return _p; }
    inline void priority(double p) { _p = p; }

    virtual void print(FILE *f);

 protected:
    int _id;
    
    class chart *_C;
    class agenda *_A;
  
    // priority
    double _p;
    
    friend class task_priority_less;
};

class item_task : public basic_task
{
 public:
    inline item_task(class chart *C, class agenda *A, item *it)
        : basic_task(C, A), _item(it)
    {
        _p = it->priority();
    }
    
    virtual item *execute();
 private:
    item *_item;
};

class rule_and_passive_task : public basic_task
{
 public:
    inline rule_and_passive_task(class chart *C, class agenda *A,
                                 grammar_rule *R, item *passive)
        : basic_task(C, A), _R(R), _passive(passive)
    {

        list<item *> daughters;
        daughters.push_back(passive);
        score(R, daughters, Grammar->sm());
    }
    
    virtual item *execute();
    virtual void print(FILE *f);
    
 private:
    grammar_rule *_R;
    item *_passive;
};

class active_and_passive_task : public basic_task
{
 public:
    inline active_and_passive_task(class chart *C, class agenda *A,
                                   item *active, item *passive)
        : basic_task(C, A), _active(active), _passive(passive)
    {
        list<item *> daughters(dynamic_cast<phrasal_item *>(active)->
                               _daughters);
        daughters.push_back(passive);
        score(active->rule(), daughters, Grammar->sm());
    }
    virtual item *execute();
 private:
    item *_active;
    item *_passive;
};

class task_priority_less
{
 public:
    inline bool
    operator() (const basic_task* x, const basic_task* y) const
    {
        return x->_p < y->_p;
    }
};

#endif

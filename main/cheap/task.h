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

#define MAX_TASK_PRIORITY 64000

class basic_task
{
 public:
  static int next_id;
  basic_task(class chart *C, class agenda *A,
	     int p, int q = 0, int r = 0, int s = 0, int t = 0) 
    : _id(next_id++), _C(C), _A(A), _p(p), _q(q), _r(r), _s(s), _t(t)
    {}

  virtual item *execute() = 0;

  inline int priority() { return _p; }
  inline void priority(int p) { _p = p; }

  virtual void print(FILE *f);

 protected:
  int _id;

  class chart *_C;
  class agenda *_A;
  
  // priorities
  int _p;
  int _q;
  int _r;
  int _s;
  int _t;

  friend class task_priority_less;
};

class item_task : public basic_task
{
 public:
  inline item_task(class chart *C, class agenda *A, item *it)
    : basic_task(C, A, it->priority(), MAX_TASK_PRIORITY, it->age()), _item(it) {}
  inline item_task(class chart *C, class agenda *A, item *it, int p)
    : basic_task(C, A, p, MAX_TASK_PRIORITY, it->age()), _item(it) {}
  virtual item *execute();
 private:
  item *_item;
};

class rule_and_passive_task : public basic_task
{
 public:
  inline rule_and_passive_task(class chart *C, class agenda *A,
			       grammar_rule *R, item *passive)
    : basic_task(C, A, 
                 R->priority(passive->priority()), 
                 passive->priority(), passive->age()), 
      _R(R), _passive(passive) {}
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
    : basic_task(C, A, active->priority(), 
                 passive->priority(), passive->age(),
                 active->qriority(), active->age()),
      _active(active), _passive(passive) {}
  virtual item *execute();
 private:
  item *_active;
  item *_passive;
};

class task_priority_less
{
 public:
  inline bool operator() (const basic_task* x, const basic_task* y) const
    {
      if(x->_p == y->_p)
        if(x->_q == y->_q) 
          if(x->_r == y->_r) 
            if(x->_s == y->_s) return x->_t > y->_t;
            else return x->_s < y->_s;
          else return x->_r > y->_r;
        else return x->_q < y->_q;
      else return x->_p < y->_p;
    }
};

#endif

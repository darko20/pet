/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* task class hierarchy */

#ifndef _TASK_H_
#define _TASK_H_

#include "grammar.h"
#include "item.h"

class basic_task
{
 public:
  static int next_id;
  basic_task(int p, int q = 0, int r = 0, int s = 0, int t = 0) 
    : _p(p), _q(q), _r(r), _s(s), _t(t), _id(next_id++) {}

  virtual item *execute() = 0;

  inline int priority() { return _p; }

  virtual void print(FILE *f);

  // private:
 protected:
  int _p;
  int _q;
  int _r;
  int _s;
  int _t;
  int _id;

  friend class task_priority_less;
};

class item_task : public basic_task
{
 public:
  inline item_task(item *it)
    : basic_task(it->priority(), it->priority(), it->id()), _item(it) {}
  inline item_task(item *it, int p)
    : basic_task(p, p, it->id()), _item(it) {}
  virtual item *execute();
 private:
  item *_item;
};

class rule_and_passive_task : public basic_task
{
 public:
  inline rule_and_passive_task(grammar_rule *R, item *passive)
    : basic_task(R->priority(), passive->priority(), passive->id()), 
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
  inline active_and_passive_task(item *active, item *passive)
    : basic_task(active->priority(), 
                 passive->priority(), passive->id(),
                 active->qriority(), active->id()),
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

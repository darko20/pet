/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* agenda */

#ifndef _AGENDA_H_
#define _AGENDA_H_

// #define SIMPLE_AGENDA

#ifdef SIMPLE_AGENDA
#include <list>
#else
#include <queue>
#endif

#include "task.h"

class agenda
{
 public:

  agenda() : _A() {}
  ~agenda() { while(!this->empty()) delete this->pop(); }

  void push(basic_task *t);
  basic_task *pop();
  inline bool empty() { return _A.empty(); }

 private:
  
#ifdef SIMPLE_AGENDA
  std::list<basic_task *> _A;
#else
  std::priority_queue<basic_task *, vector<basic_task *>, task_priority_less> _A;
#endif

};

#endif

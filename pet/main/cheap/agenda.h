/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* agenda */

#ifndef _AGENDA_H_
#define _AGENDA_H_

#include "task.h"

class agenda
{
 public:

  agenda() : _A() {}
  agenda(agenda &a) : _A(a._A) {}
  ~agenda() { while(!this->empty()) delete this->pop(); }

  
  void push(basic_task *t);
  basic_task *top();
  basic_task *pop();

  inline bool empty() { return _A.empty(); }

 private:

  std::priority_queue<basic_task *, vector<basic_task *>, task_priority_less> _A;
};

#endif

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

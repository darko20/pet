/* -*- Mode: C++ -*-
 * Pet
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2003 Ulrich Callmeier uc@coli.uni-sb.de
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

/** Agenda: a priority queue adapter */

#ifndef _AGENDA_H_
#define _AGENDA_H_

#include <queue>
#include <vector>

#include "item.h"
#include "options.h"
#include "task.h"


/** agenda: a priority queue adapter */
template <typename T, typename LESS_THAN > class abstract_agenda {
public:

  virtual ~abstract_agenda() {}

  virtual void push(T *t) = 0;
  virtual T * top() = 0;
  virtual T * pop() = 0;     // Responsibility to delete the task is for the caller. 
  virtual bool empty() = 0;
  virtual void feedback (T *t, tItem *result) = 0; 
  
};
    

template <typename T, typename LESS_THAN > class exhaustive_agenda : public abstract_agenda<T, LESS_THAN > {
public : 

  exhaustive_agenda() : _A() {}
  ~exhaustive_agenda() { while(!this->empty()) delete this->pop(); }

  void push(T *t) { _A.push(t); }
  T * top()       { return _A.top(); }
  T * pop()       { T *t = top(); _A.pop(); return t; }
  bool empty() { return _A.empty(); }
  void feedback (T *t, tItem *result) {}

private:

  std::priority_queue<T *, std::vector<T *>, LESS_THAN> _A;
};





/*
 * LOCAL CAP AGENDA
 */

template <typename T, typename LESS_THAN > class local_cap_agenda : public abstract_agenda<T, LESS_THAN > {
/* This class provides functionality to define a per-cell cap on the number of tasks to be executed. */

public : 

  local_cap_agenda(int cell_size, int max_pos) : _A(), _popped((max_pos+1)*(max_pos+1)), _max_pos(max_pos), _cell_size(cell_size),
                                                 _exec((max_pos+1)*(max_pos+1)), _succ((max_pos+1)*(max_pos+1)), _pass((max_pos+1)*(max_pos+1)) { 
  }
  ~local_cap_agenda();
  
  void push(T *t) {
    _A.push(t); 
  }
  T * top();
  T * pop(); 
  bool empty() { return top() == NULL; }
  void feedback (T *t, tItem *result);

private:

  std::priority_queue<T *, std::vector<T *>, LESS_THAN> _A;
  std::vector<int> _popped;
  int _max_pos;
  int _cell_size;
  
  std::vector<int> _exec;
  std::vector<int> _succ;
  std::vector<int> _pass; 
};


/*
 * (implementation here, due to use of templates)
 */

template <typename T, class LESS_THAN>
local_cap_agenda<T, LESS_THAN>::~local_cap_agenda() {
  while (!_A.empty()) {
    T* t = _A.top();
    delete t;
    _A.pop();
  }
}

template <typename T, class LESS_THAN>
T * local_cap_agenda<T, LESS_THAN>::top() {
  T* t;
  bool found = false;
  while (!found) {
    if (!_A.empty()) {
      t = _A.top();
      if (t->phrasal() && _popped[t->start()*(_max_pos+1) + t->end()] >= _cell_size) {
        // This span reached the limit, so continue searching for a new task. 
        // Inflectional and lexical rules are always carried out. 
        delete t;
        _A.pop();
      } else {
        found = true;
      }
    } else {
      t = NULL;
      break;
    }
  }
  return t;
}

template <typename T, class LESS_THAN>
T * local_cap_agenda<T, LESS_THAN>::pop() { 
  T *t = top(); 
  if (t != NULL) { 
    _A.pop(); 
  }
  return t; 
}

template <typename T, class LESS_THAN>
void local_cap_agenda<T, LESS_THAN>::feedback (T *t, tItem *result) { 
  if (t->phrasal()) {
    if (get_opt_int("opt_chart_pruning_strategy") == 0) {
      _popped[t->start()*(_max_pos+1) + t->end()]++;
    } else if (get_opt_int("opt_chart_pruning_strategy") == 1 && (result != 0)) {
      _popped[t->start()*(_max_pos+1) + t->end()]++;
    } else if (get_opt_int("opt_chart_pruning_strategy") == 2 && (result != 0) && t->yields_passive()) {
      _popped[t->start()*(_max_pos+1) + t->end()]++;
    }    
  }
}


#endif

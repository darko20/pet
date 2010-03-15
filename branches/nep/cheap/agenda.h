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
#include <set>
#include <utility>
#include <map>

#include "logging.h"

/** agenda: a priority queue adapter */
template <typename T, typename LESS_THAN > class abstract_agenda {
public:

  virtual ~abstract_agenda() {}
  
  virtual void push(T *t) = 0;
  virtual T * top() = 0;
  virtual T * pop() = 0;     // Responsibility to delete the task is for the caller. 
  virtual bool empty() = 0;

};


template <typename T, typename LESS_THAN > class exhaustive_agenda : public abstract_agenda<T, LESS_THAN > {
public : 

  exhaustive_agenda() : _A() {}
  ~exhaustive_agenda() { while(!this->empty()) delete this->pop(); }
  
  void push(T *t) { _A.push(t); }
  T * top() { return _A.top(); }
  T * pop() { T *t = top(); _A.pop(); return t; }
  bool empty() { return _A.empty(); }
  
private:

  std::priority_queue<T *, std::vector<T *>, LESS_THAN> _A;
};




/* 
 * GLOBAL CAP
 */

template <typename T, typename LESS_THAN > class global_cap_agenda : public abstract_agenda<T, LESS_THAN > {
/* This class provides functionality to define a global cap on the number of tasks to be executed. */

public : 

  global_cap_agenda(int cell_size, int max_pos) : _A(), _popped(), _max_popped(max_pos*cell_size*(cell_size+1)/2) {}
  ~global_cap_agenda(); 
  
  void push(T *t) { _A.push(t); }
  T * top();
  T * pop(); 
  bool empty() { return top() == NULL; }

private:

  std::priority_queue<T *, std::vector<T *>, LESS_THAN> _A;
  int _popped;
  int _max_popped;
};


template <typename T, class LESS_THAN>
global_cap_agenda<T, LESS_THAN>::~global_cap_agenda() {
  while (!_A.empty()) {
    T* t = _A.top();
    delete t;
    _A.pop();
  }
}

template <typename T, class LESS_THAN>
T * global_cap_agenda<T, LESS_THAN>::top() {
  T* t;
  bool found = false;
  while (!found) {
    if (!_A.empty()) {
      t = _A.top();
      if (t->end() - t->start() > 1 && _popped >= _max_popped) {
        // This span reached the limit, so continue searching for a new task. 
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
T * global_cap_agenda<T, LESS_THAN>::pop() { 
  T *t = top(); 
  if (t != NULL) { 
    _A.pop(); 
    _popped++;
  }
  return t; 
}


/*
 * STRIPED CAP AGENDA  
 */

template <typename T, typename LESS_THAN > class striped_cap_agenda : public abstract_agenda<T, LESS_THAN > {
/* This class provides functionality to define a per span length cap on the number of tasks to be executed. */

public : 

  striped_cap_agenda(int cell_size, int max_pos) : _A(), _popped(), _cell_size(cell_size) {}
  ~striped_cap_agenda();
  
  void push(T *t) { _A.push(t); }
  T * top();
  T * pop(); 
  bool empty() { return top() == NULL; }

private:

  std::priority_queue<T *, std::vector<T *>, LESS_THAN> _A;
  std::map<int, int> _popped;
  int _cell_size;
  int _span; 
};

template <typename T, class LESS_THAN>
striped_cap_agenda<T, LESS_THAN>::~striped_cap_agenda() {
  while (!_A.empty()) {
    T* t = _A.top();
    delete t;
    _A.pop();
  }
}

template <typename T, class LESS_THAN>
T * striped_cap_agenda<T, LESS_THAN>::top() {
  T* t;
  bool found = false;
  while (!found) {
    if (!_A.empty()) {
      t = _A.top();
      _span = t->end() - t->start(); 
      if (_span > 1 && _popped[_span] >= _cell_size*_span) {
        // This span reached the limit, so continue searching for a new task. 
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
T * striped_cap_agenda<T, LESS_THAN>::pop() { 
  T *t = top(); 
  if (t != NULL) { 
    _A.pop(); 
    _popped[t->end()-t->start()]++;
  }
  return t; 
}



/*
 * LOCAL CAP AGENDA
 */

template <typename T, typename LESS_THAN > class local_cap_agenda : public abstract_agenda<T, LESS_THAN > {
/* This class provides functionality to define a per-cell cap on the number of tasks to be executed. */

public : 

  local_cap_agenda(int cell_size, int max_pos) : _A(), _popped(), _cell_size(cell_size) {}
  ~local_cap_agenda();
  
  void push(T *t) {
    _A.push(t); 
  }
  T * top();
  T * pop(); 
  bool empty() { return top() == NULL; }

private:

  std::priority_queue<T *, std::vector<T *>, LESS_THAN> _A;
  std::map< std::pair<int,int>, int> _popped;
  int _cell_size;
};

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
      if (t->end() - t->start() > 1 && _popped[std::pair<int,int>(t->start(), t->end())] >= _cell_size) {
        // This span reached the limit, so continue searching for a new task. 
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
    _popped[std::pair<int,int>(t->start(), t->end())]++;
  }
  return t; 
}

#endif

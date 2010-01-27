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

/** agenda: a priority queue adapter */
template <typename T, typename LESS_THAN > class abstract_agenda {
public:

  virtual ~abstract_agenda() {}
  
  virtual void push(T *t) = 0;
  virtual T * top() = 0;
  virtual T * pop() = 0;
  virtual bool empty() = 0;
  virtual int size() = 0; 

};


template <typename T, typename LESS_THAN > class exhaustive_agenda : public abstract_agenda<T, LESS_THAN > {
public : 

  exhaustive_agenda() : _A() {}
  ~exhaustive_agenda() { while(!this->empty()) delete this->pop(); }
  
  void push(T *t) { _A.push(t); }
  T * top() { return _A.top(); }
  T * pop() { T *t = top(); _A.pop(); return t; }
  bool empty() { return _A.empty(); }
  int size() { return _A.size();}
  
private:

  std::priority_queue<T *, std::vector<T *>, LESS_THAN> _A;
};

template <typename T, typename LESS_THAN > class global_cap_agenda : public abstract_agenda<T, LESS_THAN > {
/* This class provides functionality to define a global cap on the number of tasks to be executed. */

public : 

  global_cap_agenda(int max_popped) : _A(), _max_popped(max_popped) {_popped = 0; }
  ~global_cap_agenda() {make_empty(); }
  
  void push(T *t) { _A.push(t); }
  T * top() { return _A.top();}
  T * pop(); 
  bool empty() { return _A.empty(); }
  int size() { return _A.size();}

private:

  std::priority_queue<T *, std::vector<T *>, LESS_THAN> _A;
  int _max_popped;
  int _popped;
  
  void make_empty () { while(!this->empty()) delete this->pop(); }

};

template <typename T, class LESS_THAN>
T * global_cap_agenda<T, LESS_THAN>::pop()
{ 
  T *t = top(); 
  _A.pop(); 
  _popped++; 
  if (_popped >= _max_popped) make_empty();
  return t; 
}


template <typename T, typename LESS_THAN > class global_beam_agenda : public abstract_agenda<T, LESS_THAN > {
/* This class provides functionality to define a global number of tasks to be held at the same time. */

public :
  global_beam_agenda (int beam_size) :  _beam_size(beam_size) {_S = std::set<T *, LESS_THAN>();}
  ~global_beam_agenda ();

  void push(T *t);
  T * top();
  T * pop(); 
  bool empty() { return _S.empty(); }
  int size() { return _S.size(); }

private :

  int _beam_size;
  std::set<T *, LESS_THAN > _S;
  typename std::set<T *, LESS_THAN>::iterator iter;
  typename std::set<T *, LESS_THAN>::reverse_iterator riter;
  
  void drop_worst_task ();

};

template <typename T, typename LESS_THAN>
global_beam_agenda<T,LESS_THAN>::~global_beam_agenda () 
{
  while (!empty()) {
    drop_worst_task();
  }
}

template <typename T, typename LESS_THAN>
void global_beam_agenda<T,LESS_THAN>::push (T *t) 
{
  _S.insert (t);
  if (size() > _beam_size) drop_worst_task(); 
}

template <typename T, typename LESS_THAN>
T * global_beam_agenda<T,LESS_THAN>::top () 
{
  return *_S.begin();
}

template <typename T, typename LESS_THAN>
T * global_beam_agenda<T, LESS_THAN>::pop () 
{ 
  iter = _S.begin();
  _S.erase(*iter);
  return *iter;
}

template <typename T, typename LESS_THAN>
void global_beam_agenda<T, LESS_THAN>::drop_worst_task () 
{
  riter = _S.rbegin();
  delete *riter;
  _S.erase(*riter);
}



#endif

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

/** agenda: a priority queue adapter */
template <typename T, typename LESS_THAN > class agenda {
public:

  agenda() : _A() {}

  ~agenda() { while(!this->empty()) delete this->pop(); }
  
  /** Push \a t onto agenda */
  void push(T *t) { _A.push(t); }
    
  /** Return the topmost (best) element from the agenda */
  T * top() { return _A.top(); }

  /** Remove the topmost element from the agenda and return it */
  T * pop() { T *t = top(); _A.pop(); return t; }

  /** Test if agenda is empty */
  bool empty() { return _A.empty(); }

private:

  std::priority_queue<T *, vector<T *>, LESS_THAN> _A;
};

#endif

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

/* class `chart' */

#ifndef _CHART_H_
#define _CHART_H_

#include "item.h"

class chart
{
 public:
  chart(int, auto_ptr<item_owner>);
  ~chart();

  void add(item *);

  void print(FILE *f);

  void get_statistics();

  inline int& pedges() { return _pedges; }

  unsigned int length() { return (unsigned int) _Cp_start.size() ; }
  unsigned int rightmost() { return length() - 1; }

  vector<item *> &trees() { return _trees; }
  vector<item *> &readings() { return _readings; }

  void shortest_path(list <item *> &);

 private:
  static int _next_stamp;

  vector<item *> _Chart;
  vector<item *> _trees;
  vector<item *> _readings;

  int _pedges;

  vector< list<item *> > _Cp_start, _Cp_end;
  vector< list<item *> > _Ca_start, _Ca_end;
  vector< vector < list<item*> > > _Cp_span;

  auto_ptr<item_owner> _item_owner;

  friend class chart_iter;
  friend class chart_iter_span_passive;
  friend class chart_iter_adj_active;
  friend class chart_iter_adj_passive;
};

// iterators must return items in order of `stamp', so the `excursion' works

class chart_iter
{
  public:
    inline chart_iter(chart *C) : _LI(C->_Chart)
    {
        _curr = _LI.begin();
    }

    inline chart_iter(chart &C) : _LI(C._Chart)
    {
        _curr = _LI.begin();
    }

    inline chart_iter &operator++(int)
    {
        ++_curr;
        return *this;
    }

    inline bool valid()
    {
        return _curr != _LI.end();
    }

    inline item *current()
    {
        if(valid())
            return *_curr;
        else
            return 0;
    }

 private:
    vector<class item *> &_LI;
    vector<class item *>::iterator _curr;
};

class chart_iter_span_passive
{
  public:
    inline chart_iter_span_passive(chart *C, int i1, int i2) :
        _LI(C->_Cp_span[i1][i2-i1])
    {
        _curr = _LI.begin();
    }

    inline chart_iter_span_passive(chart &C, int i1, int i2) :
        _LI(C._Cp_span[i1][i2-i1])
    {
        _curr = _LI.begin();
    }

    inline chart_iter_span_passive &operator++(int)
    {
        ++_curr;
        return *this;
    }

    inline bool valid()
    {
        return _curr != _LI.end();
    }

    inline item *current()
    {
        if(valid())
            return *_curr;
        else
            return 0;
    }

 private:
    list<item *> &_LI;
    list<item *>::iterator _curr;
};

class chart_iter_adj_passive
{
 public:
    inline
    chart_iter_adj_passive(chart *C, item *active)
        : _LI(active->left_extending() ?
              C->_Cp_end[active->start()] : C->_Cp_start[active->end()])
    {
        _curr = _LI.begin();
    }

    inline chart_iter_adj_passive &operator++(int)
    {
        ++_curr;
        return *this;
    }

    inline bool valid()
    {  
        return _curr != _LI.end();
    }

    inline item *current()
    {
        if(valid())
            return *_curr;
        else
            return 0;
    }

 private:
  list<item *> &_LI;
  list<item *>::iterator _curr;
};

class chart_iter_adj_active
{
  public:
    inline void overflow()
    {
        _at_start = false;
        _curr = _LI_end.begin();
    }

    inline chart_iter_adj_active(chart *C, item *passive)
        : _LI_start(C->_Ca_start[passive->end()]),
          _LI_end(C->_Ca_end[passive->start()]),
          _at_start(true)
    {
        _curr = _LI_start.begin();
        if(_curr == _LI_start.end()) overflow();
    }

    inline chart_iter_adj_active &operator++(int)
    {
        if(_at_start)
        {
            ++_curr;
            if(_curr == _LI_start.end())
                overflow();
        }
        else
            ++_curr;

        return *this;
    }

    inline bool valid()
    {   
        return _curr != _LI_end.end();
    }

    inline item *current()
    {
        if(valid()) return *_curr; else return 0;
    }

 private:
    list<item *> &_LI_start, &_LI_end;
    
    bool _at_start;
    
    list<item *>::iterator _curr;
};

#endif
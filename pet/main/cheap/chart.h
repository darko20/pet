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

/** \file chart.h 
 * Chart data structure for parsing in dynamic programming style.
 */

#ifndef _CHART_H_
#define _CHART_H_

#include "item.h"

/** Chart data structure for parsing, aka dynamic programming */
class chart
{
 public:
  /** Create a chart for \a len words, and item_owner \a owner to handle the
   *  proper destruction of items on chart destruction.
   *  \attention The function length() will return \a len + 1.
   */
  chart(int len, auto_ptr<item_owner> owner);
  ~chart();

  /** Add item to the appropriate internal data structures, depending on its
   *  activity.
   */
  void add(tItem *);

  /** Remove the item in the set from the chart */
  void remove(hash_set<tItem *> &to_delete);

  /** Print all chart items */
  void print(FILE *f);

  /** Print chart items using \a f, select active and passive items with \a
   *  passives and \a actives.
   */
  void print(tItemPrinter *f, bool passives = true, bool actives = false);

  /** Get statistics from the chart, like nr. of active/passive edges, average
   *  feature structure size, items contributing to a reading etc.
   */
  void get_statistics();

  /** Return the number of passive edges */
  inline int& pedges() { return _pedges; }

  /** Return the length of the chart */
  unsigned int length() { return (unsigned int) _Cp_start.size() ; }
  /** The number of the rightmost chart node */
  unsigned int rightmost() { return length() - 1; }

  /** Return the trees stored in the chart after unpacking */
  vector<tItem *> &trees() { return _trees; }
  /** Return the readings found during parsing */
  vector<tItem *> &readings() { return _readings; }

  /** If the parse was not successful, this function computes a shortest path
   *  through the chart based on some heuristic built into the tItem score()
   *  function to get the best partial results.
   */
  void shortest_path(list <tItem *> &items, bool all = false);
  
  /** Return \c true if the chart is connected using only edges considered \a
   *  valid, i.e., there is a path from the first to the last node.
   */
  bool connected(item_predicate &valid);

 private:
  static int _next_stamp;

  vector<tItem *> _Chart;
  vector<tItem *> _trees;
  vector<tItem *> _readings;

  int _pedges;

  vector< list<tItem *> > _Cp_start, _Cp_end;
  vector< list<tItem *> > _Ca_start, _Ca_end;
  vector< vector < list<tItem*> > > _Cp_span;

  auto_ptr<item_owner> _item_owner;

  friend class chart_iter;
  friend class chart_iter_span_passive;
  friend class chart_iter_topo;
  friend class chart_iter_adj_active;
  friend class chart_iter_adj_passive;
};

/** Return all items from the chart.
 * \attention iterators must return items in order of `stamp', so the
 * `excursion' works.
 */
class chart_iter
{
  public:
    /** Create a new iterator for \a C */
    inline chart_iter(chart *C) : _LI(C->_Chart)
    {
        _curr = _LI.begin();
    }

    /** Create a new iterator for \a C */
    inline chart_iter(chart &C) : _LI(C._Chart)
    {
        _curr = _LI.begin();
    }

    /** Increase iterator */
    inline chart_iter &operator++(int)
    {
        ++_curr;
        return *this;
    }

    /** Is the iterator still valid? */
    inline bool valid()
    {
        return _curr != _LI.end();
    }

    /** If valid(), return the current item, \c NULL otherwise. */
    inline tItem *current()
    {
        if(valid())
            return *_curr;
        else
            return 0;
    }

 private:
    friend class chart;

    vector<class tItem *> &_LI;
    vector<class tItem *>::iterator _curr;
};

/** Return all passive items having a specified span.
 * \attention iterators must return items in order of `stamp', so the
 * `excursion' works.
 */
class chart_iter_span_passive
{
  public:
    /** Create an iterator for all passive items in \a C starting at \a i1 and
     *  ending at \a i2.
     */
    inline chart_iter_span_passive(chart *C, int i1, int i2) :
        _LI(C->_Cp_span[i1][i2-i1])
    {
        _curr = _LI.begin();
    }

    /** Create an iterator for all passive items in \a C starting at \a i1 and
     *  ending at \a i2.
     */
    inline chart_iter_span_passive(chart &C, int i1, int i2) :
        _LI(C._Cp_span[i1][i2-i1])
    {
        _curr = _LI.begin();
    }

    /** Increase iterator */
    inline chart_iter_span_passive &operator++(int)
    {
        ++_curr;
        return *this;
    }

    /** Is the iterator still valid? */
    inline bool valid()
    {
        return _curr != _LI.end();
    }

    /** If valid(), return the current item, \c NULL otherwise. */
    inline tItem *current()
    {
        if(valid())
            return *_curr;
        else
            return 0;
    }

 private:
    list<tItem *> &_LI;
    list<tItem *>::iterator _curr;
};


/** Return all passive in topological order, i.e., those with smaller start
 * position first.
 */
class chart_iter_topo
{
  private:
    inline void next()
    {
        while ((_curr == _LI[_currindex].end()) && ++_currindex && valid())
        {
            _curr = _LI[_currindex].begin();
        }
    }

  public:
    /** Create a new iterator for \a C */
    inline chart_iter_topo(chart *C) : _max(C->rightmost()), _LI(C->_Cp_start)
    {
        _currindex = 0;
        _curr = _LI[_currindex].begin();
        next();
    }

    /** Create a new iterator for \a C */
    inline chart_iter_topo(chart &C) : _max(C.rightmost()), _LI(C._Cp_start)
    {
        _currindex = 0;
        _curr = _LI[_currindex].begin();
        next();
    }

    /** Increase iterator */
    inline chart_iter_topo &operator++(int)
    {
        _curr++;
        next();
        return *this;
    }

    /** Is the iterator still valid? */
    inline bool valid()
    {
        return (_currindex <= _max);
    }

    /** If valid(), return the current item, \c NULL otherwise. */
    inline tItem *current()
    {
        if(valid())
            return *_curr;
        else
            return 0;
    }

 private:
    friend class chart;

    int _max, _currindex;
    vector< list< class tItem * > > &_LI;
    list<class tItem *>::iterator _curr;
};

/** Return all passive items adjacent to a given active item
 * \attention iterators must return items in order of `stamp', so the
 * `excursion' works.
 */
class chart_iter_adj_passive
{
 public:
    /** Create new iterator for chart \a C that returns the passive items
     *  adjacent to \a active.
     */
    inline
    chart_iter_adj_passive(chart *C, tItem *active)
        : _LI(active->left_extending() ?
              C->_Cp_end[active->start()] : C->_Cp_start[active->end()])
    {
        _curr = _LI.begin();
    }

    /** Increase iterator */
    inline chart_iter_adj_passive &operator++(int)
    {
        ++_curr;
        return *this;
    }

    /** Is the iterator still valid? */
    inline bool valid()
    {  
        return _curr != _LI.end();
    }

    /** If valid(), return the current item, \c NULL otherwise. */
    inline tItem *current()
    {
        if(valid())
            return *_curr;
        else
            return 0;
    }

 private:
  list<tItem *> &_LI;
  list<tItem *>::iterator _curr;
};

/** Return all active items adjacent to a given passive item
 * \attention iterators must return items in order of `stamp', so the
 * `excursion' works.
 */
class chart_iter_adj_active
{
    inline void overflow()
    {
        _at_start = false;
        _curr = _LI_end.begin();
    }

  public:
    /** Create new iterator for chart \a C that returns the active items
     *  adjacent to \a passive.
     */
    inline chart_iter_adj_active(chart *C, tItem *passive)
        : _LI_start(C->_Ca_start[passive->end()]),
          _LI_end(C->_Ca_end[passive->start()]),
          _at_start(true)
    {
        _curr = _LI_start.begin();
        if(_curr == _LI_start.end()) overflow();
    }

    /** Increase iterator */
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

    /** Is the iterator still valid? */
    inline bool valid()
    {   
        return _curr != _LI_end.end();
    }

    /** If valid(), return the current item, \c NULL otherwise. */
    inline tItem *current()
    {
        if(valid()) return *_curr; else return 0;
    }

 private:
    list<tItem *> &_LI_start, &_LI_end;
    
    bool _at_start;
    
    list<tItem *>::iterator _curr;
};

#endif

/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 * (C) 2001 Bernd Kiefer kiefer@dfki.de
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

// input_chart implements a (quite) generic input token interface

#ifndef _INPUT_CHART_H_
#define _INPUT_CHART_H_

#include "postags.h"
#include "types.h"
#include "chartpositions.h"
#include "inputtoken.h"

extern struct settings *cheap_settings;

/*
This constructor of add_token gets a string with a lexical specification 
of the following form:

<start> <end> <instance> <inflection> <priority>
   \{ ( <feature>\{.<feature>\}* <sort-name> ) \}*

where the path / sort-name pairs indicate feature structures to be unified into
the prototypical lexicon entry.

*/

// adj_iterator implements the possibility to find the appropriate
// adjacent items to complete multi word entries. Depending on the value of
// left_p, it looks to the left or the right of pos for items whose orthography
// is orth
// at the moment, this is implemented rather inefficiently always scanning the
// whole list of input items.

class adj_iterator
{
public:

  adj_iterator(bool left_p, int pos, const char *orth,
	       list <input_token *>::iterator curr,
	       list <input_token *>::iterator end) :
    _left_p(left_p), _pos(pos), _orth(orth), _curr(curr), _end(end)
  {
    next_appropriate();
  }

  bool valid()
  {
    return _curr != _end;
  }

  adj_iterator& operator++(int)
  {
    // seek for next matching item 
    _curr++;
    next_appropriate();
    return *this;
  }

  input_token *operator *()
  {
    return *_curr;
  }

  const static bool left = true;
  const static bool right = false;
private:

  // *curr is of type input_token*, set _curr to next appropriate token, if any
  void next_appropriate()
  {
    while(valid() &&
	  ((_pos != (_left_p ? (*_curr)->end() : (*_curr)->start()))
	    || ((_orth != 0)
		&& (strcasecmp(_orth, ((*_curr)->orth()).c_str()) != 0))))
      _curr++;
  }

  bool _left_p;
  int _pos;
  const char *_orth;
  list<input_token *>::iterator _curr, _end;
};


class input_chart
{
public:
  input_chart(position_map * pmap) :
    _positionmap(pmap)
  {
    clear();
  }

  // every input_token was created by adding it to a input_chart, so its
  // destruction obliges the same input_chart
  // furthermore, input_chart assumes that it holds a fresh position_map, which it
  // handles and destroys. So maybe it's best to keep no reference to this
  // position_map from the outside
  ~input_chart()
  {
    clear();
    delete _positionmap;
  }

  void clear()
  {
    for(iterator it = _inputtokens.begin(); it != _inputtokens.end(); it++)
      delete *it;
    _inputtokens.clear();
    _positionmap->clear();
    clear_dynamic_symbols(); // _fix_me_ does this belong here?
  }

  void populate(class tokenizer *t);
  
  input_token *add_token(const string &tokenstring);
  input_token *add_token(int id, int start, int end, class full_form ff,
                         string orth, double p, const postags &pos,
                         const tPaths &paths,
                         bool synthesized = false)
  {
    return add_token(new input_token(id, start, end, ff, orth, p, pos, paths,
                                     this, synthesized));
  }

  void assign_position(input_token *t);
  void assign_positions();

  adj_iterator begin(bool left_p, int pos, const char *orth = 0)
  {
    return adj_iterator(left_p, pos, orth, _inputtokens.begin(),
			_inputtokens.end());
  }

  // produces a list of lex_items that can be added to the task list from the
  // list of input items
  int expand_all(list<class lex_item *> &result);

  typedef list<pair<int, int> > gaplist;

  // finds gaps in the input chart
  gaplist gaps(int max, list<class lex_item *> &input);

  // find all forms between chart positions p1 and p2
  set<string> forms(int p1, int p2);

  // returns the orths of the unmatched tokens 
  string uncovered(const gaplist &gaps);

  // cover gaps with generic entries
  list<class lex_item *> cover_gaps(const gaplist &gaps);

  // add generic entries
  void add_generics(list<class lex_item *> &input);

  int max_position() { return _positionmap->max_chart_position(); }

  void print(FILE * f)
  {
    for(iterator i = begin(); i != end(); i++)
      {
	(*i)->print(f);
	fprintf(f, "\n");
      }
  }

private:

  typedef list<input_token *> impltype;
  typedef impltype::iterator iterator;

  iterator begin()
  {
    return _inputtokens.begin();
  }

  iterator end()
  {
    return _inputtokens.end();
  }

  iterator operator++(int)
  {
    return (*this)++;
  }

  input_token *add_token(input_token *);

  position_map *_positionmap;
  list<input_token *> _inputtokens;
};

void dependency_filter(list<class lex_item *> &result, struct setting *deps,
                       bool unidirectional);

#endif

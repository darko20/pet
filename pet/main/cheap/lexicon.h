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

/* Lexicon classes */

#ifndef _LEXICON_H_
#define _LEXICON_H_

#include "fs.h"

class lex_stem
{
 public:

  lex_stem(type_t t, const modlist &mods = modlist(), const list<string> &orths = list<string>());
  lex_stem(const lex_stem &le);
  ~lex_stem();

  lex_stem & operator=(const lex_stem &le);

  fs instantiate();

  inline char *name() const { return typenames[_type]; }
  inline char *printname() const { return printnames[_type]; }

  inline int type() const { return _type; }
  inline int length() const { return _nwords; }
  inline int inflpos() const { return _nwords - 1; }

  inline const char *orth(int i) const { 
    assert(i < _nwords);
    return _orth[i];
  }
  
  void print(FILE *f) const;

 private:
  static int next_id;

  int _id;

  int _type;        // type index

  modlist _mods;

  int _nwords;      // length of _orth
  char **_orth;     // array of _nwords strings

  vector<string> get_stems();

  friend class tGrammar;
};

class full_form
{
 public:
  full_form() :
    _stem(0), _affixes(0), _offset(0) {};

  full_form(lex_stem *st, modlist mods = modlist(), list_int *affixes = 0)
    : _stem(st), _affixes(copy_list(affixes)), _offset(0), _mods(mods)
    { if(st) _offset = st->inflpos(); }

  full_form(dumper *f, class tGrammar *G);

#ifdef ONLINEMORPH
  full_form(lex_stem *st, class tMorphAnalysis a);
#endif

  full_form(const full_form &that)
    : _stem(that._stem), _affixes(copy_list(that._affixes)),
    _offset(that._offset), _form(that._form), _mods(that._mods) {}

  full_form &operator=(const full_form &that)
  {
    _stem = that._stem;
    free_list(_affixes);
    _affixes = copy_list(that._affixes);
    _offset = that._offset;
    _form = that._form;
    _mods = that._mods;
    return *this;
  }

  ~full_form()
  {
    free_list(_affixes);
  }

  bool valid() const { return _stem != 0; }

  bool operator==(const full_form &f) const
  {
    return (compare(_affixes, f._affixes) == 0) && _stem == f._stem;
    // it's ok to compare pointers here, as stems are not copied
  }

  bool operator<(const full_form &f) const
  {
    if(_stem == f._stem)
      return compare(_affixes, f._affixes) == -1;
    else
      return _stem < f._stem;
      // it's ok to compare pointers here, as stems are not copied
  }

  fs instantiate();

  inline const lex_stem *stem() const { return _stem; }

  inline int offset() { return _offset; }

  inline int length()
  {
    if(valid()) return _stem->length(); else return 0;
  }

  inline string orth(int i)
  {
    if(i >= 0 && i < length())
      {
	if(i != offset()) return _stem->orth(i); else return _form;
      }
    else
      return 0;
  }
  
  inline string key()
  {
    return orth(offset());
  }

  list_int *affixes() { return _affixes; }

  const char *stemprintname()
  {
    if(valid()) return _stem->printname(); else return 0;
  }

  string affixprintname();

  void print(FILE *f);

  string description();

 private:
  lex_stem *_stem;
  list_int* _affixes;
  int _offset;

  string _form;
  modlist _mods;

  friend class tGrammar;
};

#endif

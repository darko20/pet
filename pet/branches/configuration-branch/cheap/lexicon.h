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

/* \file lexicon.h 
 * Lexicon classes
 */

#ifndef _LEXICON_H_
#define _LEXICON_H_

#include "types.h"
#include "logging.h"

/** A lexicon entry. */
class lex_stem
{
 public:

  /** Create a lex_stem for the given instance and lexical types
   * \param instance_type This type may exist, or it may only be there for
   *        decoration purposes, i.e., for printing/storing the derivation
   *        This might be the case if the data for this lex_stem come from
   *        an external lexical database.
   *        A problem may occur for those pieces of code that use this type
   *        id as identifier for the lexicon entry in use, such as the
   *        statistical model code. In this case, the appropriate dynamic
   *        symbol must already be present in the system, to guarantee that
   *        the string will be mapped onto the correct id.
   * \param lex_type The type that has to be a valid lexical type, i.e., it is
   *        possible to build a lexical entry with its corresponding type dag
   *        alone. If lex_type is not passed as an argument, it will be
   *        determined by the parent of the instance type
   * \param mods Possible feature structure modifications (default: empty)
   * \param orths The surface forms for this entry. If this list is empty, the
   *              forms are computed from the type dag using the global setting
   *              \c orth-path.
   */
  lex_stem(type_t instance_type, type_t lex_type = -1
           , const std::list<std::string> &orths = std::list<std::string>());
  ~lex_stem();

  /** (Re)create the feature structure for this entry from the dags of the
   *  instance and the root type of the instance.
   */
  class fs instantiate();

  /** Return the (internal) type name for this entry */
  inline const char *name() const { return type_name(_instance_type); }
  /** Return the (external) type name for this entry */
  inline const char *printname() const { return print_name(_instance_type); }

  /** Return the type of this entry.
   * \attn If anybody uses this function, she/he must be aware that this might
   * be a dynamic type with NO CONNECTION TO THE TYPE HIERARCHY, e.g. because
   * this type came from an external lexicon.
   * If any code relies on this id, it must be sure that either a) the type is
   * in the .grm file already, and consequently has a fixed id, or b) the
   * corresponding type name has been registered early enough as a dynamic
   * symbol.
   */
  inline int type() const { return _instance_type; }

  /** Return the arity of this lexicon entry */
  inline int length() const { return _nwords; }

  /** Return the inflected argument of this lexicon entry.
   *  \todo This has to be made variable.
   */
  inline int inflpos() const { return _nwords - 1; }

  /** Return the \a i th surface element */
  inline const char *orth(int i) const { 
    assert(i < _nwords);
    return _orth[i];
  }
  
  /** Print readable representation for debugging purposes */
  void print(IPrintfHandler &iph) const;

 private:
  /** Inhibited copy constructor */
  lex_stem(const lex_stem &le) {}
  /** Inhibited assignment operator */
  lex_stem & operator=(const lex_stem &le) { return *this; } 

  static int next_id;

  /** unique internal id */
  int _id;
  /** type id of the instance */
  int _instance_type;
  /** type id of the lexical class */
  int _lexical_type;

  // modlist _mods;

  /** length of _orth */
  int _nwords;
  /** array of _nwords strings */
  char **_orth;

  std::vector<std::string> get_stems();

  friend class tGrammar;
};

#ifdef USE_DEPRECATED_CODE
//obsolete
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

  class fs instantiate();

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

#endif
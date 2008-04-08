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

/** \file postags.h
 * class `postags': use of part of speech information.
 */

#ifndef _POSTAG_H_
#define _POSTAG_H_

#include "types.h"
#include <string>
#include <set>
#include <vector>
#include <ios>

/** Implements a list of POS tags with probabilities. All string handling is
 *  case insensitive.
 */
class postags
{
 public:
  /** Construct an empty set of tags. */
  postags() : _tags() {} ;
  /** Create a set of POS tags with \a tagslist and \a probslist.
   * \pre the \c size() of the lists has to be equal.
   */
  postags(const std::vector<std::string> &tagslist,
          const std::vector<double> &probslist);
  /** Create a set of POS tags specified for the stem of \a le by the setting
   * \c type-to-pos.
   */
  postags(const class lex_stem *le);
  /** Create a set of POS tags by merging the supplied POS tags of the lexical
   *  items in \a les.
   */
  postags(const std::list<class tItem *> &les);
  /** Copy constructor: copies only the tags, not the probabilities. */
  postags(const postags &t) : _tags(t._tags) {} ;

  ~postags() {} ;

  /** Return true if tag set is empty. */
  bool empty() const
    { return _tags.empty(); }

  /** Assignment operator: the tag set is set to the tag set of \a b */
  void operator=(const postags &b) 
  {
    _tags = b._tags;
  }

  /** Return true, if the tag sets are equal */
  bool operator==(const postags &b) const;

  /** Add a single POS tag \a s to the set */
  void add(std::string s);
  /** Add a single POS tag \a s with probability \a prob to the set */
  void add(std::string s, double prob);
  /** Add all tags of \a s to the set (without probabilities). */
  void add(const postags &s);

  /** Remove POS tag \a s from the set */
  void remove(std::string s);
  /** Remove all tags of \a s from the set. */
  void remove(const postags &s);

  /** Does the tag set contain a tag with name \a s (case insensitive) */
  bool contains(const std::string &s) const;
  /** Return \c true if \a t is a subtype of some type mentioned in the \c
   *  posmapping setting and the POS tag associated with the type in the
   *  setting is contained in this set of tags.  If there is no valid setting,
   *  this function returns \c false.
   */
  bool contains(type_t t) const;

  /** Like contains, but in case the setting is not available, return \c true.
   * \see contains
   */
  bool license(type_t t) const;

  /** Print the contents of this set for debugging purposes */
  void print(std::ostream &out) const;

  /** serialise as string (for debugging purposes) **/
  std::string getPrintString() const;

 private:
  bool contains(type_t t, const class setting *set) const;

  struct ltstr {
    bool operator()(const std::string &s1, const std::string &s2) const {
      return strcasecmp(s1.c_str(), s2.c_str()) < 0;
    }
  };

  /** String set with case insensitive comparison */
  std::set<std::string, ltstr> _tags;
  std::map<std::string, double, ltstr> _probs;
};

#endif

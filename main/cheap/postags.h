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

/* class `postags' */

#ifndef _POSTAG_H_
#define _POSTAG_H_

#include "types.h"

class postags
{
 public:
  postags() : _tags() {} ;
  postags(const vector<string> &, const vector<double> &);
  postags(const class full_form ff);
  postags(const list<class tLexItem *> &les);
  postags(const postags &t) : _tags(t._tags) {} ;

  ~postags() {} ;

  bool empty() const
    { return _tags.empty(); }

  void operator=(const postags &b) 
  {
    _tags = b._tags;
  }

  bool operator==(const postags &b) const;

  void add(string s);
  void add(const postags &s);

  void remove(string s);
  void remove(const postags &s);

  bool contains(string s) const;
  bool contains(type_t t) const;

  bool license(const char *setting, type_t t) const;

  void print(FILE *) const;

 private:
  set<string> _tags;
  map<string, double> _probs;
};

#endif

/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class `postags' */

#ifndef _POSTAG_H_
#define _POSTAG_H_

#include "types.h"

class postags
{
 public:
  postags() : _tags() {} ;
  postags(const set<string> &tags) : _tags(tags) {} ;
  postags(const class full_form ff);
  postags(const list<class lex_item *> &les);
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

  int priority(type_t t, int initialp = 0) const;

  void print(FILE *) const;

 private:

  set<string> _tags;
};

#endif

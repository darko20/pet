/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class `postags' */

#ifndef _POSTAG_H_
#define _POSTAG_H_

#include <stdio.h>
#include <set>
#include <string>
using namespace std;

class postags
{
 public:
  postags() : _tags() {} ;
  postags(const set<string> &tags) : _tags(tags) {} ;

  bool empty() const
    { return _tags.empty(); }

  bool contains(string s) const;

  void remove(string s);

  void print(FILE *) const;

 private:

  set<string> _tags;
};

#endif

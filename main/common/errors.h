/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* defines classes representing errors - used in exceptions */

#ifndef _ERROR_H_
#define _ERROR_H_

#include <stdio.h>

#include <new>
#include <string>

using namespace std;

class error
{
 public:
  error();
  error(string s);
  virtual ~error();

  virtual string msg() { return _msg; }
  virtual void print(FILE *f);
#ifdef TSDBAPI
  virtual void tsdb_print();
#endif

 private:
  string _msg;
};

class error_ressource_limit : public error
{
 public:
  error_ressource_limit(string limit, int v);

  virtual string msg();
  virtual void print(FILE *f);
#ifdef TSDBAPI
  virtual void tsdb_print();
#endif

 private:
  string _lim;
  int _val;
};

#endif


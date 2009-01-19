/* -*- Mode: C++ -*- */

#ifndef _VPM_H_
#define _VPM_H_

#include "lex-tdl.h"
#include "mrs.h"
#include <string>
#include <list>

#define MAXVPMLINE 1024

using namespace std;
using namespace mrs;

class tVPM {
public:

  tVPM() {
  }
  
  virtual ~tVPM();

  string id;
  // what is this tms?
  //list<class tm*> tms;

  // parameter mapping rules
  list<class tPM*> pms;

  bool read_vpm(const string &filename, string id="default");
  
  tPSOA* map_mrs(tPSOA* mrs_in, bool forwardp=true);
  tVar* map_variable(tVar* var_in, tPSOA* mrs, bool forwardp=true);

private:
  map<tVar*,tVar*> _vv_map;
  
};

class tPM {
public:

  tPM() {
  }

  virtual ~tPM();
  
  list<string> lhs;
  list<string> rhs;
  list<class tPMR*> pmrs;
};

class tPMR {
public:
  tPMR() : forward(false), backward(false), eqtest(false) {
  }

  list<string> left;
  list<string> right;
  bool forward;
  bool backward;
  bool eqtest;
  
  bool test(char type, list<string> values, bool forwardp);
};


#endif //_VPM_H_

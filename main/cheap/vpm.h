/* -*- Mode: C++ -*- */

#ifndef _VPM_H_
#define _VPM_H_

#include "lex-tdl.h"
#include "mrs.h"
#include <string>
#include <list>

#define MAXVPMLINE 1024

using namespace mrs;

class tVPM {
public:

  tVPM() {
  }
  
  virtual ~tVPM();

  std::string id;

  /** variable type mappings */
  std::list<class tMR*> tms;

  /** parameter mapping rules*/
  std::list<class tPM*> pms;

  bool read_vpm(const std::string &filename, std::string id="default");
  
  tPSOA* map_mrs(tPSOA* mrs_in, bool forwardp=true);
  tVar* map_variable(tVar* var_in, tPSOA* mrs, bool forwardp=true);

private:
  std::map<tVar*,tVar*> _vv_map;
  
};

/** Parameter mapping */
class tPM {
public:

  tPM() {
  }

  virtual ~tPM();
  
  std::list<std::string> lhs;
  std::list<std::string> rhs;
  std::list<class tMR*> pmrs;
};

/** Mapping rule, being it for parameters or types */
class tMR {
public:
  tMR() : forward(false), backward(false), eqtest(false) {
  }

  std::list<std::string> left;
  std::list<std::string> right;
  bool forward;
  bool backward;
  bool eqtest;
  
  bool test(std::string type, std::list<std::string> values, bool forwardp);
};


#endif //_VPM_H_

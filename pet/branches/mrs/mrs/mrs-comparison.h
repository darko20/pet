/* -*- Mode: C++ -*- */

#ifndef _MRS_COMPARISON_H_
#define _MRS_COMPARISON_H_

#include "pet-config.h"

#include "dag.h"
#include "mrs.h"

#include <list>
#include <set>
#include <map>
#include <string>
#include <iostream>

namespace mrs {

/** \brief Solution for MRS transfer and comparison
 * Solution
 */
class Solution {
  
public:

  /** Default Constructor*/
  Solution() {
  }

  /** Copy Constructor */
  Solution(const Solution&);
  
  tVar* lookup_variable(tVar*);
  tBaseRel* lookup_ep(tBaseRel*);
  tHCons* lookup_hcons(tHCons*);

  void align_variables(tVar*, tVar*);
  void align_eps(tBaseRel*, tBaseRel*);
  void align_hconss(tHCons*, tHCons*);

  ///** Comparator */
  //operator<=(Solution &s2);

private:
  std::map<tVar*,tVar*> _variables;
  std::map<tBaseRel*,tBaseRel*> _eps;
  std::map<tHCons*,tHCons*> _hconss;
  //  std::list<> _matches; // what are these then?
};

class MrsComparator {
  
public:


  MrsComparator(int type = 0, bool ignore_hcons = false, 
                bool ignore_all_roles = false) : 
    _type(type), _ignore_hcons(ignore_hcons), 
    _ignore_all_roles(ignore_all_roles) {
  }

  Solution* compare_mrss(tBaseMRS*, tBaseMRS*);
  std::list<Solution*> compare_epss(std::list<tBaseRel*>, std::list<tBaseRel*>, Solution*);
  Solution* compare_eps(tBaseRel*, tBaseRel*, Solution*);
  std::list<Solution*> compare_hconss(std::list<tHCons*>, std::list<tHCons*>, Solution*);
  Solution* compare_hcons(tHCons*, tHCons*, Solution*);
  bool compare_preds(std::string, std::string); 
  bool compare_values(tValue*, tValue*, Solution*);
  bool compare_variables(tVar*, tVar*, Solution*); // or should return tVar*?
  bool compare_extras(std::map<std::string,std::string>, std::map<std::string,std::string>);
  bool compare_constants(tConstant*, tConstant*);
  bool compare_types(std::string, std::string);
  
private:
  std::list<Solution*> _solutions;

  int _type; // 0: equivalence; 1: subsumption
  bool _ignore_hcons;
  std::map<std::string,std::list<std::string> > _equivalent_types; 
  std::map<std::string,std::list<std::string> > _equivalent_predicates;
  bool _ignore_all_roles;
  std::set<std::string> _ignore_roles;
  bool _ignore_all_properties;
  std::set<std::string> _ignore_properties;
};

}

#endif // _MRS_COMPARISON_H_

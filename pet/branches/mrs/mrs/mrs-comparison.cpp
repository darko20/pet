/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
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

#include "mrs-comparison.h"
#include "vpm.h"

#include "logging.h"
#include "settings.h"
#include "types.h"
#ifdef MRS_ONLY
#include "mrs-utility.h"
#else
#include "pet-config.h"
#include "utility.h"
#endif

using namespace mrs;


Solution::Solution(const Solution& s) {
  // TODO
}


tBaseRel*
Solution::lookup_ep(tBaseRel* ep) {
  if (_eps.find(ep) != _eps.end())
    return _eps[ep];
  else
    return NULL;
}

tVar*
Solution::lookup_variable(tVar* var) {
  if (_variables.find(var) != _variables.end())
    return _variables[var];
  else
    return NULL;
}

tHCons*
Solution::lookup_hcons(tHCons* hcons) {
  if (_hconss.find(hcons) != _hconss.end())
    return _hconss[hcons];
  else
    return NULL;
}

void
Solution::align_variables(tVar* var1,
                          tVar* var2) {
  _variables[var1] = var2;
}

void
Solution::align_hconss(tHCons* hcons1, 
                       tHCons* hcons2) {
  _hconss[hcons1] = hcons2;
}

void
Solution::align_eps(tBaseRel* ep1,
                    tBaseRel* ep2) {
  _eps[ep1] = ep2;
}


Solution*
MrsComparator::compare_mrss(tBaseMRS* mrs1,
                            tBaseMRS* mrs2) {
  // TODO
  Solution *solution = new Solution;
  Solution *result = NULL;
  if (!compare_variables(mrs1->top_h, mrs2->top_h, solution)) {
    delete solution;
    return NULL;
  }
  std::list<Solution*> solutions = compare_epss(mrs1->liszt,
                                                mrs2->liszt, solution);
  if (solutions.size() == 0) {
    delete solution;
    return NULL;
  }
  if (_ignore_hcons) {
    Solution *result = solutions.front();
    if (solution != result)
      delete solution;
    for (std::list<Solution*>::iterator s = solutions.begin();
         s != solutions.end(); s ++)
      if ((*s) != result)
        delete *s;
    return result;
  }

  for (std::list<Solution*>::iterator sol = solutions.begin();
       sol != solutions.end(); sol ++) {
    std::list<Solution*> solved = compare_hconss(mrs1->h_cons,
                                                 mrs2->h_cons, *sol);
    if (solved.size() != 0) {
      result = solved.front();
      solved.pop_front();
      for (std::list<Solution*>::iterator s = solutions.begin();
           s != solutions.end(); s ++) {
        if ((*s) != result) {
          delete *s;
          *s = NULL;
        }
      }
      for (std::list<Solution*>::iterator s = solved.begin();
           s != solved.end(); s++) {
        if ((*s) != result) {
          delete *s;
          *s = NULL;
        }
      }
      if (solution != result) 
        delete solution;
      return result;
    }
  }
  for (std::list<Solution*>::iterator s = solutions.begin();
       s != solutions.end(); s ++) {
    delete *s;
    *s = NULL;
  }
  delete solution;
  return NULL;
}

std::list<Solution*>
MrsComparator::compare_epss(std::list<tBaseRel*> eps1, 
                            std::list<tBaseRel*> eps2,
                            Solution* solution) {
  std::list<Solution*> solutions;
  if (eps1.size() != 0) {
    if (eps2.size() == 0) {
      solutions.push_back(solution);
      return solutions;
    }
    tBaseRel* ep2 = eps2.front();
    eps2.pop_front();
    std::list<Solution*> sols;
    for (std::list<tBaseRel*>::iterator ep1 = eps1.begin();
         ep1 != eps1.end(); ep1 ++) {
      Solution* newsolution = compare_eps(*ep1, ep2, solution);
      if (newsolution != NULL) {
        sols.push_back(newsolution);
      }
    }
    for (std::list<Solution*>::iterator sol = sols.begin();
         sol != sols.end(); sol ++) {
      std::list<Solution*> newsols = compare_epss(eps1, eps2, *sol);
      solutions.splice(solutions.end(),newsols);
    }
  } else if (eps2.size() == 0) {
    solutions.push_back(solution);
  }
  return solutions;
}

Solution*
MrsComparator::compare_eps(tBaseRel* ep1,
                           tBaseRel* ep2,
                           Solution* solution) {
  // if ep1 is already aligned with something, abort
  if (solution->lookup_ep(ep1) != NULL)
    return NULL;
  Solution* newsolution = new Solution(*solution);
  bool succeed = false;
  if (compare_preds(ep1->pred, ep2->pred)) {
    tRel* rel1 = dynamic_cast<tRel*>(ep1);
    tRel* rel2 = dynamic_cast<tRel*>(ep2);
    if (rel1 == NULL || rel2 == NULL ||
        !compare_variables(rel1->handel, rel2->handel, newsolution)) {
      delete newsolution;
      return NULL;
    }
    if (_ignore_all_roles) {
      succeed = true;
    } else {
      std::set<std::string> features;
      std::map<std::string,tValue*> roleset1;
      for (std::map<std::string,tValue*>::iterator role1 
             = ep1->flist.begin();
           role1 != ep1->flist.end(); role1 ++) {
        if (_ignore_roles.find((*role1).first) == _ignore_roles.end()) {
          roleset1.insert(*role1);
          features.insert((*role1).first);
        }
      }
      std::map<std::string,tValue*> roleset2;
      for (std::map<std::string,tValue*>::iterator role2 
             = ep2->flist.begin();
           role2 != ep2->flist.end(); role2 ++) {
        if (_ignore_roles.find((*role2).first) == _ignore_roles.end()) {
          if (_type == 1 
              && roleset1.find((*role2).first) == roleset1.end()) {
            // in subsumption mode, make sure all roles from the 
            // second EP are present in the target EP. 
            delete newsolution;
            return NULL;
          }
          roleset2.insert(*role2);
          features.insert((*role2).first);
        }
      }
      std::set<std::string> intersection;
      for (std::set<std::string>::iterator feat = features.begin();
           feat != features.end(); feat ++) {
        if (roleset1.find(*feat) != roleset1.end()
            && roleset2.find(*feat) != roleset2.end()) {
          intersection.insert(*feat);
        } else if (_type == 0) {
          // in equivalence mode, the intersection better be the same
          // as each of the two input sets (in terms of role labels).
          delete newsolution;
          return NULL;
        }
      }
      for (std::set<std::string>::iterator f = intersection.begin();
           f != intersection.end(); f ++) {
        if (!compare_values(roleset1[*f], roleset2[*f], newsolution)) {
          delete newsolution;
          return NULL;
        }
      }
      succeed = true;
    }
  }

  if (succeed) {
    newsolution->align_eps(ep1, ep2);
    return newsolution;
  } else {
    delete newsolution;
    return NULL;
  }
}

std::list<Solution*>
MrsComparator::compare_hconss(std::list<tHCons*> hconss1,
                              std::list<tHCons*> hconss2,
                              Solution* solution) {
  std::list<Solution*> solutions;
  if (hconss1.size() != 0) {
    if (hconss2.size() == 0) {
      solutions.push_back(solution);
      return solutions;
    }
    tHCons* hcons2 = hconss2.front();
    hconss2.pop_front();
    std::list<Solution*> sols;
    for (std::list<tHCons*>::iterator hcons1 = hconss1.begin();
         hcons1 != hconss1.end(); hcons1 ++) {
      Solution* newsolution = compare_hcons(*hcons1, hcons2, solution);
      if (newsolution != NULL) {
        sols.push_back(newsolution);
      }
    }
    for (std::list<Solution*>::iterator sol = sols.begin();
         sol != sols.end(); sol ++) {
      std::list<Solution*> newsols = 
        compare_hconss(hconss1, hconss2 , *sol);
      solutions.splice(solutions.end(), newsols);
    }
  } else if (hconss2.size() == 0) {
    solutions.push_back(solution);
  }
  return solutions;
}

Solution*
MrsComparator::compare_hcons(tHCons* hcons1,
                             tHCons* hcons2,
                             Solution* solution) {
  Solution* newsolution = new Solution(*solution);
  if (compare_values(hcons1->scarg, hcons2->scarg, newsolution) &&
      compare_values(hcons1->outscpd, hcons2->outscpd, newsolution)) {
    newsolution->align_hconss(hcons1, hcons2);
    return newsolution;
  } else {
    delete newsolution;
    return NULL;
  }
}

bool
MrsComparator::compare_preds(std::string pred1,
                             std::string pred2) {
  if (compare_types(pred1, pred2))
    return true;
  for (std::map<std::string,std::list<std::string> >::iterator it
         = _equivalent_predicates.begin();
       it != _equivalent_predicates.end(); it ++) {
    bool p1match = false;
    for (std::list<std::string>::iterator old = (*it).second.begin();
         old != (*it).second.end(); old ++) {
      if (compare_types(pred1, *old)) {
        p1match = true;
        break;
      }
    }
    if (p1match 
        && compare_preds((*it).first, pred2)) { 
      // TODO: be careful with infinite loops here
      return true;
    }
  }
  return false;
}

bool
MrsComparator::compare_values(tValue* val1,
                              tValue* val2,
                              Solution* solution) {
  tVar* var1 = dynamic_cast<tVar *>(val1);
  if (var1 != NULL) {
    tVar* var2 = dynamic_cast<tVar *>(val2);
    if (var2 == NULL) 
      return false;
    else
      return compare_variables(var1, var2, solution);
  } else {
    tConstant* const1 = dynamic_cast<tConstant *>(val1);
    if (const1 != NULL) {
      tConstant* const2 = dynamic_cast<tConstant *>(val2);
      if (const2 == NULL)
        return false;
      else
        return compare_constants(const1, const2);
    }
  }
    return false;
}

bool
MrsComparator::compare_variables(tVar* var1,
                                 tVar* var2,
                                 Solution* solution) {
  if (var1 == var2 && var1 == NULL)
    return true;
  tVar* foo = solution->lookup_variable(var2);
  if (foo == var1) {
    solution->align_variables(var2, var1);
    return true;
  }
  if (foo == NULL) {
    if (compare_types(var1->type, var2->type) 
        && compare_extras(var1->extra, var2->extra)) {
      solution->align_variables(var2, var1);
      return true;
    }
    for (std::map<std::string,std::list<std::string> >::iterator it
           = _equivalent_types.begin();
         it != _equivalent_types.end(); it ++) {
      if (compare_types((*it).first, var2->type)) {
        for (std::list<std::string>::iterator old = (*it).second.begin();
             old != (*it).second.end(); old ++) {
          if (compare_types(var1->type, *old)
              && compare_extras(var1->extra, var2->extra)) {
            solution->align_variables(var2, var1);
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool
MrsComparator::compare_extras(std::map<std::string,std::string> extras1,
                              std::map<std::string,std::string> extras2) {
  if (_ignore_all_properties)
    return true;
  std::set<std::string> features;
  std::map<std::string,std::string> properties1;
  for (std::map<std::string,std::string>::iterator p1
         = extras1.begin();
       p1 != extras1.end(); p1 ++) {
    if (_ignore_properties.find((*p1).first) == _ignore_properties.end()) {
      properties1.insert(*p1);
      features.insert((*p1).first);
    }
  }
  std::map<std::string,std::string> properties2;
  for (std::map<std::string,std::string>::iterator p2
         = extras2.begin();
       p2 != extras2.end(); p2 ++) {
    if (_ignore_properties.find((*p2).first) == _ignore_properties.end()) {
      if (_type == 1
          && properties1.find((*p2).first) == properties1.end()) {
        // in subsumption mode, make sure all properties from the
        // second variable are present in the target variable.
        return false;
      }
      properties2.insert(*p2);
      features.insert((*p2).first);
    }
  }
  std::set<std::string> intersection;
  for (std::set<std::string>::iterator feat = features.begin();
       feat != features.end(); feat ++) {
    if (properties1.find(*feat) != properties1.end()
        && properties2.find(*feat) != properties2.end()) {
      intersection.insert(*feat);
    } else if (_type == 0) {
      // in equivalence mode, the intersection better be the same
      // as each of the two input sets (in terms of properties).
      return false;
    }
  }
  for (std::set<std::string>::iterator f = intersection.begin();
       f != intersection.end(); f ++) {
    if (!compare_types(properties1[*f], properties2[*f])) {
      return false;
    }
  }
  return true;
}

bool
MrsComparator::compare_constants(tConstant* const1,
                                 tConstant* const2) {
  if (const1 == const2 ||
      compare_types(const1->value, const2->value))
    // TODO: allow for numeric value comparison, some day
    return true;
  else
    return false;
}

bool
MrsComparator::compare_types(std::string t1,
                             std::string t2) {
  if (t1.compare(t2) == 0)
    return true;
  if (_type == 1) { // subsumption
    type_t type1 = lookup_type(t1);
    type_t type2 = lookup_type(t2);
    if (type1 != -1 && type2 != -1 &&
        subtype(type1, type2))
      return true;
    // TODO VPM external types?
  }
  return false;
}

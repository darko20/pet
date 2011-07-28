/* -*- Mode: C++ -*- */

#ifndef _MRS_H_
#define _MRS_H_

#include "pet-config.h"

#include "dag.h"

#include <list>
#include <vector>
#include <map>
#include <string>
#include <iostream>

void mrs_init(const std::string &vpm_file);
void mrs_finalize();

namespace mrs {

struct ltfeat {
  bool operator()(const std::string feat1, const std::string feat2) const;
};

struct ltextra {
  bool operator()(const std::string feat1, const std::string feat2) const;
};

class tValue {
public:

  virtual ~tValue() {
  }

};

class tConstant : public tValue {
public:
  tConstant() {
  }

  tConstant(std::string v) : value(v) {
  }

  std::string value;

};

class tBaseVar : public tValue {
public:

  tBaseVar() {
  }

  tBaseVar(std::string t) : type(t) {
  }
  std::string type;
  std::map<std::string,std::string> extra; // extra agreements
};

class tVar : public tBaseVar {
public:
  tVar() {
  }

  tVar(int vid) : id(vid) {
  }

  tVar(int vid, std::string t) : tBaseVar(t), id(vid) {
  }

  tVar(int vid, dag_node* dag, bool indexing);

  int id;

};

class tHCons {
public:

  tHCons() {
  }

  tHCons(struct dag_node* dag, class tBaseMRS* mrs=NULL);

  tHCons(class tBaseMRS* mrs);

  tHCons(std::string hreln, class tBaseMRS* mrs);

  virtual ~tHCons() {
  }

  enum tHConsRelType {QEQ, LHEQ, OUTSCOPES};
  tHConsRelType relation;
  tVar* scarg;
  tVar* outscpd;

  class tBaseMRS* _mrs;
};

class tBaseRel {
public:
  tBaseRel() {
  }

  tBaseRel(class tBaseMRS *mrs) : _mrs(mrs) {
  }

  virtual ~tBaseRel() {
  }

  std::string pred;
  std::map<std::string,tValue*> flist;

  /* reference to the parent mrs */
  class tBaseMRS *_mrs;

};

class tRel : public tBaseRel {
public:
  tRel() {
  }

  tRel(class tBaseMRS *mrs) : tBaseRel(mrs) {
  }

  tRel(struct dag_node* dag, bool indexing, class tBaseMRS* mrs=NULL);

  virtual ~tRel() {
  }

  std::string str;
  tVar* handel;
  //  int anchor;
  std::map<std::string,tValue*> parameter_strings;
  std::string extra;
  std::string link;
  int cfrom;
  int cto;

  /** collect paramerter strings from the flist */
  void collect_param_strings();

};

/**
 * Basic MRS class
 */
class tBaseMRS {
public:

  tBaseMRS() : _vid_generator(1) {
  }

  virtual ~tBaseMRS();

  /** top handle */
  tVar* top_h;
  /** bag of eps */
  std::list<tBaseRel*> liszt;
  /** qeq constraints */
  std::list<tHCons*> h_cons;
  /** attachment constraints */
  std::list<tHCons*> a_cons;
  // not sure what this is
  std::list<int> vcs;

  /**
   * get the registed variable, returns the pointer to the variable
   * or NULL if not found.
   */
  tVar* find_var(int vid);
  /** register the variable if it has not been */
  void register_var(tVar *var);
  /**
   * locate a variable by its id
   * if the variable is not registered
   * a new variable will be created, registered and returned
   */
  tVar* request_var(int vid, std::string type);
  tVar* request_var(int vid);
  tVar* request_var(std::string type);
  tVar* request_var(struct dag_node* dag);

  /**
   * create a constant and register it
   */
  tConstant* request_constant(std::string value);

  bool valid() { return _valid; }

  // note that the variable names are scoped within one MRS
  std::list<tVar*> _vars;
  std::map<int,tVar*> _vars_map;
  std::list<tConstant*> _constants;
  std::map<dag_node*,mrs::tVar*> _named_nodes;

  bool _valid;

  int _vid_generator;
};

class tMRS : public tBaseMRS {
public:
  tMRS() {
    _vid_generator = 1;
  }

  tMRS(struct dag_node* dag);

  tVar* index;
};

void create_index_property_list(struct dag_node* dag, std::string path, std::map<std::string,std::string>& extra);

  /* check the the compatibility of two types
   * returns true if type1 is compatible with type2
   */
  bool compatible_var_types(std::string type1, std::string type2);

}

struct ltrel {
  bool operator()(const mrs::tRel* ra, const mrs::tRel* rb) const {
    if (ra->pred < rb->pred) // ra->pred < rb->pred
      return true;
    else
      return false;

    /* detailed comparison should be made later
    // now ra->pred == rb->pred, compare parameter_strings
    if (ra->parameter_strings.size() != rb->parameter_strings.size())
      return false;

    // now ra and rb have the same number of parameters, compare fv-pairs
    for (std::map<std::string,tValue*>::iterator fvp = ra->parameter_strings.begin();
         fvp != ra->parameter_strings.end(); ++fvp) {
      std::string feature = (*fvp).first;
      tValue* ra_value = (*fvp).second;
      if (rb->parameter_strings.find(feature) == rb->parameter_string.end())
        return false; // a feature in ra is not in rb, short curcirt
      tValue* rb_value = rb[feature];
      if (dynamic_cast<tConstant*>(ra_value) != NULL &&
          dynamic_cast<tConstant*>(rb_value) != NULL) {
        // both are constants

      } else if (dynamic_cast<tVar/
    }

    // hooray, matched rels!
    return true;
    */
  }
};

#endif

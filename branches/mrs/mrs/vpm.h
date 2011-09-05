/* -*- Mode: C++ -*- */

#ifndef _VPM_H_
#define _VPM_H_

#include "lex-tdl.h"
#include "mrs.h"
#include <string>
#include <list>

#define MAXVPMLINE 1024

void mrs_init(const std::string &vpm_file);
void mrs_finalize();

using namespace mrs;

class tVPM {
  public:
    tVPM() {}
    virtual ~tVPM();
    bool read_vpm(const std::string &filename, std::string id="default");
    tMrs* map_mrs(tMrs* mrs_in, bool forwardp=true);
    tVar* map_variable(tVar* var_in, tMrs* mrs, bool forwardp=true);

    std::string id;
    /** variable type mappings */
    std::list<class tMR*> tms;
    /** parameter mapping rules*/
    std::list<class tPM*> pms;

  private:
    std::map<tVar*, tVar*> _vv_map;
};

/** Parameter mapping */
class tPM {
  public:
    tPM() {}
    virtual ~tPM();
    
    std::list<std::string> lhs;
    std::list<std::string> rhs;
    std::list<class tMR*> pmrs;
};

/** Mapping rule, being it for parameters or types */
class tMR {
  public:
    tMR() : forward(false), backward(false), eqtest(false) {}
    bool test(std::string type, std::list<std::string> values, bool forwardp);

    std::list<std::string> left;
    std::list<std::string> right;
    bool forward;
    bool backward;
    bool eqtest;
};

// from LKB implementation, made obsolete?
bool compatible_var_types(std::string type1, std::string type2);


extern tVPM* vpm;

#endif //_VPM_H_

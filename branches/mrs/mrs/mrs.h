/* -*- Mode: C++ -*- */

#ifndef _MRS_H_
#define _MRS_H_

#include <vector>
#include <map>
#include <string>

namespace mrs {

class tValue {
  public:
    virtual ~tValue() {}
};

class tConstant : public tValue {
  public:
    tConstant() {}
    tConstant(std::string v) : value(v) {}

    std::string value;
};

class tBaseVar : public tValue {
  public:
    tBaseVar() {} //default type to "u"?
    tBaseVar(std::string t) : type(t) {}
    std::string type;
    std::map<std::string,std::string> properties; 
};

class tVar : public tBaseVar {
  public:
    tVar() {}
    tVar(int vid) : id(vid) {}
    tVar(int vid, std::string t) : tBaseVar(t), id(vid) {}

    int id;
};

class tHCons {
  public:
    tHCons() {}
    tHCons(class tBaseMrs* mrs) : relation(QEQ), _mrs(mrs) {}
    tHCons(std::string hreln, class tBaseMrs* mrs);
    // no constructor that takes harg/larg?
    virtual ~tHCons() {}

    enum tHConsRelType {QEQ, LHEQ, OUTSCOPES};
    //make these private?
    tHConsRelType relation;
    tVar* harg;
    tVar* larg;

    class tBaseMrs* _mrs;
};

class tBaseEp {
  public:
    tBaseEp() {}
    tBaseEp(class tBaseMrs *mrs) : _mrs(mrs) {}
    virtual ~tBaseEp() {}

    std::string pred;
    std::map<std::string, tValue*> roles;

  private:
    std::list<tConstant*> _constants;
    /* reference to the parent mrs */
    class tBaseMrs *_mrs;
};

class tEp : public tBaseEp {
  public:
    tEp() {}
    tEp(class tBaseMrs *mrs) : tBaseEp(mrs) {}
    virtual ~tEp() {}

    std::map<std::string,tConstant*> parameter_strings;
    tVar* label;
    std::string link;
    int cfrom;
    int cto;
};

// Basic MRS class
class tBaseMrs {
  public:
    tBaseMrs() : _vid_generator(1) {}
    virtual ~tBaseMrs();

    tVar* find_var(int vid);
    void register_var(tVar *var);
    tVar* request_var(int vid, std::string type);
    tVar* request_var(int vid);
    tVar* request_var(std::string type);
    bool valid() { return _valid; }

    tVar* ltop; // top handle
    std::vector<tBaseEp*> eps; // bag of eps 
    std::vector<tHCons*> hconss; // qeq constraints

  private:
    // note that the variable names are scoped within one MRS
    std::vector<tVar*> _vars;
    std::map<int, tVar*> _vars_map;
    bool _valid;
    int _vid_generator;
};

class tMrs : public tBaseMrs {
  public:
    tMrs() : tBaseMrs() {}
    tMrs(std::string input);

    tVar* index;
  /* to be moved to the dedicated readers 
  private:
    void parseChar(char x, std::string &rest);
    void parseID(const std::string id, std::string &rest);
    tVar *readVar(std::string &rest);
    tConstant *readCARG(std::string &rest);
    void parseProps(tVar *var, std::string &rest);
    std::string readFeature(std::string &rest);
    void parseEP(std::string &rest);
    void parsePred(tEp *ep, std::string &rest);
    bool parseHCONS(std::string &rest);
    std::string readReln(std::string &rest);
  */
};

struct ltep {
  bool operator()(const tEp* ra, const tEp* rb) const {
    if (ra->pred < rb->pred) 
      return true;
    else
      return false;
  }
};

} //namespace mrs


#endif

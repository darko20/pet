/* -*- Mode: C++ -*- */

#ifndef _MRS_READER_H_
#define _MRS_READER_H_

#include <string>
#include <set>
#include "mrs.h"

namespace mrs {

class tMrsReader {
  public:
    tMrsReader(){};
    ~tMrsReader(){};

    virtual tMrs *readMrs(std::string) = 0;
};

class SimpleMrsReader : public tMrsReader {
  public:
    SimpleMrsReader(); //set up label correspondences (CARG, LBL, etc)
    ~SimpleMrsReader(){};

    tMrs *readMrs(std::string);

  private:
    std::set<std::string> _constant_roles;
    std::string _lblstr;

    void parseChar(char x, std::string &rest);
    void parseID(const std::string id, std::string &rest);
    tVar *readVar(tMrs *mrs, std::string &rest);
    void parseProps(tVar *var, std::string &rest);
    std::string readFeature(std::string &rest);
    void parseEP(tMrs *mrs, std::string &rest);
    void parsePred(tEp *ep, std::string &rest);
    tConstant *readCARG(std::string &rest);
    bool parseHCONS(tMrs *mrs, std::string &rest);
    std::string readReln(std::string &rest);
};

class XmlMrsReader : public tMrsReader {
public:
  XmlMrsReader(){};
  ~XmlMrsReader(){};

  tMrs *readMrs(std::string);
};

}

#endif

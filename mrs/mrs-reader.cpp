/* -*- Mode: C++ -*- */

#ifdef MRS_ONLY
#include "mrs-errors.h"
#else
#include "pet-config.h"
#include "errors.h"
#endif
#include "mrs-reader.h"

#ifdef HAVE_XML
#include "mrs-handler.h"
#include <xercesc/framework/MemBufInputSource.hpp>
XERCES_CPP_NAMESPACE_USE
#endif

#include <sstream>
#include <boost/algorithm/string.hpp>
#include <iostream>

namespace mrs {

SimpleMrsReader::SimpleMrsReader() {
  _constant_roles.insert("CARG");
  _lblstr = "LBL";
}

tMrs *SimpleMrsReader::readMrs(std::string input){

  tMrs *mrs = new tMrs();
  std::string rest = input;

  parseChar('[', rest);
  parseID("LTOP:", rest);
  mrs->ltop = readVar(mrs, rest);
  parseID("INDEX:", rest);
  mrs->index = readVar(mrs, rest);
  if (rest.at(0) == '[')
    parseProps(mrs->index, rest);
  parseID("RELS:", rest);
  parseChar('<', rest);
  while (rest.at(0) == '[')
    parseEP(mrs, rest);
  parseChar('>', rest);
  parseID("HCONS:", rest);
  parseChar('<', rest);
  while (parseHCONS(mrs, rest)) ;
  parseChar('>', rest);
  parseChar(']', rest);
  if (!rest.empty())
    throw tError("ignoring trailing data: \"" + rest + "\"");

  return mrs;
}

void removeWhitespace(std::string &rest) {
  while (!rest.empty() && isspace(rest.at(0)))
    rest.erase(0,1);
}

// expects and removes char x at start of string rest,
// skipping (and removing) whitespace before and after x
void SimpleMrsReader::parseChar(char x, std::string &rest) {
  removeWhitespace(rest);
  if (!rest.empty() && rest.at(0) == x) {
    rest.erase(0,1);
    removeWhitespace(rest);
  } else {
    if (rest.empty())
      throw tError("Reached end of string while looking for " + x);
    else
      throw tError("Ill-formed MRS. Got \"" + rest + "\", looking for " + x);
  }
}

// expects and removes string id at start of rest, removes trailing whitespace
void SimpleMrsReader::parseID(const std::string id, std::string &rest) {
  if (rest.length() < id.length() 
    || ! boost::iequals(rest.substr(0,id.length()), id)) 
    throw tError("Ill-formed MRS: no " + id + " at \"" + rest + "\".");
  rest.erase(0,id.length());
  removeWhitespace(rest);
}

// parses variable, and registers it with the MRS, if not already registered
tVar *SimpleMrsReader::readVar(tMrs *mrs, std::string &rest) {
  tVar *var;
  std::string vtype;
  std::string vidstring;
  int vid;
  if (isalpha(rest.at(0))) {
    vtype = rest.substr(0,1);
    rest.erase(0,1);
    while(!rest.empty() && !isspace(rest.at(0)) && !isdigit(rest.at(0))) {
      vtype += rest.at(0);
      rest.erase(0,1);
    }
    if (!isdigit(rest.at(0))) {
      throw tError("Ill-formed variable at \"" + rest + "\"");
    }
    while (!rest.empty() && isdigit(rest.at(0))) {
      vidstring += rest.at(0);
      rest.erase(0,1);
    }
    std::istringstream vidstream(vidstring);
    vidstream >> vid;
    var = mrs->request_var(vid, vtype);
  }
  else {
    throw tError("Couldn't find a variable at \"" + rest + "\"");
  }
  removeWhitespace(rest);
  return var;
}

// read property list and add to variable var
void SimpleMrsReader::parseProps(tVar *var, std::string &rest) {
  parseChar('[', rest);
  if (!isalpha(rest.at(0))) {
    throw tError("Ill-formed MRS. Expecting variable type at \"" + rest +"\"");
  }
  // could check that variable type matches var, but currently discarding
  rest.erase(0,1);
  while(!rest.empty() && !isspace(rest.at(0)) && !isdigit(rest.at(0)))
    rest.erase(0,1);
  removeWhitespace(rest);

  std::string prop = readFeature(rest);
  boost::to_upper(prop);
  while (!prop.empty()) {
    std::string val;
    // needs to be Unicode-safe (assuming properties can have non-ascii vals)
    // can values have spaces in them?
    while (!rest.empty() && !isspace(rest.at(0))) {
      val += rest.at(0);
      rest.erase(0,1);
    }
    var->properties[prop] = val;
    removeWhitespace(rest);
    prop = readFeature(rest);
    boost::to_upper(prop);
  }
  parseChar(']', rest);
}

// feature is a feature label, finishing with ':'. returns the label, minus :
std::string SimpleMrsReader::readFeature(std::string &rest) {
  std::string f;
  // can features (property or role names) be non-ascii?
  while (!rest.empty() && !isspace(rest.at(0))
    && rest.at(0) != '[' && rest.at(0) != ']') {
    if (rest.at(0) == ':') {
      rest.erase(0,1);
      removeWhitespace(rest);
      return f;
    }
    f += rest.at(0);
    rest.erase(0,1);
  }
  std::string fail;
  return fail;
}

void SimpleMrsReader::parseEP(tMrs *mrs, std::string &rest) {
  parseChar('[', rest);
  tEp *ep = new tEp(mrs);
  parsePred(ep, rest); //set pred name, link string, cfrom, cto
  std::string role = readFeature(rest);
  boost::to_upper(role);
  while (!role.empty()) {
    if (_constant_roles.count(role)) { //val should be a constant
      tConstant *val = readCARG(ep, rest);
      if (val != NULL) {//don't record uninstantiated cargs (*TOP* etc)
        ep->roles[role] = val;
      }
    } else { //val should be a var
      tVar *val = readVar(mrs, rest);
      if (rest.at(0) == '[')
        parseProps(val, rest);
      ep->roles[role] = val;
      if (role == _lblstr)
        ep->label = val;
      // special handling for ARG0?
    }
    role = readFeature(rest);
    boost::to_upper(role);
  }
  parseChar(']', rest);
  mrs->eps.push_back(ep);
}

void SimpleMrsReader::parsePred(tEp *ep, std::string &rest){
  std::string name, span;
  int from, to;
  if (rest.at(0) == '"') { //real pred
    rest.erase(0,1);
    // needs to be Unicode-safe
    while (!rest.empty() && !isspace(rest.at(0))) {
      if (rest.at(1) == '"') {
        if (rest.at(0) != '\\') {
          name += rest.at(0);
          rest.erase(0,1);
          break;
        }
      }
      name += rest.at(0);
      rest.erase(0,1);
    }
    //we should have broken, with rest[0] == '"'
    if (!rest.empty() && rest.at(0) != '"') {
      throw tError("Unterminated quoted string at \"" + rest + "\"."); 
    } else {
      if (!rest.empty())
        rest.erase(0,1); //erase terminating quote, span should be next
    }
  } else { //grammar pred
    //needs to be Unicode-safe?
    while (!rest.empty() && !isspace(rest.at(0)) && rest.at(0) != '<') {
      name += rest.at(0);
      rest.erase(0,1);
    }
  }
  ep->pred = name;
  if (rest.at(0) == '<') { //link
    while (!rest.empty() && isgraph(rest.at(0)) && rest.at(0) != '>') {
      span += rest.at(0);
      rest.erase(0,1);
    }
    if (rest.at(0) == '>') {
      span += rest.at(0);
      rest.erase(0,1);
    } else {
      throw tError("Unterminated span at \"" + rest + "\".");
    }
    unsigned int colon = span.find(':');
    if (colon == std::string::npos) {
      if (span == "<>") {//don't fall over on empty spans (grammar bugs)
        from = -1;
        to = -1;
      } else {
        throw tError("Ill-formed span \"" + span + "\".");
      }
    } else {
      std::istringstream fromstr(span.substr(1, colon-1));
      fromstr >> from;
      std::istringstream tostr(span.substr(colon+1));
      tostr >> to;
    }
    ep->link = span;
    ep->cfrom = from;
    ep->cto = to;
  } else { //no link
    ep->link = std::string("<-1:-1>");
    ep->cfrom = -1;
    ep->cto = -1;
  }
  removeWhitespace(rest);
}

tConstant *SimpleMrsReader::readCARG(tEp *ep, std::string &rest) {
  std::string carg;
  if (rest.at(0) == '"') {// quoted string, anything goes inside
    rest.erase(0,1);
    //needs to be Unicode-safe
    while (!rest.empty()) {
      if (rest.at(1) == '"') {
        if (rest.at(0) != '\\') {
          carg += rest.at(0);
          rest.erase(0,1);
          break;
        }
      }
      carg += rest.at(0);
      rest.erase(0,1);
    }
    //we should have broken, with rest[0] == '"'
    if (!rest.empty() && rest.at(0) != '"') {
      throw tError("Unterminated quoted string at \"" + rest + "\".");
    } else {
      if (!rest.empty())
        rest.erase(0,1); //erase terminating quote
    }
  } else { //unquoted string, not real carg, remove and ignore  
    while (!rest.empty() && !isspace(rest.at(0))) 
      rest.erase(0,1);
  }
  if (carg.empty())
    return NULL;
  else
    return ep->request_constant(carg);
}

bool SimpleMrsReader::parseHCONS(tMrs *mrs, std::string &rest) {
  if (isalpha(rest.at(0))) {
    tVar *lhdl = readVar(mrs, rest);
    std::string reln = readReln(rest);
    tVar *rhdl = readVar(mrs, rest);
    tHCons *hcons = new tHCons(reln, mrs);
    hcons->harg = lhdl;
    hcons->larg = rhdl;
    mrs->hconss.push_back(hcons);

    return true;
  }
  return false;
}

std::string SimpleMrsReader::readReln(std::string &rest) {
  std::string reln;
  while (!rest.empty() && !isspace(rest.at(0))) {
    reln += rest.at(0);
    rest.erase(0,1);
  }
  removeWhitespace(rest);
  return reln;
}

#ifdef HAVE_XML
tMrs* XmlMrsReader::readMrs(std::string input) {
  MrsHandler mrs_handler(false);
  std::string buffer = input;
  MemBufInputSource xml_input((const XMLByte *) buffer.c_str(), 
			     buffer.length(), "STDIN");
  parse_file(xml_input, &mrs_handler);
  return mrs_handler.mrss().front();
}
#endif // HAVE_XML

}

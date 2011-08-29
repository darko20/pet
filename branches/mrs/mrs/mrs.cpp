#include "mrs.h"
#include "errors.h"
#include <sstream>

namespace mrs {

  tBaseMrs::~tBaseMrs() {
  for (std::vector<tBaseEp*>::iterator ep = eps.begin();
       ep != eps.end(); ++ep)
    delete *ep;
  for (std::vector<tHCons*>::iterator hcons = hconss.begin();
       hcons != hconss.end(); ++hcons)
    delete *hcons;
  for (std::vector<tVar*>::iterator var = _vars.begin();
       var != _vars.end(); ++var)
    delete *var;
}

tVar* tBaseMrs::find_var(int vid) {
  tVar* var = NULL;
  if (_vars_map.find(vid) != _vars_map.end())
    var = _vars_map[vid];
  return var;
}

void tBaseMrs::register_var(tVar *var) {
  if (find_var(var->id) == NULL) {
    _vars.push_back(var);
    _vars_map[var->id] = var;
  }
}

tVar* tBaseMrs::request_var(int vid, std::string type) {
  tVar* var;
  if (_vars_map.find(vid) != _vars_map.end()) {
    var = _vars_map[vid];
    var->type = type; //reassign type? really?
  }
  else {
    var = new tVar(vid, type);
    register_var(var);
  }
  return var;
}

tVar* tBaseMrs::request_var(int vid) {
  tVar* var;
  if (_vars_map.find(vid) != _vars_map.end())
    var = _vars_map[vid];
  else {
    var = new tVar(vid);
    register_var(var);
  }
  return var;
}

tVar* tBaseMrs::request_var(std::string type) {
  tVar* var = new tVar(_vid_generator++, type);
  register_var(var);
  return var;
}

tHCons::tHCons(std::string hreln, tBaseMrs* mrs) : _mrs(mrs) {
  if (hreln == "qeq") {
    relation = QEQ;
  } else if (hreln == "lheq") {
    relation = LHEQ;
  } else if (hreln == "outscopes") {
    relation = OUTSCOPES;
  } else {
    throw tError("Invalid hcons relation: \"" + hreln + "\"");
  }
}

tMrs::tMrs(std::string input) : tBaseMrs() {
  std::string rest = input;

  //parse MRS simple string
  parseChar('[', rest);
  parseID("LTOP:", rest);
  ltop = readVar(rest);
  parseID("INDEX:", rest);
  index = readVar(rest);
  if (rest.at(0) == '[')
    parseProps(index, rest);
  parseID("RELS:", rest);
  parseChar('<', rest);
  while (rest.at(0) == '[')
    parseEP(rest);
  parseChar('>', rest);
  parseID("HCONS:", rest);
  parseChar('<', rest);
  while (parseHCONS(rest)) ;
  parseChar('>', rest);
  parseChar(']', rest);
  if (!rest.empty())
    throw tError("ignoring trailing data: \"" + rest + "\"");
}


/* to be moved to the dedicated MRS readers 

void removeWhitespace(std::string &rest) {
  while (!rest.empty() && isspace(rest.at(0)))
    rest.erase(0,1);
}

// expects and removes char x at start of string rest,
// skipping (and removing) whitespace before and after x
void tMrs::parseChar(char x, std::string &rest) {
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
void tMrs::parseID(const std::string id, std::string &rest) {
  if (rest.length() < id.length() || rest.substr(0,id.length()) != id) 
    throw tError("Ill-formed MRS: no " + id + " at \"" + rest + "\".");
  rest.erase(0,id.length());
  removeWhitespace(rest);
}

// parses variable, and registers it with the MRS, if not already registered
tVar *tMrs::readVar(std::string &rest) {
  tVar *var;
  std::string vtype;
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
      vid += rest.at(0);
      rest.erase(0,1);
    }
    var = request_var(vid, vtype);
  }
  else {
    throw tError("Couldn't find a variable at \"" + rest + "\"");
  }
  removeWhitespace(rest);
  return var;
}

// read property list and add to variable var
void tMrs::parseProps(tVar *var, std::string &rest) {
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
  }
  parseChar(']', rest);
}

// feature is a feature label, finishing with ':'. returns the label, minus :
std::string tMrs::readFeature(std::string &rest) {
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

void tMrs::parseEP(std::string &rest) {
  parseChar('[', rest);
  tEp *ep = new tEp(this);
  parsePred(ep, rest); //set pred name, link string, cfrom, cto
  std::string role = readFeature(rest);
  while (!role.empty()) {
    if (role == "CARG") { //val should be a constant
      tConstant *val = readCARG(rest);
      if (val != NULL) {//don't record uninstantiated cargs (*TOP* etc)
        ep->roles[role] = val;
        ep->carg = val->value;
      }
    } else { //val should be a var
      tVar *val = readVar(rest);
      if (rest.at(0) == '[')
        parseProps(val, rest);
      ep->roles[role] = val;
      if (role == "LBL")
        ep->label = val;
      // special handling for ARG0?
    }
    role = readFeature(rest);
  }
  parseChar(']', rest);
  eps.push_back(ep);
}

void tMrs::parsePred(tEp *ep, std::string &rest){
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
    int colon = span.find(':');
    if (colon == std::string::npos) 
      throw tError("Ill-formed span \"" + span + "\".");
    std::istringstream spanstr(span.substr(1, colon-1));
    spanstr >> from;
    spanstr.str(span.substr(colon+1));
    spanstr >> to;
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

tConstant *tMrs::readCARG(std::string &rest) {
  std::string carg;
  if (rest.at(0) == '"') {// quoted string, anything goes inside
    rest.erase(0,1);
    //needs to be Unicode-safe
    while (!rest.empty() && !isspace(rest.at(0))) {
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
    return new tConstant(carg);
}

bool tMrs::parseHCONS(std::string &rest) {
  if (isalpha(rest.at(0))) {
    tVar *lhdl = readVar(rest);
    std::string reln = readReln(rest);
    tVar *rhdl = readVar(rest);
    tHCons *hcons = new tHCons(reln, this);
    hcons->harg = lhdl;
    hcons->larg = rhdl;
    hconss.push_back(hcons);

    return true;
  }
  return false;
}

std::string tMrs::readReln(std::string &rest) {
  std::string reln;
  while (!rest.empty() && !isspace(rest.at(0))) {
    reln += rest.at(0);
    rest.erase(0,1);
  }
  removeWhitespace(rest);
  return reln;
}

*/

tEp::~tEp() {
  for (std::list<tConstant*>::iterator constant = _constants.begin();
       constant != _constants.end(); ++constant)
    delete *constant;
}

} // namespace mrs


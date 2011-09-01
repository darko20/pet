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

  /* this is reader/extracter specific and should be removed from the general MRS class 
tVar* tBaseMrs::request_var(std::string type) {
  tVar* var = new tVar(_vid_generator++, type);
  register_var(var);
  return var;
}
  */

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

tEp::~tEp() {
  for (std::map<std::string, tConstant*>::iterator 
      constant = parameter_strings.begin();
      constant != parameter_strings.end(); ++constant)
    delete constant->second;
}

} // namespace mrs


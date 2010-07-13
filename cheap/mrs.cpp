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

#include "mrs.h"

#include "logging.h"
#include "settings.h"
#include "types.h"
#include "utility.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

int variable_generator = 0;
extern settings *cheap_settings;


namespace mrs {

/**
 * Escape XML delimiters.
 */
std::string
xml_escape(std::string s) {
  boost::replace_all(s, "<", "&lt;");
  boost::replace_all(s, ">", "&gt;");
  boost::replace_all(s, "&", "&amp;");
  boost::replace_all(s, "'", "&apos;");
  boost::replace_all(s, "\"", "&quot;");
  return s;
}
 
tBaseMRS::~tBaseMRS() {
  for (std::list<tBaseRel*>::iterator rel = liszt.begin(); 
       rel != liszt.end(); rel ++)
    delete *rel;
  for (std::list<tHCons*>::iterator hcons = h_cons.begin();
       hcons != h_cons.end(); hcons ++)
    delete *hcons;
  for (std::list<tHCons*>::iterator acons = a_cons.begin();
       acons != a_cons.end(); acons ++)
    delete *acons;
  for (std::list<tVar*>::iterator var = _vars.begin();
       var != _vars.end(); var ++)
    delete *var;
  for (std::list<tConstant*>::iterator constant = _constants.begin();
       constant != _constants.end(); constant ++)
    delete *constant;
}

tVar* tBaseMRS::
find_var(int vid) {
  tVar* var = NULL;
  if (_vars_map.find(vid) != _vars_map.end())
    var = _vars_map[vid];
  return var;
}

void tBaseMRS::
register_var(tVar *var) {
  if (find_var(var->id) == NULL) {
    _vars.push_back(var);
    _vars_map[var->id] = var;
  }
}

tVar* tBaseMRS::
request_var(int vid, std::string type) {
  tVar* var;
  if (_vars_map.find(vid) != _vars_map.end()) {
    var = _vars_map[vid];
    var->type = type;
  }
  else {
    var = new tVar(vid, type); 
    register_var(var);
  }
  return var;
}

tVar* tBaseMRS::
request_var(int vid) {
  tVar* var;
  if (_vars_map.find(vid) != _vars_map.end())
    var = _vars_map[vid];
  else {
    var = new tVar(vid); 
    register_var(var);
  }
  return var;
}

tVar* tBaseMRS::
request_var(std::string type) {
  tVar* var = new tVar(_vid_generator++, type);
  register_var(var);
  return var;
}

tVar* tBaseMRS::
request_var(struct dag_node* dag) {
  if (_named_nodes.find(dag) != _named_nodes.end())
    return _named_nodes[dag];
  else {
    tVar* newvar = new tVar(_vid_generator++, dag, false);
    register_var(newvar);
    _named_nodes[dag] = newvar;
    return newvar;
  }
}


tConstant* tBaseMRS::
request_constant(std::string value) {
  tConstant* constant = new tConstant(value);
  _constants.push_back(constant);
  return constant;
}

tPSOA::tPSOA(struct dag_node* dag) {
  dag = dag_expand(dag); // expand the fs to be well-formed 
  _vid_generator = 1;
  struct dag_node* init_sem_dag = FAIL;
  init_sem_dag = dag_get_path_value(dag, cheap_settings->req_value("mrs-initial-semantics-path"));
  if (init_sem_dag == FAIL) {
    _valid = false;
    LOG(logMRS, ERROR, "no mrs-initial-semantics-path found.");
    return;
  }

  // extract top-h
  struct dag_node* d = FAIL;
  char* psoa_top_h_path = cheap_settings->value("mrs-psoa-top-h-path");
  if (!psoa_top_h_path || strcmp(psoa_top_h_path, "") == 0) 
    top_h = request_var("h");
  else { 
    d = dag_get_path_value(init_sem_dag, psoa_top_h_path);
    if (d == FAIL) {
      _valid = false;
      LOG(logMRS, ERROR, "no mrs-psoa-top-h-path found.");
      return;
    }
    top_h = request_var(d);
  }
  
  // extract index
  d = dag_get_path_value(init_sem_dag, cheap_settings->req_value("mrs-psoa-index-path"));
  if (d == FAIL) {
    _valid = false;
    LOG(logMRS, ERROR, "no mrs-psoa-index-path found.");
    return;
  }
  index = request_var(d);
  
  // extract liszt
  d = dag_get_path_value(init_sem_dag, cheap_settings->req_value("mrs-psoa-liszt-path"));
  if (d == FAIL) {
    _valid = false;
    LOG(logMRS, ERROR, "no mrs-psoa-index-path found.");
    return;
  }
  int n = 0;
  std::list<struct dag_node*> rel_list = dag_get_list(d);
  for (std::list<dag_node*>::iterator iter = rel_list.begin();
       iter != rel_list.end(); iter ++, n ++) {
    if (*iter == FAIL) {
      _valid = false;
      LOG(logMRS, ERROR, boost::format("no relation %d") % n);
      liszt.clear();
      return;
    }
    liszt.push_back(new tRel(*iter, false, this));
  }
  
  // extract hcons
  d = dag_get_path_value(init_sem_dag, cheap_settings->req_value("mrs-psoa-rh-cons-path"));
  if (d == FAIL) {
    _valid = false;
    LOG(logMRS, ERROR, "no mrs-psoa-rh-cons-path found.");
    return;
  }
  n = 0;
  std::list<struct dag_node*> hcons_list = dag_get_list(d);
  for (std::list<dag_node*>::iterator iter = hcons_list.begin();
       iter != hcons_list.end(); iter ++, n ++) {
    if (*iter == FAIL) {
      _valid = false;
      LOG(logMRS, ERROR, boost::format("no hcons %d") % n);
      h_cons.clear();
      return;
    }
    h_cons.push_back(new tHCons(*iter, this));
  }
  // extract acons
  
  _valid = true;
}

void tPSOA::
print(std::ostream &out) {
  out << "<mrs>\n";
  out << boost::format("<label vid='%d'/>") % top_h->id;
  out << boost::format("<var vid='%d'/>\n") % index->id;
  for (std::list<tBaseRel*>::iterator rel = liszt.begin();
       rel != liszt.end(); rel ++) {
    (*rel)->print(out);
  }
  for (std::list<tHCons*>::iterator hcons = h_cons.begin();
       hcons != h_cons.end(); hcons ++) {
    (*hcons)->print(out);
  }
  for (std::list<tHCons*>::iterator acons = a_cons.begin();
       acons != a_cons.end(); acons ++) {
    (*acons)->print(out);
  }
  out << "</mrs>\n";
} 

tHCons::tHCons(struct dag_node* dag, class tBaseMRS* mrs) : _mrs(mrs) {
  // _fixme_ : for the moment there is only one type of H-Cons: QEQ
  relation = QEQ;
  
  struct dag_node* d = dag_get_path_value(dag, cheap_settings->req_value("mrs-sc-arg-feature"));
  if (d == FAIL) {
    LOG(logMRS, ERROR, "no mrs-sc-arg-feature found.");
    return;
  }
  scarg = _mrs->request_var(d);
  
  d = dag_get_path_value(dag, cheap_settings->req_value("mrs-outscpd-feature"));
  if (d == FAIL) {
    LOG(logMRS, ERROR, "no mrs-outscpd-feature found.");
    return;
  }
  outscpd = _mrs->request_var(d);
}

tHCons::tHCons(class tBaseMRS* mrs): relation(QEQ), _mrs(mrs) {
}

tHCons::tHCons(std::string hreln, tBaseMRS* mrs) : _mrs(mrs) {
  if (hreln == "qeq") {
    relation = QEQ;
  } else if (hreln == "lheq") {
    relation = LHEQ;
  } else if (hreln == "outscopes") {
    relation = OUTSCOPES;
  }
}

tRel::tRel(struct dag_node* dag, bool indexing, class tBaseMRS* mrs) : tBaseRel(mrs) {
  struct dag_node* d = FAIL;

  // do not instantiate the handel when indexing the lexical entries
  if (!indexing) {
    d = dag_get_path_value(dag, cheap_settings->req_value("mrs-rel-handel-path"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-rel-handel-path found.");
      return;
    }
    handel = _mrs->request_var(d);
  }
  
  // get the relation name
  d = dag_get_path_value(dag, cheap_settings->req_value("mrs-rel-name-path"));
  if (d != FAIL) {
    pred = type_name(d->type);
  }
  
  // ignore the str for the moment
  
  // get flist (fvpairs)
  dag_arc* attribute;
  for (attribute = dag->arcs; attribute != NULL; attribute = attribute->next) {
    char* feature = attrname[attribute->attr];
    if (strcmp(feature,
               cheap_settings->req_value("mrs-rel-handel-path")) != 0 &&
        strcmp(feature,
               cheap_settings->req_value("mrs-rel-name-path")) != 0 &&
        !cheap_settings->member("mrs-ignored-sem-features", feature)) {
      tValue* value;
      if (cheap_settings->member("mrs-value-feats", feature)) {
        value = new tConstant(type_name(attribute->val->type));
      } else {
        if (indexing) {// ??? what's _mrs in indexing?
          value = new tVar(_mrs->_vid_generator++, attribute->val, indexing);
        } else {
          value = _mrs->request_var(attribute->val);
        }
      }
      flist[feature] = value;
    }
  }
  
  // collect parameter strings
  collect_param_strings();
  
  // _todo_ ignore cfrom/cto for the moment
  d = dag_get_path_value(dag, cheap_settings->req_value("mrs-rel-cfrom-feature"));
  if (d != FAIL) {
    if ((d->type == BI_TOP) || (d->type == BI_STRING))
      cfrom = -1;
    else
      cfrom = strtoint(type_name(d->type), "Invalid CFROM value", true);
  }
  d = dag_get_path_value(dag, cheap_settings->req_value("mrs-rel-cto-feature"));
  if (d != FAIL) {
    if ((d->type == BI_TOP) || (d->type == BI_STRING))
      cto = -1;
    else
      cto = strtoint(type_name(d->type), "Invalid CTO value", true);
  }
}

void tRel::
collect_param_strings() {
  for (std::map<std::string,tValue*>::iterator iter = flist.begin();
       iter != flist.end(); iter ++) {
    if (cheap_settings->member("mrs-value-feats", (*iter).first.c_str())) {
      parameter_strings[(*iter).first] = (*iter).second;
    }
  }
}

void tRel::
print(std::ostream &out) {
  out << boost::format("<ep cfrom='%d' cto='%d'>") % cfrom % cto;
  if (pred[0] == '"')
    out << "<spred>" << xml_escape(pred.substr(1,pred.length()-2)) << "</spred>";
  else {
    char* uppred = new char[pred.length()+1];
    strtoupper(uppred, pred.c_str());
    out << "<pred>" << xml_escape(uppred) << "</pred>"; //pred;
    delete uppred;
  }
  out << boost::format("<label vid='%d'/>") % handel->id;
  std::list<std::string> feats;
  for (std::map<std::string,tValue*>::iterator fvpair = flist.begin();
       fvpair != flist.end(); fvpair ++) 
    feats.push_back((*fvpair).first);
  feats.sort(ltfeat());
  for (std::list<std::string>::iterator feat = feats.begin();
       feat != feats.end(); feat ++) {
    out << std::endl << "<fvpair>";
    out << "<rargname>" << xml_escape(*feat) << "</rargname>";
    flist[*feat]->print_full(out);
    out << "</fvpair>";
  }

  out << "</ep>" << std::endl;
}

void tHCons::
print(std::ostream &out) {
  out << "<hcons hreln='";
  if (relation == QEQ)
    out << "qeq";
  else if (relation == LHEQ)
    out << "lheq";
  else if (relation == OUTSCOPES)
    out << "outscopes";
  out << "'>";
  out << "<hi>";
  scarg->print(out);
  out << "</hi>";
  out << "<lo>";
  outscpd->print(out);
  out << "</lo>";
  out << "</hcons>\n";
}

void tConstant::
print(std::ostream &out) {
  if (value[0] == '"')
    out << "<constant>" << xml_escape(value.substr(1,value.length()-2))
        << "</constant>";
  else
    out << "<constant>" << xml_escape(value) << "</constant>";
}

void tConstant::
print_full(std::ostream &out) {
  print(out);
}

tVar::tVar(int vid, struct dag_node* dag, bool indexing) : id(vid) {
  if (dag != FAIL) {

    /* 
       this has been deprecated by variable type mapping in the VPM  (yi jan-09)
    //   determine variable type
    type_t t = dag_type(dag);
    
    type_t event_type = lookup_type(cheap_settings->req_value("mrs-event-type"));
    type_t event_or_index_type = lookup_type(cheap_settings->req_value("mrs-event-or-index-type"));
    type_t non_expl_ind_type = lookup_type(cheap_settings->req_value("mrs-non-expl-ind-type"));
    type_t deg_ind_type = lookup_type(cheap_settings->req_value("mrs-deg-ind-type"));
    type_t handle_type = lookup_type(cheap_settings->req_value("mrs-handle-type"));
    type_t ref_ind_type = lookup_type(cheap_settings->req_value("mrs-ref-ind-type"));
    type_t non_event_type = lookup_type(cheap_settings->req_value("mrs-non-event-type"));

    if (subtype(t, event_type)) type = "e";
    else if (event_type != -1 && subtype(t, ref_ind_type)) type = "x";
    else if (non_expl_ind_type != -1 && subtype(t, non_expl_ind_type)) type = "i";
    else if (deg_ind_type != -1 && subtype(t, deg_ind_type)) type = "d";
    else if (handle_type != -1 && subtype(t, handle_type)) type = "h";
    else if (non_event_type != -1 && subtype(t, non_event_type)) type = "p";
    else if (event_or_index_type != -1 && subtype(t, event_or_index_type)) type = "i";
    else type = "u"; 
    */

    type = typenames[dag_type(dag)];
    
    // create index property list
    create_index_property_list(dag, "", extra);

  }
      
}

void tVar::
print(std::ostream &out) {
  out << boost::format("<var vid='%d' sort='%s'></var>") % id % xml_escape(type);
}

void tVar::
print_full(std::ostream &out) {
  out << boost::format("<var vid='%d' sort='%s'>") % id % xml_escape(type);
  // _todo_ this should be adapted to handle the feature priority
  std::list<std::string> feats;
  for (std::map<std::string,std::string>::iterator extrapair = extra.begin();
       extrapair != extra.end(); extrapair ++)
    feats.push_back((*extrapair).first);
  feats.sort(ltextra());
  for (std::list<std::string>::iterator feat = feats.begin();
       feat != feats.end(); feat ++) {
    char* upvalue = new char[extra[*feat].length()+1];
    strtoupper(upvalue, extra[*feat].c_str());
    out << std::endl << "<extrapair><path>" << xml_escape(*feat)
          << "</path><value>"
          << xml_escape(upvalue) << "</value></extrapair>";
    delete upvalue;
  }
  out << "</var>";
}

void
create_index_property_list(dag_node* dag, std::string path, std::map<std::string,std::string>& extra) {
  dag_arc* attribute;
  std::string currpath = path == "" ? path : path+"."; 
  for (attribute = dag->arcs; attribute != NULL; attribute = attribute->next) { // for each attribute under current dag root node
    if (cheap_settings->member("mrs-ignored-extra-features", 
                               attrname[attribute->attr]))
      continue;
    if (attribute->val->arcs == NULL) { // value of the attribute is atomic
      extra[currpath+attrname[attribute->attr]] = type_name(attribute->val->type);
    } else { // collect property recursively
      create_index_property_list(attribute->val, 
                                 currpath+attrname[attribute->attr], extra);
    }
  }
}

  /* this was from the LKB implementation, and is now largely
    deprecated by the extension of variable type mapping in VPM */
bool compatible_var_types(std::string type1, std::string type2) {
  if (type1 == type2)
    return true;
  if (type1 == "u" || type2 == "u")
    return true;
  if ((type1 == "e" && type2 == "i") ||
      (type1 == "i" && type2 == "e"))
    return true;
  if ((type1 == "x" && type2 == "i") ||
      (type1 == "i" && type2 == "x"))
    return true;
  return false;
}

bool ltfeat::
operator()(const std::string feat1, const std::string feat2) const {
  bool f1in = cheap_settings->member("mrs-feat-priority-list", feat1.c_str());
  bool f2in = cheap_settings->member("mrs-feat-priority-list", feat2.c_str());
  if (f1in && !f2in)
    return true;
  if (!f1in && f2in)
    return false;
  if (f1in && f2in) {
    setting* plist = cheap_settings->lookup("mrs-feat-priority-list");
    for (int i = 0; i < plist->n; i ++) 
      if (strcasecmp(plist->values[i], feat1.c_str()) == 0)
        return true;
      else if (strcasecmp(plist->values[i], feat2.c_str()) == 0)
        return false;
  }
  return feat1 < feat2;
}

bool ltextra::
operator()(const std::string feat1, const std::string feat2) const {
  bool f1in = cheap_settings->member("mrs-extra-priority-list", feat1.c_str());
  bool f2in = cheap_settings->member("mrs-extra-priority-list", feat2.c_str());
  if (f1in && !f2in)
    return true;
  if (!f1in && f2in)
    return false;
  if (f1in && f2in) {
    setting* plist = cheap_settings->lookup("mrs-extra-priority-list");
    for (int i = 0; i < plist->n; i ++) 
      if (strcasecmp(plist->values[i], feat1.c_str()) == 0)
        return true;
      else if (strcasecmp(plist->values[i], feat2.c_str()) == 0)
        return false;
  }
  return feat1 < feat2;
}

} // namespace mrs

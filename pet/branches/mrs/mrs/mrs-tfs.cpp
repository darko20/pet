/* -*- Mode: C++ -*- */

#include "mrs-tfs.h"

#include "logging.h"
#include "settings.h"
#include "types.h"
#include "utility.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

extern settings *cheap_settings;

namespace mrs {
  
  tMrs* MrsTfsExtractor::extractMrs(struct dag_node* dag) {
    dag = dag_expand(dag); // expand the fs to be well-formed
    tMrs* mrs = new tMrs();
    struct dag_node* init_sem_dag = FAIL;
    init_sem_dag = dag_get_path_value(dag, cheap_settings->req_value("mrs-initial-semantics-path"));
    if (init_sem_dag == FAIL) {
      LOG(logMRS, ERROR, "no mrs-initial-semantics-path found.");
      delete mrs;
      return NULL;
    }

    // extract top-h
    struct dag_node* d = FAIL;
    const char* psoa_top_h_path = cheap_settings->value("mrs-psoa-top-h-path");
    if (!psoa_top_h_path || strcmp(psoa_top_h_path, "") == 0)
      mrs->ltop = requestVar("handle", mrs);
    else {
      d = dag_get_path_value(init_sem_dag, psoa_top_h_path);
      if (d == FAIL) {
	LOG(logMRS, ERROR, "no mrs-psoa-top-h-path found.");
	delete mrs;
	return NULL;
      }
      mrs->ltop = requestVar(d, mrs);
    }

    // extract index
    d = dag_get_path_value(init_sem_dag, cheap_settings->req_value("mrs-psoa-index-path"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-psoa-index-path found.");
      delete mrs;
      return NULL;
    }
    mrs->index = requestVar(d, mrs);

    // extract liszt
    d = dag_get_path_value(init_sem_dag, cheap_settings->req_value("mrs-psoa-liszt-path"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-psoa-liszt-path found.");
      delete mrs;
      return NULL;
    }
    int n = 0;
    std::list<struct dag_node*> ep_list = dag_get_list(d);
    for (std::list<dag_node*>::iterator iter = ep_list.begin();
	 iter != ep_list.end(); ++iter, ++n) {
      if (*iter == FAIL) {
	LOG(logMRS, ERROR, boost::format("no relation %d") % n);
	delete mrs;
	return NULL;
      }
      tEp* ep = extractEp(*iter, mrs);
      if (ep != NULL)
	mrs->eps.push_back(ep);
    }

    // extract hcons
    d = dag_get_path_value(init_sem_dag, cheap_settings->req_value("mrs-psoa-rh-cons-path"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-psoa-rh-cons-path found.");
      delete mrs;
      return NULL;
    }
    n = 0;
    std::list<struct dag_node*> hcons_list = dag_get_list(d);
    for (std::list<dag_node*>::iterator iter = hcons_list.begin();
	 iter != hcons_list.end(); ++iter, ++n) {
      if (*iter == FAIL) {
	LOG(logMRS, ERROR, boost::format("no hcons %d") % n);
	delete mrs;
	return NULL;
      }
      tHCons* hcons = extractHCons(*iter, mrs);
      if (hcons != NULL)
	mrs->hconss.push_back(hcons);
    }

    return mrs;
  }

  tVar* MrsTfsExtractor::requestVar(struct dag_node* dag, tMrs* mrs) {
    if (_named_nodes.find(dag) != _named_nodes.end())
      return _named_nodes[dag];
    else {
      tVar* newvar = extractVar(_vid_generator++, dag);
      if (newvar != NULL) {
	mrs->register_var(newvar);
	_named_nodes[dag] = newvar;
      }
      return newvar;
    }
  }

  tVar* MrsTfsExtractor::requestVar(std::string type, tMrs* mrs) {
    tVar* var = new tVar(_vid_generator++, type);
    mrs->register_var(var);
    return var;
  }

  tHCons* MrsTfsExtractor::extractHCons(struct dag_node* dag, tMrs* mrs) {
    // _fixme_ : for the moment there is only one type of H-Cons: QEQ
    tHCons* hcons = new tHCons(mrs);
    
    struct dag_node* d = dag_get_path_value(dag, cheap_settings->req_value("mrs-sc-arg-feature"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-sc-arg-feature found.");
      delete hcons;
      return NULL;
    }
    hcons->harg = requestVar(d, mrs);

    d = dag_get_path_value(dag, cheap_settings->req_value("mrs-outscpd-feature"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-outscpd-feature found.");
      delete hcons;
      return NULL;
    }
    hcons->larg = requestVar(d, mrs);

    return hcons;
  }

  tEp* MrsTfsExtractor::extractEp(struct dag_node* dag, tMrs* mrs) {
    struct dag_node* d = FAIL;
    tEp* ep = new tEp(mrs);

    d = dag_get_path_value(dag, cheap_settings->req_value("mrs-rel-handel-path"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-rel-handel-path found.");
      delete ep;
      return NULL;
    }
    ep->label = requestVar(d, mrs);
    
    // get the relation name
    d = dag_get_path_value(dag, cheap_settings->req_value("mrs-rel-name-path"));
    if (d != FAIL) {
      ep->pred = type_name(d->type);
    }

    // get roles/flist (fvpairs)
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
	  value = ep->request_constant(type_name(attribute->val->type));
	} else {
	  value = requestVar(attribute->val, mrs);
	}
	ep->roles[feature] = value;
      }
    }

    // collect parameter strings
    // collect_param_strings();
    for (std::map<std::string,tValue*>::iterator iter = ep->roles.begin();
	 iter != ep->roles.end(); ++iter) {
      if (cheap_settings->member("mrs-value-feats", (*iter).first.c_str())) {
	ep->parameter_strings[(*iter).first] = (*iter).second;
      }
    }

     d = dag_get_path_value(dag, cheap_settings->req_value("mrs-rel-cfrom-feature"));
    if (d != FAIL) {
      if ((d->type == BI_TOP) || (d->type == BI_STRING))
	ep->cfrom = -1;
      else
	ep->cfrom = strtoint(type_name(d->type), "Invalid CFROM value", true);
    }
    d = dag_get_path_value(dag, cheap_settings->req_value("mrs-rel-cto-feature"));
    if (d != FAIL) {
      if ((d->type == BI_TOP) || (d->type == BI_STRING))
	ep->cto = -1;
      else
	ep->cto = strtoint(type_name(d->type), "Invalid CTO value", true);
    }

    return ep;
  }
 
  tVar* MrsTfsExtractor::extractVar(int vid, struct dag_node* dag) {
    tVar* var = new tVar(vid);
    if (dag != FAIL) {
      var->type = typenames[dag_type(dag)];

      // create variable property list
      createVarPropertyList(dag, "", var);
    }
    return var;
  }

  void MrsTfsExtractor::createVarPropertyList(struct dag_node* dag, std::string path, tVar* var) {
    dag_arc* attribute;
    std::string currpath = path == "" ? path : path+".";
    for (attribute = dag->arcs; attribute != NULL; attribute = attribute->next) { // for each attribute under current dag root node
      if (cheap_settings->member("mrs-ignored-extra-features",
				 attrname[attribute->attr]))
	continue;
      if (attribute->val->arcs == NULL) { // value of the attribute is atomic
	var->properties[currpath+attrname[attribute->attr]] = type_name(attribute->val->type);
      } else { // collect property recursively
	createVarPropertyList(attribute->val,
				currpath+attrname[attribute->attr], var);
      }
    }
  }

}


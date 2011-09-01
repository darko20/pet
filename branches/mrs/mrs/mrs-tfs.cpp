/* -*- Mode: C++ -*- */

#include "mrs-tfs.h"

namespace mrs {
  
  tMrs* MrsTfsExtrator::extractMrs(struct dag_node* dag) {
    dag = dag_expand(dag); // expand the fs to be well-formed
    tMrs* mrs = new tMrs();
    struct dag_node* init_sem_dag = FAIL;
    init_sem_dag = dag_get_path_value(dag, cheap_settings->req_value("mrs-initial-semantics-path"));
    if (init_sem_dag == FAIL) {
      LOG(logMRS, ERROR, "no mrs-initial-semantics-path found.");
      return;
    }

    // extract top-h
    struct dag_node* d = FAIL;
    const char* psoa_top_h_path = cheap_settings->value("mrs-psoa-top-h-path");
    if (!psoa_top_h_path || strcmp(psoa_top_h_path, "") == 0)
      mrs->ltop = mrs->request_var("handle");
    else {
      d = dag_get_path_value(init_sem_dag, psoa_top_h_path);
      if (d == FAIL) {
	LOG(logMRS, ERROR, "no mrs-psoa-top-h-path found.");
	return NULL;
      }
      mrs->ltop = requestVar(d);
    }

    // extract index
    d = dag_get_path_value(init_sem_dag, cheap_settings->req_value("mrs-psoa-index-path"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-psoa-index-path found.");
      return NULL;
    }
    mrs->index = requestVar(d);

    // extract liszt
    d = dag_get_path_value(init_sem_dag, cheap_settings->req_value("mrs-psoa-liszt-path"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-psoa-liszt-path found.");
      return NULL;
    }
    int n = 0;
    std::list<struct dag_node*> ep_list = dag_get_list(d);
    for (std::list<dag_node*>::iterator iter = ep_list.begin();
	 iter != ep_list.end(); ++iter, ++n) {
      if (*iter == FAIL) {
	LOG(logMRS, ERROR, boost::format("no relation %d") % n);
	mrs->eps.clear();
	return;
      }
      mrs->eps.push_back(extractEp(*iter, false, mrs));
    }

    // extract hcons
    d = dag_get_path_value(init_sem_dag, cheap_settings->req_value("mrs-psoa-rh-cons-path"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-psoa-rh-cons-path found.");
      return;
    }
    n = 0;
    std::list<struct dag_node*> hcons_list = dag_get_list(d);
    for (std::list<dag_node*>::iterator iter = hcons_list.begin();
	 iter != hcons_list.end(); ++iter, ++n) {
      if (*iter == FAIL) {
	LOG(logMRS, ERROR, boost::format("no hcons %d") % n);
	mrs->hcons.clear();
	return;
      }
      mrs->hcons.push_back(extractHCons(*iter, mrs));
    }
  }

  tVar* MrsTfsExtractor::requestVar(struct dag_node* dag) {
    if (_named_nodes.find(dag) != _named_nodes.end())
      return _named_nodes[dag];
    else {
      tVar* newvar = extractVar(_vid_generator++, dag);
      mrs->register_var(newvar);
      _named_nodes[dag] = newvar;
      return newvar;
    }
  }

  tVar* MrsTfsExtractor::requestVar(std::string type) {
    tVar* var = new tVar(_vid_generator++, type);
    register_var(var);
    return var;
  }

  tHCons* MrsTfsExtractor::extractHCons(struct dag_node* dag, tMrs* mrs) {
    // _fixme_ : for the moment there is only one type of H-Cons: QEQ
    tHCons* hcons = new tHCons(mrs);
    
    struct dag_node* d = dag_get_path_value(dag, cheap_settings->req_value("mrs-sc-arg-feature"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-sc-arg-feature found.");
      return;
    }
    hcons->harg = requestVar(d);

    d = dag_get_path_value(dag, cheap_settings->req_value("mrs-outscpd-feature"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-outscpd-feature found.");
      return;
    }
    hcons->larg = requestVar(d);

    return hcons;
  }

  tEp* MrsTfsExtractor::extractEp(struct dag_nod* dag, tMrs* mrs) {
    struct dag_node* d = FAIL;
    tEp* ep = new tEp(mrs);

    d = dag_get_path_value(dag, cheap_settings->req_value("mrs-rel-handel-path"));
    if (d == FAIL) {
      LOG(logMRS, ERROR, "no mrs-rel-handel-path found.");
      return;
    }
    label = requestVar(d);
    
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
	  value = new tConstant(type_name(attribute->val->type));
	} else {
	  value = requestVar(attribute->val);
	}
	ep->roles[feature] = value;
      }
    }

    // collect parameter strings
    // collect_param_strings();
    for (std::map<std::string,tValue*>::iterator iter = ep->roles.begin();
	 iter != roles.end(); ++iter) {
      if (cheap_settings->member("mrs-value-feats", (*iter).first.c_str())) {
	parameter_strings[(*iter).first] = (*iter).second;
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

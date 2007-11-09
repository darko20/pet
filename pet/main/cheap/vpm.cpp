#include "vpm.h"

extern FILE* ferr;

tVPM::~tVPM() {
  for (list<tPM*>::iterator pm = pms.begin();
       pm != pms.end(); pm ++) 
    delete *pm;
}

bool tVPM::
read_vpm(const string &filename, string id) {
  this->id = id;
  tPM* pm = NULL;
  char* line = new char[MAXVPMLINE];
  FILE* f = fopen(filename.c_str(), "r");
  while (!feof(f)) {
    fgets(line, MAXVPMLINE, f);
    size_t length = strspn(line, " \t\n");
    if (length == strlen(line)) // empty line;
      continue;
    else if (line[length] == ';') // comment line;
      continue;
    if (strchr(line, ':') != NULL) {
      // beginning line of a pm
      if (pm != NULL)
	pms.push_back(pm);
      pm = new tPM();
      char* tok = strtok(line, " \t\n");
      bool onleft = true;
      while (tok != NULL) {
	if (strcmp(tok, ":") == 0)
	  onleft = false;
	else {
	  if (onleft) 
	    pm->lhs.push_back(tok);
	  else 
	    pm->rhs.push_back(tok);
	}
	tok = strtok(NULL, " \t\n");
      }
    } else if (strpbrk(line, "<>=") != NULL) {
      // a line of pmr
      tPMR* pmr = new tPMR();
      char* tok = strtok(line, " \t\n");
      bool onleft = true;
      while (tok != NULL) {
	if (strpbrk(tok, "<>=") != NULL) {
	  int len = strlen(tok);
	  if (len <2 || len >3) { 
	    fprintf(ferr, "Invalid direction (must be 2 or 3 chars): `%s'\n", tok);
	    return false;
	  }
	  if (strcmp(tok, "==") == 0) {
	    pmr->forward = true;
	    pmr->backward = true;
	  }
	  if (strchr(tok, '>') != NULL)
	    pmr->forward = true;
	  if (strchr(tok, '<') != NULL)
	    pmr->backward = true;
	  if (strchr(tok, '=') != NULL)
	    pmr->eqtest = true;
	  if (strspn(tok, "<>=") != strlen(tok)) {
	    fprintf(ferr, "Invalid direction (unrecognized chars): `%s'\n", tok);
	    return false;
	  }
	  onleft = false;
	} else {
	  if (onleft) 
	    pmr->left.push_back(tok);
	  else 
	    pmr->right.push_back(tok);
	}
	tok = strtok(NULL, " \t\n");
      }
      if (pm == NULL) {
	fprintf(ferr, "Invalid line while reading vpm.\n");
	return false;
      }
      if (pmr->left.size() != pm->lhs.size() ||
	  pmr->right.size() != pm->rhs.size()) {
	fprintf(ferr, "The pm rule size does not match.\n");
	return false;
      }
      pm->pmrs.push_back(pmr);
    }
  }
  if (pm != NULL)
    pms.push_back(pm);

  fclose(f);
  delete[] line;
  return true;

}

tPSOA* tVPM::
map_mrs(tPSOA* mrs_in, bool forwardp) {
  tPSOA* mrs_new = new tPSOA();
  mrs_new->top_h = map_variable(mrs_in->top_h, mrs_new, forwardp);
  mrs_new->index = map_variable(mrs_in->index, mrs_new, forwardp);
  for (list<tBaseRel*>::iterator iter = mrs_in->liszt.begin();
       iter != mrs_in->liszt.end(); iter ++) {
    tRel* rel_in = dynamic_cast<tRel*>(*iter);
    tRel* rel_new = new tRel(mrs_new);
    rel_new->handel = map_variable(rel_in->handel, mrs_new, forwardp);
    rel_new->pred = rel_in->pred;
    for (map<string,tValue*>::iterator pair = rel_in->flist.begin();
	 pair != rel_in->flist.end(); pair ++) {
      tVar* vval = dynamic_cast<tVar*>((*pair).second);
      if (vval == NULL) {
	rel_new->flist[(*pair).first] = 
	  mrs_new->request_constant((dynamic_cast<tConstant*>((*pair).second))->value);
      } else {
	vval = map_variable(vval, mrs_new, forwardp);
	rel_new->flist[(*pair).first] = vval;
      }
    }
    rel_new->collect_param_strings();
    rel_new->cfrom = rel_in->cfrom;
    rel_new->cto = rel_in->cto;
    mrs_new->liszt.push_back(rel_new);
  }
  for (list<tHCons*>::iterator iter = mrs_in->h_cons.begin();
       iter != mrs_in->h_cons.end(); iter ++) {
    tHCons* hcons_new = new tHCons(mrs_new); 
    hcons_new->relation = (*iter)->relation;
    hcons_new->scarg = map_variable((*iter)->scarg, mrs_new, forwardp);
    hcons_new->outscpd = map_variable((*iter)->outscpd, mrs_new, forwardp);
    mrs_new->h_cons.push_back(hcons_new);
  }
  mrs_new->_valid = true;
  _vv_map.clear();
  return mrs_new;
}

tVar* tVPM::
map_variable(tVar* var_in, tPSOA* mrs, bool forwardp) {

  if (_vv_map.find(var_in) != _vv_map.end()) // already mapped
    return _vv_map[var_in];

  tVar* var_new = mrs->request_var(var_in->id, var_in->type);
  // map properties
  for (list<tPM*>::iterator pm = pms.begin();
       pm != pms.end(); pm ++) { // for each PM
    list<string> lhs = forwardp ? (*pm)->lhs : (*pm)->rhs;
    list<string> rhs = forwardp ? (*pm)->rhs : (*pm)->lhs;
    list<string> values;
    for (list<string>::iterator property = lhs.begin();
	 property != lhs.end(); property ++) { // for each property in the PM lhs
      if (var_in->extra.find(*property) == var_in->extra.end())
	values.push_back("");
      else
	values.push_back(var_in->extra[*property]);
    }
    for (list<tPMR*>::iterator pmr = (*pm)->pmrs.begin();
	 pmr != (*pm)->pmrs.end(); pmr ++) { // for each PM-rule
      if ((*pmr)->test(var_new->type, values, forwardp)) {
	list<string> right = forwardp ? (*pmr)->right : (*pmr)->left;
	for (list<string>::iterator property = rhs.begin(), 
	       newval = right.begin();
	     property != rhs.end(), 
	       newval != right.end();
	     property ++, newval ++) {
	  if (*newval == "!")
	    continue;
	  else { // "*" is illegal here!
	    var_new->extra[*property] = *newval;
	  }
	}
	break;
      }
    }
    
  }
  _vv_map[var_in] = var_new;
  return var_new;
}

tPM::~tPM() {
  for (list<class tPMR*>::iterator pmr = pmrs.begin();
       pmr != pmrs.end(); pmr ++) 
    delete *pmr;
}

bool tPMR::
test(char type, list<string> values, bool forwardp) {
  if (forwardp && !forward)
    return false;
  if (!forwardp && !backward)
    return false;

  list<string> matches = forwardp ? left : right;
  if (matches.size() != values.size())
    return false;
  for (list<string>::iterator match = matches.begin(), value = values.begin();
       match != matches.end() && value != values.end(); match ++, value ++) {
    if ((*match).c_str()[0] == '[' && (*match).c_str()[1] == type) {
      if (compatible_var_types(type, (*match).c_str()[1]))
	continue;
      else 
	return false;
    }
    if (*match == "*")
      if (*value == "")
	return false;
      else
	continue;
    if (*match == "!")
      if (*value == "") 
	continue;
      else
	return false;
    if (eqtest) {
      if (*match == *value)
	continue;
      else
	return false;
    } else {
      type_t t_value = lookup_type((*value).c_str());
      type_t t_match = lookup_type((*match).c_str());
      if (t_value != -1 && t_match != -1 &&
	  subtype(t_value, t_match))
	continue;
      else
	return false;
    }
  }
  return true;
}


#include "vpm.h"

#include <cstring>

extern FILE* ferr;

using namespace std;

tVPM::~tVPM() {
  for (list<tPM*>::iterator pm = pms.begin(); pm != pms.end(); ++pm)
    delete *pm;
}

bool tVPM::read_vpm(const string &filename, string id) {
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
      // a line of mapping rule, being it for types or parameters
      tMR* mr = new tMR();
      char* tok = strtok(line, " \t\n");
      bool onleft = true;
      while (tok != NULL) {
        if (strpbrk(tok, "<>=") != NULL) {
          int len = strlen(tok);
          if (len <2 || len >3) {
            fprintf(ferr, "Invalid direction (must be 2 or 3 chars): `%s'\n", 
              tok);
            return false;
          }
          if (strcmp(tok, "==") == 0) {
            mr->forward = true;
            mr->backward = true;
          }
          if (strchr(tok, '>') != NULL)
            mr->forward = true;
          if (strchr(tok, '<') != NULL)
            mr->backward = true;
          if (strchr(tok, '=') != NULL)
            mr->eqtest = true;
          if (strspn(tok, "<>=") != strlen(tok)) {
            fprintf(ferr, "Invalid direction (unrecognized chars): `%s'\n", i
              tok);
            return false;
          }
          onleft = false;
        } else {
          if (onleft)
            mr->left.push_back(tok);
          else
            mr->right.push_back(tok);
        }
        tok = strtok(NULL, " \t\n");
      }
      if (pm == NULL && mr->left.size() == 1 && mr->right.size() == 1)
        // it's a variable type mapping
        tms.push_back(mr);
      else if (mr->left.size() == pm->lhs.size() ||
               mr->right.size() == pm->rhs.size())
        pm->pmrs.push_back(mr);
      else {
        // a bad mapping rule with unmatched lhs or rhs
        fprintf(ferr, "The pm rule is not valid.\n");
        return false;
      }
    }
  }
  if (pm != NULL)
    pms.push_back(pm);

  fclose(f);
  delete[] line;
  return true;

}

tMrs* tVPM::map_mrs(tMrs* mrs_in, bool forwardp) {
  tMrs* mrs_new = new tMrs();
  mrs_new->ltop = map_variable(mrs_in->ltop, mrs_new, forwardp);
  mrs_new->index = map_variable(mrs_in->index, mrs_new, forwardp);
  for (list<tBaseEp*>::iterator iter = mrs_in->eps.begin();
       iter != mrs_in->eps.end(); ++iter) {
    tEp* ep_in = dynamic_cast<tEp*>(*iter);
    tEp* ep_new = new tEp(mrs_new);
    ep_new->label = map_variable(ep_in->label, mrs_new, forwardp);
    ep_new->pred = ep_in->pred;
    for (map<string, tValue*>::iterator pair = ep_in->roles.begin();
         pair != ep_in->roles.end(); ++pair) {
      tVar* vval = dynamic_cast<tVar*>((*pair).second);
      if (vval == NULL) {
        ep_new->roles[(*pair).first] =
          mrs_new->request_constant(
          (dynamic_cast<tConstant*>((*pair).second))->value);
      } else {
        vval = map_variable(vval, mrs_new, forwardp);
        ep_new->roles[(*pair).first] = vval;
      }
    }
//    ep_new->collect_param_strings();
    ep_new->cfrom = ep_in->cfrom;
    ep_new->cto = ep_in->cto;
    mrs_new->eps.push_back(ep_new);
  }
  for (list<tHCons*>::iterator iter = mrs_in->hconss.begin();
       iter != mrs_in->hconss.end(); ++iter) {
    tHCons* hcons_new = new tHCons(mrs_new);
    hcons_new->relation = (*iter)->relation;
    hcons_new->harg = map_variable((*iter)->harg, mrs_new, forwardp);
    hcons_new->larg = map_variable((*iter)->larg, mrs_new, forwardp);
    mrs_new->hconss.push_back(hcons_new);
  }
  mrs_new->_valid = true;
  _vv_map.clear();
  return mrs_new;
}

tVar* tVPM::map_variable(tVar* var_in, tMRS* mrs, bool forwardp) {

  if (_vv_map.find(var_in) != _vv_map.end()) // already mapped
    return _vv_map[var_in];

  // map variable type
  string new_type = var_in->type;
  for (list<tMR*>::iterator tm = tms.begin();
       tm != tms.end(); ++tm) { // for each TM
    if (forwardp && !(*tm)->forward)
      continue;
    if (!forwardp && !(*tm)->backward)
      continue;
    string ltname = forwardp ? (*tm)->left.front() : (*tm)->right.front();
    string rtname = forwardp ? (*tm)->right.front() : (*tm)->left.front();
    type_t lt = lookup_type(ltname.c_str());
    type_t vt = lookup_type(var_in->type.c_str());
    if ((lt != -1 && vt != -1 && subtype(vt, lt)) ||
        ltname == "*" ) {
      new_type = rtname;
      break;
    }
  }
  tVar* var_new = mrs->request_var(var_in->id, new_type);
  // map properties
  for (list<tPM*>::iterator pm = pms.begin();
       pm != pms.end(); ++pm) { // for each PM set
    list<string> lhs = forwardp ? (*pm)->lhs : (*pm)->rhs;
    list<string> rhs = forwardp ? (*pm)->rhs : (*pm)->lhs;
    list<string> values;
    for (list<string>::iterator property = lhs.begin();
         property != lhs.end(); ++property) { // for each property in the PM lhs
      if (var_in->extra.find(*property) == var_in->extra.end())
        values.push_back("");
      else
        values.push_back(var_in->extra[*property]);
    }
    for (list<tMR*>::iterator pmr = (*pm)->pmrs.begin();
         pmr != (*pm)->pmrs.end(); ++pmr) { // for each mapping-rule
      if ((*pmr)->test(var_in->type, values, forwardp)) {
        list<string> right = forwardp ? (*pmr)->right : (*pmr)->left;
        for (list<string>::iterator property = rhs.begin(),
               newval = right.begin();
             property != rhs.end(),     /// \todo shouldn't this be && ??
               newval != right.end();
             ++property, ++newval) {
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
  for (list<class tMR*>::iterator pmr = pmrs.begin(); pmr != pmrs.end(); ++pmr)
    delete *pmr;
}

bool tMR::
test(string type, list<string> values, bool forwardp) {
  if (forwardp && !forward)
    return false;
  if (!forwardp && !backward)
    return false;

  list<string> matches = forwardp ? left : right;
  if (matches.size() != values.size())
    return false;
  for (list<string>::iterator match = matches.begin(), value = values.begin();
       match != matches.end() && value != values.end(); ++match, ++value) {
    if ((*match).c_str()[0] == '[' &&
        strncmp((*match).c_str()+1,type.c_str(),type.length()) &&
        (*match).c_str()[(*match).length()-1] == ']') {
      if (compatible_var_types(type, *match))
        continue;
      else
        return false;
    }
    if (*match == "*") {
      if (*value == "")
        return false;
      else
        continue;
    }
    if (*match == "!") {
      if (*value == "")
        continue;
      else
        return false;
    }
    if (eqtest) {
      if (*match == *value)
        continue;
      else
        return false;
    } else { // grammar internal types
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


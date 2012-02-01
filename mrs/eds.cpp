/* ex: set expandtab ts=2 sw=2: */
#include "eds.h"
#ifdef MRS_ONLY
#include "mrs-errors.h"
#else
#include "errors.h"
#endif
#include <sstream>
#include <iostream>

namespace mrs {

void tEds::tEdsNode::add_edge(tEdsEdge *e) {
  outedges.push_back(e);
}

tEds::tEdsNode::~tEdsNode() {
  for(std::vector<tEdsEdge *>::iterator it = outedges.begin();
      it != outedges.end(); ++it)
    delete *it;
}

tEds::tEds():_counter(1) { 

}

tEds::tEds(tMrs *mrs):_counter(1) {

  // temporary mappings to find the correct target node
  typedef std::multimap<std::string, int>::iterator MmSIit;
  std::multimap<std::string, int> dvar2nodes;
  std::multimap<std::string, int> handle2nodes;
  std::map<std::string, int> representatives; 

  // EDG top == mrs index?
  top = var_name(mrs->index);

  // add an EDG node for each EP
  for (std::vector<tBaseEp *>::iterator it = mrs->eps.begin();
        it != mrs->eps.end(); ++it) {
    std::string dvar_name;
    tVar *dvar = NULL;
    tEp *ep = dynamic_cast<tEp*>(*it);
    std::string pred_name = pred_normalize(ep->pred);
    std::string handle_name = var_name(ep->label);
    if (ep->quantifier_ep()) {//treat quants differently
      std::ostringstream name;
      name << "_" << _counter++;
      dvar_name = name.str();
    } else {
      dvar = get_id(ep);
      dvar_name = var_name(dvar);
    }

    //create node and set link 
    tEdsNode *node 
      = new tEdsNode(pred_name, dvar_name, handle_name, ep->cfrom, ep->cto);
    if (!(ep->link).empty())
      node->link = ep->link;
    else {
      std::ostringstream lnkstr;
      lnkstr << "<" << node->cfrom << ":" << node->cto << ">";
      node->link = lnkstr.str();
    }
    if (ep->quantifier_ep())
      node->quantifier = true;
    // add to map instead, checking var hasn't been used before
    _nodes.push_back(node);
    
    // record which node(s) are attached to which arg0s (dvars) and handles
    dvar2nodes.insert(std::pair<std::string,int>(dvar_name, _nodes.size()-1));
    handle2nodes.insert(std::pair<std::string,int>(handle_name, 
                          _nodes.size()-1));
    if (linkToNodes.count(node->link) == 0)
      linkToNodes[node->link] = std::vector<int>();
    linkToNodes[node->link].push_back(_nodes.size()-1);
    if (dvar != NULL) {
    //copy var properties to node
      for (std::map<std::string,std::string>::iterator prop
        = dvar->properties.begin(); prop != dvar->properties.end(); ++prop) {
        node->properties[prop->first] = prop->second;
        Triple pt;
        pt.first = node->link;
        pt.second = std::pair<std::string,std::string>(prop->first, prop->second);
        propTriples.push_back(pt);
        if (linkToPropTriples.count(node->link) == 0)
          linkToPropTriples[node->link] = std::vector<int>();
        linkToPropTriples[node->link].push_back(propTriples.size()-1);
      }
    }
    
    // add relevant roles as edges (or cargs)
    for (std::map<std::string, tValue *>::iterator rit = ep->roles.begin();
          rit != ep->roles.end(); ++rit) {
      if (carg_rel(rit->first)) {
        _nodes.back()->carg = dynamic_cast<tConstant *>(rit->second)->value;
      } else if (relevant_rel(rit->first)) {
        _nodes.back()->add_edge(new tEdsEdge(-1, rit->first, 
                                var_name(dynamic_cast<tVar *>(rit->second))));
      }
    }
    if (_nodes.back()->quantifier) {
      if(ep->roles.count("ARG0") > 0) {
        _nodes.back()->add_edge(new tEdsEdge(-1, "BV", 
          var_name(dynamic_cast<tVar *>(ep->roles["ARG0"]))));
      }
    }
  }

  // fix the edges to point to the representative node
  for (std::vector<tEdsNode *>::iterator nit = _nodes.begin();
        nit != _nodes.end(); ++nit) {
    for (std::vector<tEdsEdge *>::iterator eit = (*nit)->outedges.begin();
          eit != (*nit)->outedges.end(); ++eit) {
      Triple at;
      at.first = (*nit)->link;
      if (representatives.count((*eit)->target_name) == 1) {
        //we've looked this one up before, just set the target
        (*eit)->target = representatives[(*eit)->target_name];
        at.second = PSS((*eit)->edge_name, 
          _nodes[representatives[(*eit)->target_name]]->link);
        argTriples.push_back(at);
        if (linkToArgTriples.count((*nit)->link) == 0)
          linkToArgTriples[(*nit)->link] = std::vector<int>();
        linkToArgTriples[(*nit)->link].push_back(argTriples.size()-1);
      } else {
        if (dvar2nodes.count((*eit)->target_name) > 0) {
          //instantiated arg0
          std::set<int> candidates;
          std::pair<MmSIit,MmSIit> spanends 
            = dvar2nodes.equal_range((*eit)->target_name);
          for (MmSIit cit = spanends.first; cit != spanends.second; ++cit) {
            if (!_nodes[cit->second]->quantifier_node()) 
              candidates.insert(cit->second);
          }
          if (candidates.size() == 1) {
            (*eit)->target = *(candidates.begin());
            representatives[(*eit)->target_name] = (*eit)->target;
            at.second = PSS((*eit)->edge_name, 
              _nodes[representatives[(*eit)->target_name]]->link);
            argTriples.push_back(at);
            if (linkToArgTriples.count((*nit)->link) == 0)
              linkToArgTriples[(*nit)->link] = std::vector<int>();
            linkToArgTriples[(*nit)->link].push_back(argTriples.size()-1);
          } else {
            int t = select_candidate(candidates);
            (*eit)->target = t;
            representatives[(*eit)->target_name] = t;
            at.second = PSS((*eit)->edge_name, 
              _nodes[representatives[(*eit)->target_name]]->link);
            argTriples.push_back(at);
            if (linkToArgTriples.count((*nit)->link) == 0)
              linkToArgTriples[(*nit)->link] = std::vector<int>();
            linkToArgTriples[(*nit)->link].push_back(argTriples.size()-1);
          }
        } else if (handle_var((*eit)->target_name)) {
          //trace handle through
          std::set<int> candidates;
          std::pair<MmSIit,MmSIit> spanends 
            = handle2nodes.equal_range((*eit)->target_name);
          for (MmSIit cit = spanends.first; cit != spanends.second; ++cit) 
            candidates.insert(cit->second);
          for (std::vector<tHCons *>::iterator hit = mrs->hconss.begin();
                hit != mrs->hconss.end(); ++hit) {
            if (var_name((*hit)->harg) == (*eit)->target_name) {
              std::string larg = var_name((*hit)->larg);
              spanends = handle2nodes.equal_range(larg);
              for (MmSIit cit = spanends.first; cit != spanends.second; ++cit)
                candidates.insert(cit->second);
            }
          }
          if (candidates.size() == 1) {
            (*eit)->target = *(candidates.begin());
            representatives[(*eit)->target_name] = (*eit)->target;
            at.second = PSS((*eit)->edge_name, 
              _nodes[representatives[(*eit)->target_name]]->link);
            argTriples.push_back(at);
            if (linkToArgTriples.count((*nit)->link) == 0)
              linkToArgTriples[(*nit)->link] = std::vector<int>();
            linkToArgTriples[(*nit)->link].push_back(argTriples.size()-1);
          } else {
            int t = select_candidate(candidates);
            (*eit)->target = t;
            representatives[(*eit)->target_name] = t;
            at.second = PSS((*eit)->edge_name, 
              _nodes[representatives[(*eit)->target_name]]->link);
            argTriples.push_back(at);
            if (linkToArgTriples.count((*nit)->link) == 0)
              linkToArgTriples[(*nit)->link] = std::vector<int>();
            linkToArgTriples[(*nit)->link].push_back(argTriples.size()-1);
          }
        } //else leave target as -1 -> no instantiated referrant
      }
    }
  }
}

tEds::~tEds() {
  for(std::map<std::string, tVar *>::iterator it = _vars_map.begin();
      it != _vars_map.end(); ++it) 
    delete it->second;

  for(std::vector<tEdsNode *>::iterator it = _nodes.begin();
      it != _nodes.end(); ++it)
    delete *it;
}

void tEds::print_eds() {
  std::cout << "{" << top << ":" << std::endl;
  for (std::vector<tEdsNode *>::iterator it = _nodes.begin();
    it != _nodes.end(); ++it) {
//      if (!(*it)->quantifier_node() && (*it)->outedges.empty()) continue;
      std::cout << " " << (*it)->dvar_name << ":" << (*it)->pred_name
        << (*it)->link << "[";
      for (std::vector<tEdsEdge *>::iterator eit = (*it)->outedges.begin();
        eit != (*it)->outedges.end(); ++eit) {
        if (eit != (*it)->outedges.begin()) 
          std::cout << ", ";
        if ((*eit)->target == -1) {//continue;
          std::cout << (*eit)->edge_name << " "
            << (*eit)->target_name;
        } else {
          std::cout << (*eit)->edge_name << " " 
            << _nodes[(*eit)->target]->dvar_name; 
            //<< ":" << _nodes[(*eit)->target]->pred_name;
          //if (!_nodes[(*eit)->target]->carg.empty())
          //  std::cout << "(" << _nodes[(*eit)->target]->carg << ")";
        }
      }
      std::cout << "]" << std::endl;
  }

  std::cout << "}" << std::endl;
}

void tEds::print_triples() {
  std::cout << "R " << top << std::endl;
  for (std::map<std::string, std::vector<int> >::iterator lint 
    = linkToNodes.begin(); lint != linkToNodes.end(); ++lint) {
    for (std::vector<int>::iterator nint = lint->second.begin(); 
      nint != lint->second.end(); ++nint) {
      std::cout << "N " << _nodes[*nint]->link << " PRED \"" 
        << _nodes[*nint]->pred_name << "\"" << std::endl;
    }
  }
  for (std::vector<Triple>::iterator aint = argTriples.begin();
    aint != argTriples.end(); ++aint) {
    std::cout << "A " << aint->first << " " << aint->second.first
      << " " << aint->second.second << std::endl;
  }
  for (std::vector<Triple>::iterator pint = propTriples.begin();
    pint != propTriples.end(); ++pint) {
    std::cout << "P " << pint->first << " " << pint->second.first
      << " " << pint->second.second << std::endl;
  }
}

void tEds::read_eds(std::string input) {

  // temporary mappings to find the correct target node
  typedef std::multimap<std::string, int>::iterator MmSIit;
  std::multimap<std::string, int> dvar2nodes;
  std::map<std::string, int> representatives; 

  std::string rest = input;

  parseChar('{', rest);
  top = parseVar(rest);
  parseChar(':', rest);
  if (rest.at(0) == '|') //skip over fragment/cyclic markers for now
    rest.erase(0,1);
  std::string var = parseVar(rest);
  while (!var.empty()) {
    dvar2nodes.insert(std::pair<std::string,int>(var, read_node(var, rest)));
    if (rest.at(0) == '|') //skip over fragment/cyclic markers for now
      rest.erase(0,1);
    var = parseVar(rest);
  }
  parseChar('}', rest);
  if (!rest.empty())
    throw tError("ignoring trailing data: \"" + rest + "\"");
  
  // fix the edges to point to the representative node
  for (std::vector<tEdsNode *>::iterator nit = _nodes.begin();
        nit != _nodes.end(); ++nit) {
    for (std::vector<tEdsEdge *>::iterator eit = (*nit)->outedges.begin();
          eit != (*nit)->outedges.end(); ++eit) {
      if (representatives.count((*eit)->target_name) == 1) {
        //we've looked this one up before, just set the target
        (*eit)->target = representatives[(*eit)->target_name];
      } else {
        if (dvar2nodes.count((*eit)->target_name) > 0) {
          //instantiated arg0
          std::set<int> candidates;
          std::pair<MmSIit,MmSIit> spanends 
            = dvar2nodes.equal_range((*eit)->target_name);
          for (MmSIit cit = spanends.first; cit != spanends.second; ++cit) {
            if (!_nodes[cit->second]->quantifier_node()) 
              candidates.insert(cit->second);
          }
          if (candidates.size() == 1) {
            (*eit)->target = *(candidates.begin());
            representatives[(*eit)->target_name] = (*eit)->target;
          } else {
            int t = select_candidate(candidates);
            (*eit)->target = t;
            representatives[(*eit)->target_name] = t;
          }
        }
      }
    }
  }
}

void tEds::removeWhitespace(std::string &rest) {
  while (!rest.empty() && isspace(rest.at(0)))
    rest.erase(0,1);
}

void tEds::parseChar(char x, std::string &rest) {
  removeWhitespace(rest);
  if (!rest.empty() && rest.at(0) == x) {
    rest.erase(0,1);
    removeWhitespace(rest);
  } else {
    if (rest.empty())
      throw tError("Reached end of string while looking for " + x);
    else
      throw tError("Ill-formed EDG. Got \"" + rest + "\", looking for " + x);
  }
}

std::string tEds::parseVar(std::string &rest) {
  tVar *var;
  std::string vtype, vidstring, varname;
  int vid;
  if (isalpha(rest.at(0)) || rest.at(0) == '_') {
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
    varname = std::string(vtype + vidstring);
    if (_vars_map.count(varname) == 0) {
      var = new tVar(vid, vtype);
      _vars_map.insert(std::pair<std::string, tVar *>(varname, var));
    }
  } else {
    varname = std::string();
  }
  removeWhitespace(rest);
  return varname;
}

int tEds::read_node(std::string id, std::string &rest) {
  std::string predname, span;
  int to, from, nodenumber;
  parseChar(':', rest);
  while(!rest.empty() && !isspace(rest.at(0)) && rest.at(0) != '<' 
        && rest.at(0) != '[') {
    predname += rest.at(0);
    rest.erase(0,1);
  }
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
    std::istringstream fromstr(span.substr(1, colon-1));
    fromstr >> from;
    std::istringstream tostr(span.substr(colon+1));
    tostr >> to;
  } else { //no link
    span = std::string("<-1:-1>");
    to = -1;
    from = -1;
  }
  tEdsNode *node = new tEdsNode(predname, id, "", from, to);
  node->link = span;
  _nodes.push_back(node);
  nodenumber = _nodes.size()-1;
  removeWhitespace(rest);
  parseChar('[', rest);
  while (rest.at(0) != ']') {
    std::string reln, targetvar, targetname, targetcarg;
    while (!rest.empty() && !isspace(rest.at(0))) {
      reln += rest.at(0);
      rest.erase(0,1);
    }
    removeWhitespace(rest);
    targetvar = parseVar(rest);
    if (rest.at(0) == ':') {
      parseChar(':', rest);
      while(!rest.empty() && !isspace(rest.at(0)) && rest.at(0) != '(' 
            && rest.at(0) != ']' && rest.at(0) != ',') {
        targetname += rest.at(0);
        rest.erase(0,1);
      }
      if (rest.at(0) == '(') { //carg
        while(!rest.empty() && rest.at(0) != ')') {
          targetcarg += rest.at(0);
          rest.erase(0,1);
        }
        if (rest.at(0) == ')') 
          rest.erase(0,1);
        else
          throw tError("Unterminated carg at \"" + rest + "\".");
      }
    }
    removeWhitespace(rest);
    _nodes.back()->add_edge(new tEdsEdge(-1, reln, targetvar));
    // add edge. add target node?
//    int targetnode = find_node(targetvar, targetname);

    if (rest.at(0) == ',')
      rest.erase(0,1);
    removeWhitespace(rest);
  }
  parseChar(']', rest);
  return nodenumber;
}

std::string tEds::var_name(tVar *v) {
  std::ostringstream name;
  name << v->type << v->id;
  return name.str();
}

int tEds::select_candidate(std::set<int> candidates) {
  for(std::set<int>::iterator it = candidates.begin(); 
      it != candidates.end(); ++it) {
    if (_nodes[*it]->quantifier_node())
      candidates.erase(it);
  }
  if (candidates.size() == 1)
    return *(candidates.begin());
  if (candidates.empty())
    return -1;
  // if we still have a candidate list > 1, take the candidate that is pointed
  // to by the most other candidates on the list
  int candidate = -1;
  int maxreferrers = -1;
  for(std::set<int>::iterator it = candidates.begin();
        it != candidates.end(); ++it) {
    int referrers = 0;
    for(std::set<int>::iterator iit = candidates.begin();
        iit != candidates.end(); ++iit) {
      if (*iit == *it) continue;
      for (std::vector<tEdsEdge *>::iterator eit 
            = _nodes[*iit]->outedges.begin();
            eit != _nodes[*iit]->outedges.end(); ++eit) {
        if ((*eit)->target_name == _nodes[*it]->dvar_name)
          ++referrers;
      }
    }
    if (referrers > maxreferrers) {
      maxreferrers = referrers;
      candidate = *it;
    }
  }
  if (maxreferrers > 0 && candidate != -1)
    return candidate;
  else
    return *(candidates.begin());
}

//replace these placeholders with something more flexible

std::string tEds::pred_normalize(std::string pred) {
  std::string normedpred = pred;
  if (normedpred.length() >= 4 && 
    normedpred.compare(normedpred.length()-4, 4, "_rel") == 0)
    normedpred.erase(normedpred.length()-4, 4);
  return normedpred;
}

tVar *tEds::get_id(tEp *ep) {
  if (ep->roles.count("ARG0") == 1)
    return dynamic_cast<tVar *>(ep->roles["ARG0"]);
  else
    return NULL;
}

bool tEds::quantifier_pred(std::string pred) {
  if (pred.find("_q") != std::string::npos)
    return true;
  if (pred.compare("quant") == 0)
    return true;
  return false;
}

bool tEds::carg_rel(std::string role) {
  return role.compare("CARG") == 0;
}

bool tEds::relevant_rel(std::string role) {
  return role.compare("ARG") == 0 
    || role.compare("ARG1") == 0
    || role.compare("ARG2") == 0
    || role.compare("ARG3") == 0
    || role.compare("ARG4") == 0
    || role.compare("BV") == 0
    || role.compare("L-INDEX") == 0
    || role.compare("R-INDEX") == 0
    || role.compare("L-HNDL") == 0
    || role.compare("R-HNDL") == 0;
}

bool tEds::handle_var(std::string var) {
  return var.at(0) == 'h';
}

bool tEds::tEdsNode::quantifier_node() {
  return quantifier;
}

} // namespace mrs


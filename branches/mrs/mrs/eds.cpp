/* ex: set expandtab ts=2 sw=2: */
#include "eds.h"
#ifdef MRS_ONLY
#include "mrs-errors.h"
#else
#include "errors.h"
#endif
#include <sstream>
#include <iostream>
#include <boost/algorithm/string.hpp>

namespace mrs {

void tEds::tEdsNode::add_edge(tEdsEdge *e) {
  outedges.push_back(e);
}

tEds::tEdsNode::tEdsNode(tEdsNode &n):pred_name(n.pred_name), 
  dvar_name(n.dvar_name), link(n.link), carg(n.carg), 
  handle_name(n.handle_name), cfrom(n.cfrom), cto(n.cto), 
  quantifier(n.quantifier) {
  for (std::vector<tEdsEdge *>::iterator it = n.outedges.begin(); 
    it != n.outedges.end(); ++it) {
    tEdsEdge *e = new tEdsEdge((*it)->edge_name, (*it)->target);
    outedges.push_back(e);
  }
  for (std::map<std::string, std::string>::iterator it = n.properties.begin();
      it != n.properties.end(); ++it) {
      properties[it->first] = it->second;
  }
}

tEds::tEdsNode::~tEdsNode() {
  for(std::vector<tEdsEdge *>::iterator it = outedges.begin();
      it != outedges.end(); ++it)
    delete *it;
}

tEds::tEds(tMrs *mrs):_counter(1) {
  // add an EDG node for each EP
  for (std::vector<tBaseEp *>::iterator it = mrs->eps.begin();
        it != mrs->eps.end(); ++it) {
    std::string dvar_name;
    tVar *dvar = NULL;
    tEp *ep = dynamic_cast<tEp*>(*it);
    std::string pred_name = pred_normalize(ep->pred);
    std::string handle_name = var_name(ep->label);
    if (ep->quantifier_ep()) {//treat quantifiers differently
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
    MmSNit newnode 
      = _nodes.insert(std::pair<std::string, tEdsNode *>(dvar_name, node));
    _orderedNodes.push_back(node);
    // record which node(s) are attached to which handles
    handle2nodes.insert(std::pair<std::string,MmSNit>(handle_name, newnode));
    if (ep->quantifier_ep())
      node->quantifier = true;

    if (dvar != NULL) {
    //copy var properties to node
      for (std::map<std::string,std::string>::iterator prop
        = dvar->properties.begin(); prop != dvar->properties.end(); ++prop) {
        node->properties[prop->first] = prop->second;
      }
    }
    // add relevant roles as edges (or cargs)
    for (std::map<std::string, tValue *>::iterator rit = ep->roles.begin();
          rit != ep->roles.end(); ++rit) {
      if (carg_rel(rit->first)) {
        node->carg = dynamic_cast<tConstant *>(rit->second)->value;
      } else if (relevant_rel(rit->first)) {
        node->add_edge(new tEdsEdge(rit->first, 
                                var_name(dynamic_cast<tVar *>(rit->second))));
      }
    }
    if (node->quantifier) {
      if(ep->roles.count("ARG0") > 0) {
        node->add_edge(new tEdsEdge("BV", 
          var_name(dynamic_cast<tVar *>(ep->roles["ARG0"]))));
      }
    }
  }

  // fix the edges to point to the representative node
  for (MmSNit nit = _nodes.begin(); nit != _nodes.end(); ++nit) {
    for (std::vector<tEdsEdge *>::iterator eit 
        = (*nit).second->outedges.begin(); eit != (*nit).second->outedges.end();
        ++eit) {
      if (handle_var((*eit)->target)) {
        (*eit)->target = find_representative(mrs, (*eit)->target);
      }
    }
  }

  // make sure nodes have unique labels
  std::string lastlabel;
  bool notfinished = true;
  while (notfinished) {
    MmSNit nit;
    notfinished = false;
    for (MmSNit nit = _nodes.begin(); nit != _nodes.end(); ++nit) {
      if (nit != _nodes.begin()) {
        if (nit->first == lastlabel) {
          unique_dvar(lastlabel);
          notfinished = true;
          break;
        }
      }
      lastlabel = nit->first;
    }
  }

  // set top to ltop, unless ltop doesn't point to a node, then use index
  std::string top_rep = find_representative(mrs, var_name(mrs->ltop));
  if (_nodes.count(top_rep) > 0) {
    top = top_rep;
  } else {
    top = var_name(mrs->index);
  }
  //check for (and mark) cycles or fragmented graphs 
  fragmented = check_fragmented();
  cyclic = find_cycles();

  //extract triples
  if (_nodes.find(top) != _nodes.end()) {
    Triple *at = new Triple();
    at->ttype = "A"; at->first = "<0:0>"; at->matched = false;
    at->second = "ROOT"; at->third = _nodes.find(top)->second->link;
    _triples.insert(std::pair<std::string, Triple *>(at->first, at));
  }
  for (MmSNit it = _nodes.begin(); it != _nodes.end(); ++it) {
    if (it->second->link.empty() || it->second->link == "<-1:-1>" 
      || it->second->link == "<>")
      continue; //don't keep triples where the pred had no link
    Triple *t = new Triple();
    t->ttype = "N"; t->first = it->second->link; t->matched = false;
    t->second = "PRED"; t->third = std::string("\""+it->second->pred_name);
    if (!it->second->carg.empty()) {
      t->third += std::string("(\\\"");
      t->third += it->second->carg;
      t->third += std::string("\\\")");
    }
    t->third += "\"";
    _triples.insert(std::pair<std::string, Triple *>(t->first, t));

    for (std::vector<tEdsEdge *>::iterator eit = it->second->outedges.begin();
      eit != it->second->outedges.end(); ++eit) {
      if (_nodes.count((*eit)->target) > 0) {
        t = new Triple();
        t->ttype = "A"; t->first = it->second->link; t->matched = false;
        t->second = (*eit)->edge_name;
        t->third = _nodes.find((*eit)->target)->second->link;
        _triples.insert(std::pair<std::string, Triple *>(t->first, t));
      }
    }
    for (std::map<std::string, std::string>::iterator pit 
          = it->second->properties.begin(); 
          pit != it->second->properties.end(); ++pit) {
      t = new Triple();
      t->ttype = "P"; t->first = it->second->link; t->matched = false;
      t->second = pit->first; t->third = pit->second;
      _triples.insert(std::pair<std::string, Triple *>(t->first, t));
    }
  }
}

tEds::~tEds() {
  for(MmSNit it = _nodes.begin(); it != _nodes.end(); ++it)
    delete it->second;
  for (std::multimap<std::string, Triple *>::iterator it = _triples.begin();
    it != _triples.end(); ++it)
    delete it->second;
}

void tEds::print_eds() {
  std::cout << "{" << top << ":";
  if (cyclic || fragmented) {
    std::cout << " (";
    if (cyclic) {
      std::cout << "cyclic";
      if (fragmented) std::cout << " ";
    }
    if (fragmented) std::cout << "fragmented";
    std::cout << ")";
  }
  std::cout << std::endl;
  for (std::vector<tEdsNode *>::iterator it = _orderedNodes.begin(); 
        it != _orderedNodes.end(); ++it) {
    std::cout << ((*it)->cyclic||(*it)->fragmented?"|":" ") 
      << (*it)->dvar_name << ":" << (*it)->pred_name
      << (*it)->link;
    if (!(*it)->carg.empty())
      std::cout << "(\"" << (*it)->carg << "\")";
    std::cout << "[";
    bool begun = false;
    for (std::vector<tEdsEdge *>::iterator eit = (*it)->outedges.begin();
      eit != (*it)->outedges.end(); ++eit) {
      if ((*eit)->target.empty()) continue;
      if ((_nodes.find((*eit)->target)) == _nodes.end()) continue;
      if (begun) 
        std::cout << ", ";
      begun = true;
      std::cout << (*eit)->edge_name << " " 
        << _nodes.find((*eit)->target)->second->dvar_name; 
    }
    std::cout << "]" << std::endl;
  }

  std::cout << "}\n" << std::endl;
}

void tEds::print_triples() {
  for (std::multimap<std::string, Triple *>::iterator it = _triples.begin();
        it != _triples.end(); ++it) {
    if (it->second->second == "ROOT")
      std::cout << "R " << it->second->third << std::endl;
    else
      std::cout << it->second->ttype << " " << it->second->first << " "
        << it->second->second << " " << it->second->third << std::endl;
  }
}

tEdsComparison *tEds::compare_triples(tEds *b, const char *type, 
  bool ignoreroot) {
  tEdsComparison *result = new tEdsComparison();
  result->totalA["ALL"] = 0; result->totalA["A"] = 0;
  result->totalA["N"] = 0; result->totalA["P"] = 0;
  result->totalB["ALL"] = 0; result->totalB["A"] = 0;
  result->totalB["N"] = 0; result->totalB["P"] = 0;
  result->totalM["ALL"] = 0; result->totalM["A"] = 0;
  result->totalM["N"] = 0; result->totalM["P"] = 0;
  typedef std::multimap<std::string, Triple *>::iterator MmSTit;

  for (MmSTit it = _triples.begin(); it != _triples.end(); ++it) {
    if (ignoreroot && it->second->second == "ROOT") continue;
    it->second->matched = false;
    result->totalA["ALL"]++;
    result->totalA[it->second->ttype]++;
    if (result->totalA.count(it->second->second) == 0)
      result->totalA[it->second->second] = 0;
    result->totalA[it->second->second]++;
  }

  if (b != NULL) {
    for (MmSTit it = b->_triples.begin(); it != b->_triples.end(); ++it) {
      if (ignoreroot && it->second->second == "ROOT") continue;
      it->second->matched = false;
      result->totalB["ALL"]++;
      result->totalB[it->second->ttype]++;
      if (result->totalB.count(it->second->second) == 0)
        result->totalB[it->second->second] = 0;
      result->totalB[it->second->second]++;
    }

    for (MmSTit it = _triples.begin(); it != _triples.end(); ++it) {
      if (ignoreroot && it->second->second == "ROOT") continue;
      std::pair<MmSTit, MmSTit> spanends = b->_triples.equal_range(it->first);
      for (MmSTit bit = spanends.first; bit != spanends.second; ++bit) {
        if (bit->second->matched == false //not already matched
            && it->second->ttype == bit->second->ttype
            && it->second->second == bit->second->second 
            && it->second->third == bit->second->third) {
          it->second->matched = true;
          bit->second->matched = true;
          result->totalM["ALL"]++; result->totalM[it->second->ttype]++;
          if (result->totalM.count(bit->second->second) == 0) 
            result->totalM[bit->second->second] = 0;
          result->totalM[bit->second->second]++;
          break;
        }
      }
    }
    for (MmSTit it = b->_triples.begin(); it != b->_triples.end(); ++it) {
      if (ignoreroot && it->second->second == "ROOT") continue;
      if (!it->second->matched) {
        std::string umtriple = std::string(it->second->ttype + " " 
          + it->second->first + " " + it->second->second + " "
          + it->second->third);
        result->unmatchedB.push_back(umtriple);
      }
    }
  }
  for (MmSTit it = _triples.begin(); it != _triples.end(); ++it) {
    if (ignoreroot && it->second->second == "ROOT") continue;
    if (!it->second->matched) {
      std::string umtriple = std::string(it->second->ttype + " " 
        + it->second->first + " " + it->second->second + " "
        + it->second->third);
      result->unmatchedA.push_back(umtriple);
    }
  }

  double precision, recall;
  if (result->totalA.count(type) == 0 || result->totalA[type] == 0)
    result->score = -1; //no gold triples of type /type/
  else {
    if (result->totalB.count(type) == 0 || result->totalM.count(type) == 0
        || result->totalB[type] == 0 || result->totalM[type] == 0) {
      result->score = 0;
    } else {
      precision = (double)result->totalM[type]/result->totalB[type];
      recall = (double)result->totalM[type]/result->totalA[type];
      result->score = (2*precision*recall)/(precision+recall);
    }
  }
  return result;
}


void tEds::read_eds(std::string input) {

//  // temporary mappings to find the correct target node
//  typedef std::multimap<std::string, int>::iterator MmSIit;
//  std::multimap<std::string, int> dvar2nodes;
//  std::map<std::string, int> representatives; 
//
//  std::string rest = input;
//
//  parseChar('{', rest);
//  top = parseVar(rest);
//  parseChar(':', rest);
//  if (rest.at(0) == '|') //skip over fragment/cyclic markers for now
//    rest.erase(0,1);
//  std::string var = parseVar(rest);
//  while (!var.empty()) {
//    dvar2nodes.insert(std::pair<std::string,int>(var, read_node(var, rest)));
//    if (rest.at(0) == '|') //skip over fragment/cyclic markers for now
//      rest.erase(0,1);
//    var = parseVar(rest);
//  }
//  parseChar('}', rest);
//  if (!rest.empty())
//    throw tError("ignoring trailing data: \"" + rest + "\"");
//  
//  // fix the edges to point to the representative node
//  for (std::vector<tEdsNode *>::iterator nit = _nodes.begin();
//        nit != _nodes.end(); ++nit) {
//    for (std::vector<tEdsEdge *>::iterator eit = (*nit)->outedges.begin();
//          eit != (*nit)->outedges.end(); ++eit) {
//      if (representatives.count((*eit)->target_name) == 1) {
//        //we've looked this one up before, just set the target
//        (*eit)->target = representatives[(*eit)->target_name];
//      } else {
//        if (dvar2nodes.count((*eit)->target_name) > 0) {
//          //instantiated arg0
//          std::set<int> candidates;
//          std::pair<MmSIit,MmSIit> spanends 
//            = dvar2nodes.equal_range((*eit)->target_name);
//          for (MmSIit cit = spanends.first; cit != spanends.second; ++cit) {
//            if (!_nodes[cit->second]->quantifier_node()) 
//              candidates.insert(cit->second);
//          }
//          if (candidates.size() == 1) {
//            (*eit)->target = *(candidates.begin());
//            representatives[(*eit)->target_name] = (*eit)->target;
//          } else {
//            int t = select_candidate(candidates);
//            (*eit)->target = t;
//            representatives[(*eit)->target_name] = t;
//          }
//        }
//      }
//    }
//  }
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
  } else {
    varname = std::string();
  }
  removeWhitespace(rest);
  return varname;
}

int tEds::read_node(std::string id, std::string &rest) {
//  std::string predname, span;
//  int to, from, nodenumber;
//  parseChar(':', rest);
//  while(!rest.empty() && !isspace(rest.at(0)) && rest.at(0) != '<' 
//        && rest.at(0) != '[') {
//    predname += rest.at(0);
//    rest.erase(0,1);
//  }
//  if (rest.at(0) == '<') { //link
//    while (!rest.empty() && isgraph(rest.at(0)) && rest.at(0) != '>') {
//      span += rest.at(0);
//      rest.erase(0,1);
//    }
//    if (rest.at(0) == '>') {
//      span += rest.at(0);
//      rest.erase(0,1);
//    } else {
//      throw tError("Unterminated span at \"" + rest + "\".");
//    }
//    unsigned int colon = span.find(':');
//    if (colon == std::string::npos)
//      throw tError("Ill-formed span \"" + span + "\".");
//    std::istringstream fromstr(span.substr(1, colon-1));
//    fromstr >> from;
//    std::istringstream tostr(span.substr(colon+1));
//    tostr >> to;
//  } else { //no link
//    span = std::string("<-1:-1>");
//    to = -1;
//    from = -1;
//  }
//  tEdsNode *node = new tEdsNode(predname, id, "", from, to);
//  node->link = span;
//  _nodes.push_back(node);
//  nodenumber = _nodes.size()-1;
//  removeWhitespace(rest);
//  parseChar('[', rest);
//  while (rest.at(0) != ']') {
//    std::string reln, targetvar, targetname, targetcarg;
//    while (!rest.empty() && !isspace(rest.at(0))) {
//      reln += rest.at(0);
//      rest.erase(0,1);
//    }
//    removeWhitespace(rest);
//    targetvar = parseVar(rest);
//    if (rest.at(0) == ':') {
//      parseChar(':', rest);
//      while(!rest.empty() && !isspace(rest.at(0)) && rest.at(0) != '(' 
//            && rest.at(0) != ']' && rest.at(0) != ',') {
//        targetname += rest.at(0);
//        rest.erase(0,1);
//      }
//      if (rest.at(0) == '(') { //carg
//        while(!rest.empty() && rest.at(0) != ')') {
//          targetcarg += rest.at(0);
//          rest.erase(0,1);
//        }
//        if (rest.at(0) == ')') 
//          rest.erase(0,1);
//        else
//          throw tError("Unterminated carg at \"" + rest + "\".");
//      }
//    }
//    removeWhitespace(rest);
//    _nodes.back()->add_edge(new tEdsEdge(-1, reln, targetvar));
//    // add edge. add target node?
////    int targetnode = find_node(targetvar, targetname);
//
//    if (rest.at(0) == ',')
//      rest.erase(0,1);
//    removeWhitespace(rest);
//  }
//  parseChar(']', rest);
//  return nodenumber;
  return 0;
}

std::string tEds::var_name(tVar *v) {
  std::ostringstream name;
  name << v->type << v->id;
  return name.str();
}

//pick the var with most outgoing links as representative when two 'real' EPs
//share an ARG0, then give the others new dvars
void tEds::unique_dvar(std::string label) {
  std::pair<MmSNit, MmSNit> spanends = _nodes.equal_range(label);
  int maxedges = -1;
  MmSNit candidate = _nodes.end();
  for (MmSNit it = spanends.first; it != spanends.second; ++it) {
    int edges = it->second->outedges.size();
    if (edges > maxedges) {
      maxedges = edges;
      candidate = it;
    }
  }
  if (candidate == _nodes.end())
    candidate = spanends.first;
  for (MmSNit it = spanends.first; it != spanends.second && it->first == label;
        ++it) {
    if (it != candidate) {
      tEdsNode *nodecopy = new tEdsNode(*(it->second));
      std::ostringstream name;
      name << "_" << _counter++;
      nodecopy->dvar_name = name.str();
      MmSNit newnode =
        _nodes.insert(std::pair<std::string, tEdsNode*>(nodecopy->dvar_name, 
        nodecopy));
      std::pair<MmSMit,MmSMit> hspanends = 
        handle2nodes.equal_range(nodecopy->handle_name);
      for (MmSMit hit = hspanends.first; 
            hit != hspanends.second && hit->first == nodecopy->handle_name; 
            ++hit) {
        if (hit->second == it) {
          handle2nodes.erase(hit);
          break;
        }
      }
      handle2nodes.insert(std::pair<std::string,
        MmSNit>(nodecopy->handle_name, newnode));
      delete(it->second);
      _nodes.erase(it);
    }
  }
}

std::string tEds::find_representative(tMrs *mrs, std::string hdl) {
  std::set<std::string> candidates;
  std::string target = hdl;
  //grab nodes that match handle directly
  std::pair<MmSMit,MmSMit> spanends = handle2nodes.equal_range(hdl);
  for (MmSMit cit = spanends.first; cit != spanends.second; ++cit) 
    candidates.insert(cit->second->second->dvar_name);
  //grab nodes that match handle via hcons
  for (std::vector<tHCons *>::iterator hit = mrs->hconss.begin();
        hit != mrs->hconss.end(); ++hit) {
    if (var_name((*hit)->harg) == hdl) {
      std::string larg = var_name((*hit)->larg);
      spanends = handle2nodes.equal_range(larg);
      for (MmSMit cit = spanends.first; cit != spanends.second; ++cit)
        candidates.insert(cit->second->second->dvar_name);
    }
  }
  if (candidates.size() == 1) {
    target = *(candidates.begin());
  } else { //this is where we start heuristics for shared handles
    std::vector<std::string> newcandidates;
    // grab candidates that does not point to other candidates
    for (std::set<std::string>::iterator cit = candidates.begin();
      cit != candidates.end(); ++cit) {
      int args = 0;
      std::pair<MmSNit, MmSNit> noderange = _nodes.equal_range(*cit);
      for (MmSNit cn = noderange.first; cn != noderange.second; ++cn) {
        for (std::vector<tEdsEdge *>::iterator outit 
            = cn->second->outedges.begin();
            outit != cn->second->outedges.end(); ++outit) {
            std::string edge = (*outit)->target;
          for (std::set<std::string>::iterator cit2 = candidates.begin();
            cit2 != candidates.end(); ++cit2) {
            if (cit == cit2) continue;
            if (*cit2 == edge) args++;
          }
        }
      }
      if (args == 0) newcandidates.push_back(*cit);
    }
    if (newcandidates.size() == 1) {
      target = *(newcandidates.begin());
    } else {//starting to scrape the barrel now
      int maxincoming = -1;
      std::string candidate;
      // grab candidate with most incoming links
      for (std::vector<std::string>::iterator cit = newcandidates.begin();
        cit != newcandidates.end(); ++cit) {
        int incoming = 0;
        for (MmSNit anit = _nodes.begin(); anit != _nodes.end(); ++anit) {
          for (std::vector<tEdsEdge *>::iterator aoutit 
              = (*anit).second->outedges.begin(); 
              aoutit != (*anit).second->outedges.end(); ++aoutit) {
            if ((*aoutit)->target == *cit) ++incoming;
          }
        }
        if (incoming > maxincoming) {
          candidate = *cit;
          maxincoming = incoming;
        }
      }
      if (!candidate.empty()) target = candidate;
    }
  }
  return target;
}

//mark all nodes not connected to top as fragmented
bool tEds::check_fragmented() {
  bool fragmented = false;
  std::set<std::string> *marked = new std::set<std::string>;
  //mark nodes directly reachable from top
  follow_links(top, marked);
  bool notfinished = true;
  //until the marked list stops growing
  while (notfinished) {
    notfinished = false;
    //follow each node until it reaches a marked node (or end)
    for(MmSNit it = _nodes.begin(); it != _nodes.end(); ++it) {
      std::set<std::string> *seen = new std::set<std::string>;
      if (follow_links(it->first, seen, marked)) {
        unsigned int setsize = marked->size();
        marked->insert(seen->begin(), seen->end());
        if (marked->size() > setsize) {//changed marked set
          notfinished = true;
        }
      }
      delete seen;
    }
  }
  //find all nodes not marked, and mark them fragmented
  for(MmSNit it = _nodes.begin(); it != _nodes.end(); ++it) {
    if (marked->count(it->first) != 1) {
      fragmented = true;
      it->second->fragmented = true;
    } else {
      it->second->fragmented = false;
    }
  }
  delete marked;
  return fragmented;
}

bool tEds::find_cycles() {
  bool cyclic = false;
  for(MmSNit it = _nodes.begin(); it != _nodes.end(); ++it) {
    std::set<std::string> seen;
    if (follow_links(it->first, seen)) {
      it->second->cyclic = true;
      cyclic = true;
    } else {
      it->second->cyclic = false;
    }
  }
  return cyclic;
}

//two versions of follow_links(): 
// * one keeps track only of antecedents (to detect cycles)
// * one keeps track of all nodes visited, and also has a fixed node list for
//    storing all marked nodes
// TODO Generalise? 

//detecting cycles
bool tEds::follow_links(std::string val, std::set<std::string> seen) {
  MmSNit tnode = _nodes.find(val);
  if (tnode == _nodes.end() || tnode->second->quantifier) return false;
  for (std::vector<tEdsEdge *>::iterator e = tnode->second->outedges.begin();
    e != tnode->second->outedges.end(); ++e) {
    if (seen.count((*e)->target) == 1) {
      return true;
    } else {
      std::set<std::string> newseen = seen;
      newseen.insert(val);
      if (follow_links((*e)->target, newseen)) return true;
    }
  }
  return false;
}

//visiting nodes to see if there's a connection to the marked nodes
//first time descend from top, marked is NULL and seen becomes the marked list
//for all other iterations
bool tEds::follow_links(std::string val, std::set<std::string> *seen, 
  std::set<std::string> *marked) {
  MmSNit tnode = _nodes.find(val);
  bool mark = false;
  if (tnode == _nodes.end()) return false;
  seen->insert(val);
  for (std::vector<tEdsEdge *>::iterator e = tnode->second->outedges.begin();
    e != tnode->second->outedges.end(); ++e) {
    if (marked != NULL && marked->count((*e)->target) == 1) {
      mark = true;
    } else {
      if (seen->count((*e)->target) == 0) {//don't get stuck in a cycle
        if (follow_links((*e)->target, seen, marked)) mark = true;
      }
    }
  }
  return mark;
}

//replace these placeholders with something more flexible

std::string tEds::pred_normalize(std::string pred) {
  std::string normedpred = pred;
  boost::algorithm::to_lower(normedpred);
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
  if (var.empty()) return false;
  return var.at(0) == 'h';
}

} // namespace mrs


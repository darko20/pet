/* ex: set expandtab ts=2 sw=2: */

#ifndef _EDS_H_
#define _EDS_H_

#include <vector>
#include <map>
#include <string>
#include <set>

#include "mrs.h"

namespace mrs {

class tEds; //forward declaration

class tEdsEdge {
  friend class tEds;

  public:
    int target;
    std::string edge_name;

    tEdsEdge(): target(-1) {};
    tEdsEdge(int t, std::string en, std::string tn) : target(t), 
      edge_name(en), target_name(tn) {};
  private:
    std::string target_name;
};

class tEdsNode {
  friend class tEds;

  public:
    std::string pred_name, dvar_name, link, carg;
    int cfrom, cto;
    std::vector<tEdsEdge *> outedges;

    tEdsNode() {};
    tEdsNode(std::string pred, std::string dvar, std::string handle, int from, 
      int to) : pred_name(pred), dvar_name(dvar), 
      cfrom(from), cto(to), handle_name(handle) {};
    ~tEdsNode();
      
    void add_edge(tEdsEdge *edge);
    bool quantifier_node();
//    remove_edge();
  private:
    std::string handle_name; //do we need to expose the handle?

};

/* Elementary Dependency Graph (EDG) class */

class tEds {
  public:
    tEds();
    tEds(tMrs *mrs);
    ~tEds();

		void read_eds(std::string input);
		void print_eds();
		void print_triples() {};

    std::string top;

  private:
    int _counter; //for new quant vars
    std::vector<tEdsNode *> _nodes;
    std::map<std::string, tVar*> _vars_map;

    void removeWhitespace(std::string &rest);
    void parseChar(char x, std::string &rest);
    std::string parseVar(std::string &rest);
    int read_node(std::string var, std::string &rest);
    std::string pred_normalize(std::string pred);
    std::string var_name(tVar *v);
    tVar *get_id(tEp *ep);
    bool carg_rel(std::string role);
    bool relevant_rel(std::string role);
    int select_candidate(std::set<int> candidates);
    bool handle_var(std::string var);
};

} //namespace mrs


#endif

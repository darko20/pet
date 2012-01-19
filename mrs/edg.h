/* ex: set expandtab ts=2 sw=2: */

#ifndef _EDG_H_
#define _EDG_H_

#include <vector>
#include <map>
#include <string>
#include <set>

#include "mrs.h"

namespace mrs {

class tEdgEdge {
  public:
    int target;
    std::string edge_name;
    std::string target_name;

    tEdgEdge(): target(-1) {};
    tEdgEdge(int t, std::string en, std::string tn) : target(t), 
      edge_name(en), target_name(tn) {};
};

class tEdgNode {
  public:
    std::string pred_name, dvar_name, handle_name, link, carg;
    int cfrom, cto;
    std::vector<tEdgEdge *> outedges;

    tEdgNode() {};
    tEdgNode(std::string pred, std::string dvar, std::string handle, int from, 
      int to) : pred_name(pred), dvar_name(dvar), handle_name(handle), 
      cfrom(from), cto(to) {};
    ~tEdgNode();
      
    void add_edge(tEdgEdge *edge);
    bool quantifier_node();
//    remove_edge();

};

/* Elementary Dependency Graph (EDG) class */

class tEdg {
  public:
    tEdg();
    tEdg(tMrs *mrs);
    ~tEdg();

		void read_edg(std::string input);
		void print_edg();
		void print_triples() {};
    std::string pred_normalize(std::string pred);
    std::string var_name(tVar *v);
    tVar *get_id(tEp *ep);
    bool carg_rel(std::string role);
    bool relevant_rel(std::string role);
    int select_candidate(std::set<int> candidates);
    bool handle_var(std::string var);

    std::string top;

  private:
    std::vector<tEdgNode *> _nodes;
    std::map<std::string, tVar*> _vars_map;
    void removeWhitespace(std::string &rest);
    void parseChar(char x, std::string &rest);
    std::string parseVar(std::string &rest);
    void read_node(std::string var, std::string &rest);
};

} //namespace mrs


#endif

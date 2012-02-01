/* ex: set expandtab ts=2 sw=2: */

#ifndef _EDS_H_
#define _EDS_H_

#include <vector>
#include <map>
#include <string>
#include <set>

#include "mrs.h"

namespace mrs {

/* Elementary Dependency Graph (EDG) class */

class tEds {
  //internal classes: edges and nodes
  class tEdsEdge {
    friend class tEds;

    int target;
    std::string edge_name;
    std::string target_name;

    tEdsEdge(): target(-1) {};
    tEdsEdge(int t, std::string en, std::string tn) : target(t), 
      edge_name(en), target_name(tn) {};
  };

  class tEdsNode {
    friend class tEds;

    std::string pred_name, dvar_name, link, carg, handle_name; 
    int cfrom, cto;
    bool quantifier;
    std::vector<tEdsEdge *> outedges;
    std::map<std::string, std::string> properties;

    tEdsNode() : quantifier(false) {};
    tEdsNode(std::string pred, std::string dvar, std::string handle, int from, 
      int to) : pred_name(pred), dvar_name(dvar), cfrom(from), cto(to), 
      handle_name(handle), quantifier(false) {};
    ~tEdsNode();
    void add_edge(tEdsEdge *edge);
    bool quantifier_node();
  };

  public:
    tEds():_counter(1) {};
    tEds(tMrs *mrs);
    ~tEds();

		void read_eds(std::string input);
		void print_eds();
		void print_triples();

    std::string top;

  private:
    int _counter; //for new quant vars
    std::vector<tEdsNode *> _nodes;

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
    bool quantifier_pred(std::string);

    typedef std::pair<std::string, std::pair<std::string, std::string> > Triple;
    typedef std::pair<std::string, std::string> PSS;
    std::vector<Triple> argTriples;
    std::vector<Triple> propTriples;
    std::map<std::string, std::vector<int> > linkToArgTriples;
    std::map<std::string, std::vector<int> > linkToPropTriples;
    std::map<std::string, std::vector<int> > linkToNodes;
};

} //namespace mrs


#endif

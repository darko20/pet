/* ex: set expandtab ts=2 sw=2: */

#ifndef _EDS_H_
#define _EDS_H_

#include <vector>
#include <map>
#include <string>
#include <set>
#include <sstream>

#include "mrs.h"

namespace mrs {

struct tEdsComparison {
  double score;
  std::map<std::string, int> totalA;
  std::map<std::string, int> totalB;
  std::map<std::string, int> totalM;
  std::vector<std::string> unmatchedA;
  std::vector<std::string> unmatchedB;
};

struct Triple {
  std::string ttype, first, second, third;
  bool matched;
};

/* Elementary Dependency Graph (EDG) class */
class tEds {

  //internal classes: edges and nodes
  class tEdsEdge {
    friend class tEds;

    std::string edge_name;
    std::string target;

    tEdsEdge(){};
    tEdsEdge(std::string en, std::string tn) : 
      edge_name(en), target(tn) {};
  };

  class tEdsNode {
    friend class tEds;

    std::string pred_name, dvar_name, link, carg, handle_name; 
    int cfrom, cto;
    bool quantifier;
    bool cyclic;
    bool fragmented;
    std::vector<tEdsEdge *> outedges;
    std::map<std::string, std::string> properties;

    tEdsNode() : quantifier(false) {};
    tEdsNode(std::string pred, std::string dvar, std::string handle, int from, 
      int to) : pred_name(pred), dvar_name(dvar), handle_name(handle), 
      cfrom(from), cto(to), quantifier(false) {};
    tEdsNode(tEdsNode &other);
    ~tEdsNode();
    void add_edge(tEdsEdge *edge);
  };

  public:
    typedef std::multimap<std::string, tEdsNode *>::iterator MmSNit;
    tEds():_counter(1) {};
    tEds(tMrs *mrs);
    ~tEds();

		void read_eds(std::string input);
		void print_eds();
		void print_triples();
    tEdsComparison *compare_triples(tEds *b, const char *type="ALL",
      bool ignoreroot=false);

    std::string top;
    bool cyclic;
    bool fragmented;

  private:
    int _counter; //for new quant vars
    std::multimap<std::string, tEdsNode *> _nodes;
    std::multimap<std::string, Triple *> _triples;
		std::vector<tEdsNode *> _orderedNodes;	
    // temporary mappings to find the correct target node
    typedef std::multimap<std::string, MmSNit>::iterator MmSMit;
    std::multimap<std::string, MmSNit> handle2nodes;


    void removeWhitespace(std::string &rest);
    void parseChar(char x, std::string &rest);
    std::string parseVar(std::string &rest);
    int read_node(std::string var, std::string &rest);
    std::string pred_normalize(std::string pred);
    std::string var_name(tVar *v);
    tVar *get_id(tEp *ep);
    bool carg_rel(std::string role);
    bool relevant_rel(std::string role);
    void unique_dvar(std::string label);
    bool handle_var(std::string var);
    bool quantifier_pred(std::string);
    std::string find_representative(tMrs *mrs, std::string hdl);
    bool check_fragmented();
    bool find_cycles();
    bool follow_links(std::string val, std::set<std::string> seen);
    bool follow_links(std::string val, std::set<std::string> *seen, 
      std::set<std::string> *marked=NULL);

};

} //namespace mrs


#endif

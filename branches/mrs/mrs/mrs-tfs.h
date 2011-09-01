/* -*- Mode: C++ -*- */

/* This file provides methods of extracting MRS from the TFS */

#ifndef _MRS_TFS_H_
#define _MRS_TFS_H_

#include "mrs.h"
#include "dag.h"

#include <map>


namespace mrs {
  class MrsTfsExtractor {
    
  public:
    MrsTfsExtractor() : _vid_geneator(1) {
    }
    
    ~MrsTfsExtractor() {
    }
    
    tMrs* extractMrs(struct dag_node* dag);

    tVar* requestVar(std::string type);
    tVar* requestVar(struct dag_node* dag);
    
    tHCons* extractHCons(struct dag_node* dag, tMrs* mrs);
    tVar* extractVar(int vid, struct dag_node* dag);
    tEp* extractEp(struct dag_node* dag, tMrs* mrs);

    void createVarPropertyList(struct dag_node* dag, std::string path, 
			       tVar* var);

  private:
    std::map<dag_node*,mrs::tVar*> _named_nodes;
    int _vid_generator;
  };

} // namespace mrs
    
#endif /* _MRS_TFS_H_ */

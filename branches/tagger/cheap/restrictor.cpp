/** \file restrictor.cpp
 * Different restrictor implementations.
 * To avoid overhead, the different restrictors should be used by template
 * instantiations instead of using the \c virtual mechanism.
 * 
 * Contains several final template specializations and static variables.
 */

#include "restrictor.h"

/*========================= dag restrictor ============================= */

type_t dag_restrictor::DEL_TYPE = 0, dag_restrictor::ONLY_TYPE = 0;

const dag_node *dag_restrictor::_copyall = NULL;
const dag_node *dag_restrictor::_deletearc = (dag_node *) -1;

template<>
const dag_node *walk_arc(const dag_node * const &dag, attr_t attr) {
  assert(dag != NULL);
  dag_arc *new_arc = dag_find_attr(dag->arcs, attr);
  if (new_arc != NULL) {
    if (new_arc->val->type == dag_restrictor::DEL_TYPE)
      // arc should be neutralized: target node says DEL
      return dag_restrictor::_deletearc;
    else
      return new_arc->val;  // continue recursive restriction
  } else {  // arc not found in restrictor
    if (dag->type == dag_restrictor::ONLY_TYPE)
      // current node type is ONLY: copy only specified arcs
      return dag_restrictor::_deletearc;
    else
      // end of description reached: copy all
      return dag_restrictor::_copyall;
  }
}

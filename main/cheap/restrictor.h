/* -*- Mode: C++ -*- */
/** \file restrictor.h
 * Different restrictor implementations.
 * To avoid overhead, the different restrictors should be used by template
 * instantiations instead of using the \c virtual mechanism.
 */

#ifndef _RESTRICTOR_H
#define _RESTRICTOR_H

#include "list-int.h"
#include "dag-tomabechi.h"

/** Pure virtual restrictor superclass */
class restrictor {
public:
  /** Clone \a dag using this restrictor 
   *
   * A restrictor is like a functor mapping feature structures to other by
   * deleting some part of the structure.
   * If the restrictor tells to remove a node, the node will be replaced by an
   * empty dag node with the maximal appropriate type of the feature pointing
   * to it.
   */
  virtual class dag_node *dag_partial_copy (class dag_node *) const = 0;
};

/** A restrictor that prunes all arcs that bear an attribute contained in a set
 *  of given attributes. 
 * list_int_restrictor is a prototype of a stateless restrictor. There could
 * even be another virtual superclass for stateless restrictors, but it seems
 * not worthwile at the moment.
 */
class list_int_restrictor : public restrictor {
  list_int *_del_arcs;

public:
  /** Create a restrictor from a list of unwanted attributes */
  list_int_restrictor(list_int *del) { _del_arcs = copy_list(del); }
  virtual ~list_int_restrictor() { free_list(_del_arcs); }

  /** Return true if this arc should be deleted */
  inline bool prune_arc(attr_t attr) const {
    return contains(_del_arcs, attr);
  }

  virtual class dag_node *dag_partial_copy (class dag_node * dag) const {
    return dag_partial_copy_stateless(dag, *this);
  }
};



/** A restrictor that prunes part of a feature structure on the basis of a
 *  dag-like structure.
 *
 *  At every node in the graph that contains a certain type (which is the same
 *  type as the type at the root of the restrictor dag), the dag will be
 *  pruned. This is a prototype of a restrictor using restrictor states.
 */
class dag_restrictor : public restrictor {

  /**  Restrictor_state:
   *  A \c restrictor_state has to be a lightweight object because lots of them
   *  are created and destroyed during copy/restriction of a feature
   *  structure. So, almost no internal state and no virtual functions.
   *  To save space, all dag restrictors have to use the same type for
   *  deletion which is stored in a static variable.
   */
  class dag_rest_state {
    class dag_node *_state;
    static type_t DEL_TYPE;
    
  public:
    dag_rest_state(dag_node *root) : _state(root) {
      DEL_TYPE = root->type;
    }
    
    /** Return \c true if from now on all nodes should be copied */
    inline bool full() const { return _state == NULL; }
    
    /** Return \c true if from now on nothing should be copied 
     * Don't call this function if \c full() did return \c true.
     */
    inline bool empty() const {
      assert (_state != NULL); 
      return (_state->type == DEL_TYPE);
    }
    
    /** Return a new restrictor state corresponding to a transition
     *  via \a attr.
     * Don't call this function if \c full() did return \c true.
     */
    inline dag_rest_state walk_arc(attr_t attr) const {
      assert (_state != NULL); 
      dag_arc *new_arc = dag_find_attr(_state->arcs, attr);
      if (new_arc != NULL) 
        return dag_rest_state(new_arc->val);
      else
        return dag_rest_state(NULL);
    }
  };

  dag_rest_state _root_state;

public:
  /** Create a restrictor object that is encoded in the dag \a del */
  dag_restrictor(class dag_node *del) : _root_state(del) { }
  //virtual ~dag_restrictor() { }

  virtual class dag_node *dag_partial_copy (class dag_node *dag) const {
    return dag_partial_copy_state(dag, _root_state);
  }
};

#if 0
// I built this to test the restrictor functionality. The stateless list
// restrictor surely is more efficient.
/** Restrictor: an object implementing a functor that deletes parts of a
 *  feature structure.
 *  Restrictor_state:
 *  A \c restrictor_state has to be a lightweight object because lots of them
 *  are created and destroyed during copy/restriction of a feature
 *  structure. So, almost no internal state and no virtual functions.
 */
class li_rest_state {
  list_int *_del_arcs;
  static list_int EMPTY;

public:
  restrictor(list_int *del) : _del_arcs(del) {}
 
  /** Return \c true if from now on all nodes should be copied */
  inline bool full() const { return false; }

  /** Return \c true if from now on nothing should be copied 
   * Don't call this function if \c full() did return \c true.
   */
  inline bool empty() const { return (_del_arcs == &EMPTY); }

  /** Return a new restrictor state corresponding to a transition via \a attr.
   * Don't call this function if \c full() did return \c true.
   */
  inline restrictor walk_arc(attr_t attr) const {
    if(contains(_del_arcs, attr))
      return restrictor(&EMPTY);
    else
      return restrictor(_del_arcs);
  }
};
#endif


#endif

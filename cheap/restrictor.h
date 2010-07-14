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
  virtual ~restrictor() {}

  /** Clone \a dag using this restrictor 
   *
   * A restrictor is like a functor mapping feature structures to other by
   * deleting some part of the structure.
   * If the restrictor tells to remove a node, the node will be replaced by an
   * empty dag node with the maximal appropriate type of the feature pointing
   * to it.
   */
  virtual dag_node *dag_partial_copy (dag_node*) const = 0;
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

  virtual dag_node *dag_partial_copy (dag_node* dag) const {
    return dag_partial_copy_stateless(dag, *this);
  }
};


/** Restrictor: an object implementing a functor that deletes parts of a
 *  feature structure.
 *
 * Restrictor_state:
 * A \c restrictor_state has to be a lightweight object because lots of them
 * are created and destroyed during copy/restriction of a feature
 * structure. So, almost no internal state and no virtual functions.
 *
 * This special restrictor gets paths as input specification and deletes the
 * nodes that are at the end of the paths. The paths don't have to start at the
 * root of the dag but can be arbitrary sub-paths in a feature structure.
 */
class path_restrictor : public restrictor {
  class tree_node {
    typedef std::pair<attr_t, tree_node> tree_arc;
    typedef std::list< tree_arc > dtrs_list;

    dtrs_list _dtrs;

    tree_node *walk_arc_internal(attr_t a) {
      for(dtrs_list::iterator it=_dtrs.begin(); it != _dtrs.end(); it++){
        if (it->first == a) return &(it->second);
      }
      return NULL;
    }

    void add_path_internal(const std::list<int> &l,
                           std::list<int>::const_iterator it) {
      if (it != l.end()) {
        tree_node *sub = walk_arc_internal(l.front());
        if (sub == NULL) {
          _dtrs.push_front(tree_arc(l.front(), tree_node()));
          sub = &(_dtrs.front().second) ;
        }
        sub->add_path_internal(l, it++);
      }
    }

  public:
    const tree_node *walk_arc(attr_t a) const {
      for(dtrs_list::const_iterator it=_dtrs.begin(); it != _dtrs.end(); it++){
        if (it->first == a) return &(it->second);
      }
      return NULL;
    }

    inline void add_path(const std::list<int> &l) { 
      add_path_internal(l, l.begin());
    }

    inline void add_path(const list_int *l) { 
      if (l != NULL) {
        tree_node *sub = walk_arc_internal(first(l));
        if (sub == NULL) {
          _dtrs.push_front(tree_arc(first(l), tree_node()));
          sub = &(_dtrs.front().second) ;
        }
        sub->add_path(rest(l));
      }
    }

    inline bool leaf_p() const { return _dtrs.empty(); }
  };

public:
  class state { 
    typedef std::list< const tree_node * > node_list;

    const tree_node *_root;
    node_list _nodes;

    void add_node(const tree_node *node) { _nodes.push_front(node); }
    void dead() { _nodes.clear(); }

  public:
    state(const tree_node *root) {
      _root = root;
      add_node(root);
    }

    /** Return \c true if from now on all nodes should be copied */
    inline bool full() const { return false; }
    
    /** Return \c true if from now on nothing should be copied 
     * Don't call this function if \c full() did return \c true.
     */
    inline bool empty() const { return _nodes.empty(); }
    
    /** Return a new restrictor state corresponding to a transition via
     *  \a attr.
     *  Don't call this function if \c full() did return \c true.
     */
    inline state walk_arc(attr_t attr) const {
      // You can always start at the root node in a living restrictor state,
      // therefore, the root node is added in the constructor
      state result(_root);
      for(node_list::const_iterator it = _nodes.begin()
            ; it != _nodes.end();it++){
        const tree_node *sub = (*it)->walk_arc(attr);
        // Did we find a subpath from that node for attr?
        if (sub != NULL) {
          // if *sub is a leaf, the returned restrictor shoule encode the
          // information that this node should be deleted
          if (sub->leaf_p()) {
            result.dead();
            return result; // smash it
          } else {
            // This is a possible sub-node for the result restrictor state
            result.add_node(sub);
          }
        }
      }
      return result;
    }
  };

private:
  tree_node _paths_to_delete;
  state _root_state;

public:
  /** Create a restrictor object that is encoded in the list of paths given by
   *  \a paths
   */
  path_restrictor(std::list< std::list<int> > paths)
    : _root_state(&_paths_to_delete) {
    for(std::list< std::list<int> >::iterator it = paths.begin()
          ; it != paths.end(); it++) {
      _paths_to_delete.add_path(*it);
    }
  }

  /** Create a restrictor object that is encoded in the list of paths given by
   *  \a paths
   */
  path_restrictor(std::list< list_int * > paths)
    : _root_state(&_paths_to_delete) {
    for(std::list< list_int * >::iterator it = paths.begin()
          ; it != paths.end(); it++) {
      _paths_to_delete.add_path(*it);
    }
  }

  virtual ~path_restrictor() {}
 
  virtual dag_node* dag_partial_copy (dag_node* dag) const
  {
    return dag_partial_copy_state(dag, _root_state);
  }
};

template< typename R_STATE > bool copy_full(const R_STATE& state);
template< typename R_STATE > bool empty(const R_STATE &state);
template< typename R_STATE > R_STATE walk_arc(const R_STATE &state, attr_t attr);

template<> inline bool
copy_full(const path_restrictor::state&) { return false; }

template<> inline bool
empty(const path_restrictor::state &state) { return state.empty(); }

template<> inline path_restrictor::state
walk_arc(const path_restrictor::state &state, attr_t attr) {
  return state.walk_arc(attr);
}


/** A restrictor that prunes part of a feature structure on the basis of a
 *  dag-like structure.
 *
 *  Every node in the graph may have one of two marker types: del or only
 *  del means:  delete the edges pointing to that node
 *  only means: keep only the edges that are mentionend in the restrictor dag
 *              and delete the others. Can also be used to strip a node down to
 *              its type
 *  This is a prototype of a restrictor using restrictor states.
 *  If this restrictor representation were to be used in multiple threads, one
 *  restrictor per thread would be necessary and sufficient because of the 
 *  stack, which has to be unique to one pass of the copy routine.
 */
class dag_restrictor : public restrictor {
public:
  static type_t DEL_TYPE, ONLY_TYPE;

  static const dag_node* _copyall;
  static const dag_node* _deletearc;

  const dag_node* _root;

public:
  /** Create a restrictor object that is encoded in the dag \a del */
  dag_restrictor(dag_node* del){ _root = del; }

  virtual ~dag_restrictor() { }

  virtual dag_node *dag_partial_copy (dag_node* dag) const
  {
    return dag_partial_copy_state(dag, _root); 
  }
};

template<> inline bool
copy_full(const dag_node* const &dag) {
  return dag == dag_restrictor::_copyall;
}

template<> inline bool
empty(const dag_node* const &dag) {
  return dag == dag_restrictor::_deletearc;
}

template<> const dag_node *
walk_arc(const dag_node* const &dag, attr_t attr);

#endif

#include "hashing.h"

class coref_map {
private:
  struct dag_node_hash : std::unary_function< struct dag_node *, size_t > {
    size_t operator()(const struct dag_node *a) const {
      return (size_t) a;
    }
  };

  typedef hash_map< struct dag_node *, int, dag_node_hash > node_map;
private:
  node_map _table;
  int _corefs;
public:
  coref_map() { clear(); }

  void clear() {
    _corefs = 0;
    _table.clear();
  }

  int node_add(dag_node *dag) {
    node_map::iterator it = _table.find(dag);
    if (it == _table.end()) {
      // We see this dag for the first time
      _table.insert(pair<dag_node *, int>(dag, 0));
      return -1;
    } else {
      if ((*it).second > 0) {
        // This dag has already been marked as a node with corefs
        return (*it).second;
      } else {
        // We see this dag for the second time: mark it as coreferenced
        return ((*it).second = ++_corefs);
      }
    }
  }

  int node_coref(dag_node *dag) {
    node_map::iterator it = _table.find(dag);
    assert(it != _table.end());
    if ((*it).second > 0) {
      return -((*it).second = - (*it).second);
    } else {
      return (*it).second;
    }
  }
};

void dag_mark_coreferences(struct dag_node *dag, coref_map &table)
// recursively set the coref table field of dag nodes to number of occurences
// in this dag - any nodes with more than one occurence are `coreferenced'
{
  if(table.node_add(dag) == -1)
    { // not yet visited
      dag_arc *arc = dag->arcs;
      while(arc != 0)
        {
          dag_mark_coreferences(arc->val, table);
          arc = arc->next;
        }
    }
}

void dag_print_rec_fed(FILE *f, struct dag_node *dag, coref_map &table) {
  // recursively print dag. Uses a hash table to keep track of coreferences
  // mark_coreferences. negative value in `visit' field means that node
  // is coreferenced, and already printed
  dag_arc *arc = dag->arcs ;
  int coref = table.node_coref(dag);

  if(coref < 0) { // dag is coreferenced, already printed
    fprintf(f, " #%d", -coref) ;
    return;
  }
  else if(coref > 0) { // dag is coreferenced, not printed yet
    fprintf(f, " #%d=", coref );
  }

  fprintf(f, " [ (%%TYPE %s #T )", type_name(dag->type));

  while(arc) {
    fprintf(f, " (%s", attrname[arc->attr]) ;
    dag_print_rec_fed(f, arc->val, table) ;
    fprintf(f, " )");
    arc=arc->next;
  }
}

void dag_print_fed(FILE *f, struct dag_node *dag) {
  coref_map table;
  dag_mark_coreferences(dag, table);
  dag_print_rec_fed(f, dag, table);
}

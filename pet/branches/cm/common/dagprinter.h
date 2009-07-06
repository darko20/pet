/* -*- Mode: C++ -*- */

#ifndef _DAGPRINTER_H
#define _DAGPRINTER_H

#include "pet-config.h"

#include "dag.h"
#include "hashing.h"

#include <ostream>

#ifdef HASH_SPACE
namespace HASH_SPACE {
  template<> struct hash<const struct ::dag_node *> {
    inline size_t operator()(const dag_node * const key) const {
      return (size_t) key;
    }
  };
}
#endif

class AbstractDagPrinter {
public:
  AbstractDagPrinter() : _dags_visited(), _coref_nr(0) {}

  virtual ~AbstractDagPrinter() {}

  virtual void
  print(std::ostream &out, const dag_node *dag, bool temp = false) = 0;

protected:
  inline int get_coref_nr(const dag_node *dag) {
    int coref = visited(dag);
    switch (coref) {
    case 1: coref = 0; break; // not coreferenced, not printed
    case 2:  // coreferenced, not printed
      coref = ++_coref_nr;
      set_visited(dag, - coref);
      break;
    }
    return coref;  // coreferenced, printed
  }

  void init_coreference_marks(const dag_node *dag, bool temp);

private:
  int visited(const dag_node *dag);
  int set_visited(const dag_node *dag, int val);
  void mark_arc_targets(const dag_arc *arcs, bool temp);
  void mark_coreferences(const dag_node *dag, bool temp);

  HASH_SPACE::hash_map<const dag_node *, int> _dags_visited;
  int _coref_nr;

};

class ReadableDagPrinter : public AbstractDagPrinter {
public:
  
  ReadableDagPrinter(bool print_pointer = false)
    : _print_pointer(print_pointer) {}
  virtual ~ReadableDagPrinter() {}

  /** \a temp determines if temporary dag information will be printed */
  virtual void
  print(std::ostream &out, const dag_node *dag, bool temp = false) {
    if (dag == NULL) { out << type_name(0); return; }
    if (dag == FAIL) { out << "fail"; return; }
    init_coreference_marks(dag, temp);
    print_dag_rec(out, dag, temp, 0);
  }

protected:

  virtual void 
  print_arcs(std::ostream &out, const dag_arc *arc, bool temp, int indent);

  virtual void 
  print_dag_rec(std::ostream &out, const dag_node *dag, bool temp, int indent);  
  
private:
  bool _print_pointer;
};


/** superclass for printing dags in machine-readable formats, like LUI,
 *  Fegramed, JXCHG
 * 
 *  \todo maybe check if template metaprogramming could increase efficiency by
 *  turning the small printing functions inline, sth. like 
 *  CompactDagPrinter<name> : AbstractDagPrinter
 */
class CompactDagPrinter : public AbstractDagPrinter {
protected:
  bool honor_temporary_dags;

  virtual void print_null_dag(std::ostream &out) { out << type_name(0); }
  virtual void print_fail_dag(std::ostream &out) { out << "fail"; }

  virtual void print_arc(std::ostream &out, const dag_arc *arc, bool temp) = 0;

  virtual void print_coref_reference(std::ostream &out, int coref_nr) = 0;

  virtual void print_coref_definition(std::ostream &out, int coref_nr) = 0;

  virtual void print_dag_node_start(std::ostream &out, const dag_node *dag) = 0;

  virtual void print_dag_node_end(std::ostream &out, const dag_node *dag) = 0;

  void print_dag_rec(std::ostream &out, const struct dag_node *dag, bool temp);

public:
  CompactDagPrinter() : honor_temporary_dags(false) {}

  virtual ~CompactDagPrinter() {}

  virtual void
  print(std::ostream &out, const dag_node *dag, bool temp = false) {
    if (dag == NULL) { print_null_dag(out); return; }
    if (dag == FAIL) { print_fail_dag(out); return; }

    temp = temp && honor_temporary_dags;
    init_coreference_marks(dag, temp);
    print_dag_rec(out, dag, temp);
  } // end print

}; // end CompactDagPrinter


class LUIDagPrinter : public CompactDagPrinter {
protected:
  virtual void print_arc(std::ostream &out, const dag_arc *arc, bool temp) {
    out << " " << attrname[arc->attr] << ": ";
    print_dag_rec(out, arc->val, temp);
  }

  virtual void print_coref_reference(std::ostream &out, int coref_nr) {
    out << "<" << coref_nr << ">";
  }
  
  virtual void print_coref_definition(std::ostream &out, int coref_nr) {
    out << "<" << coref_nr << ">=";
  }

  virtual void print_dag_node_start(std::ostream &out, const dag_node *dag) {
    if (dag->arcs != NULL) out << "#D[";
    out << type_name(dag->type);
    out << " [";
  }

  virtual void print_dag_node_end(std::ostream &out, const dag_node *dag) {
    out << " ]";
    if (dag->arcs != NULL) out << " ]";
  }
};


class ItsdbDagPrinter : public CompactDagPrinter {
protected:
  virtual void print_arc(std::ostream &out, const dag_arc *arc, bool temp);
  virtual void print_coref_reference(std::ostream &out, int coref_nr);
  virtual void print_coref_definition(std::ostream &out, int coref_nr);
  virtual void print_dag_node_start(std::ostream &out, const dag_node *dag);
  virtual void print_dag_node_end(std::ostream &out, const dag_node *dag);
};


class FegramedDagPrinter : public CompactDagPrinter {
protected:
  virtual void print_null_dag(std::ostream &out) { out << "NIL"; }

  virtual void print_arc(std::ostream &out, const dag_arc *arc, bool temp) {
    out << " (" << attrname[arc->attr];
    print_dag_rec(out, arc->val, temp);
    out << " )";
  }

  virtual void print_coref_reference(std::ostream &out, int coref_nr) {
    out << " #" << coref_nr;
  }
  
  virtual void print_coref_definition(std::ostream &out, int coref_nr) {
    out << " #" << coref_nr << "=";
  }

  virtual void print_dag_node_start(std::ostream &out, const dag_node *dag) {
    out << " [ (%%TYPE " << type_name(dag->type) << " #T )";
  }

  virtual void print_dag_node_end(std::ostream &out, const dag_node *dag) {}
};


class JxchgDagPrinter : public CompactDagPrinter {
protected:
  virtual void print_null_dag(std::ostream &out) { out << 0; }

  virtual void print_arc(std::ostream &out, const dag_arc *arc, bool temp) {
    out << " " << arc->attr;
    print_dag_rec(out, arc->val, temp);
    out << " ";
  }

  virtual void print_coref_reference(std::ostream &out, int coref_nr) {
    out << " # " << coref_nr;
  }
  
  virtual void print_coref_definition(std::ostream &out, int coref_nr) {
    out << " # " << coref_nr;
  }

  virtual void print_dag_node_start(std::ostream &out, const dag_node *dag) {
    out << " [ ";
    if (is_dynamic_type(dag->type)) {
      out << type_name(dag->type);
    } else {
      out << dag->type;
    }
  }

  virtual void print_dag_node_end(std::ostream &out, const dag_node *dag) {
    out << " ]";
  }
};

/** default printing for dags */
std::ostream &operator<<(std::ostream &out, const dag_node *dag);


#endif

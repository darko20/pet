/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/** Generic printer for arced dags */

#include "dagprinter.h"
#include <iomanip>
#include <vector>

using namespace std;

// ============================================================================
// AbstractDagPrinter implementation
// ============================================================================

inline int AbstractDagPrinter::visited(const dag_node *dag) {
  return _dags_visited[dag];
}

inline int AbstractDagPrinter::set_visited(const dag_node *dag, int val) {
  return _dags_visited[dag] = val;
}

inline void 
AbstractDagPrinter::mark_arc_targets(const dag_arc *arcs, bool temp) {
  while(arcs != 0) {
    mark_coreferences(arcs->val, temp);
    arcs = arcs->next;
  }
}

void AbstractDagPrinter::mark_coreferences(const dag_node *dag, bool temp) {
  if (temp) dag = dag_deref(dag);
  int vis_nr = visited(dag);
  if(vis_nr == 0) {
    set_visited(dag, 1);
    // not yet visited
    mark_arc_targets(dag->arcs, temp);
    if(temp) mark_arc_targets(dag_get_comp_arcs(dag), temp);
  } else {
    if (vis_nr == 1) { // met for the second time: assign a nr > 1
      set_visited(dag, 2);
    }
  }
}

void 
AbstractDagPrinter::init_coreference_marks(const dag_node *dag, bool temp) {
  _dags_visited.clear();
  _coref_nr = 0;
  mark_coreferences(dag, temp);
}

// ============================================================================
// ReadableDagPrinter implementation
// ============================================================================

void ReadableDagPrinter::
print_arcs(ostream &out, const dag_arc *arc, bool temp, int indent) {
  if(arc == 0) return;

  int maxlen = 0, i, maxatt = 0;

  vector<const dag_node*> print_attrs(nattrs, 0);
  
  while(arc) {
    i = attrnamelen[arc->attr];
    maxlen = maxlen > i ? maxlen : i;
    print_attrs[arc->attr]=arc->val;
    maxatt = arc->attr > maxatt ? arc->attr : maxatt;
    arc=arc->next;
  }

  ++maxlen; // space between label and argument
  indent += 2; // width of "[ "
  out << endl << setw(indent) << "[ ";
  bool nonfirst = false;
  int newindent = indent + maxlen;
  for(int j = 0; j <= maxatt; ++j) {
    if (print_attrs[j]) {
      if (nonfirst)
        out << "," << endl << setw(indent) << "";
      else
        nonfirst = true;
      out << attrname[j] << setw(maxlen - attrnamelen[j]) << "";
      print_dag_rec(out, print_attrs[j], temp, newindent);
    }
  }
  out << " ]";
}

/** this is incomplete, but fast and should suffice in almost all cases */
static int width(int nr) {
  if (nr < 10) return 1;
  else if (nr < 100) return 2;
  else if (nr < 1000) return 3;
  else if (nr < 10000) return 4;
  else if (nr < 100000) return 5;
  return 6;
}

void ReadableDagPrinter::
print_dag_rec(ostream &out, const dag_node *dag, bool temp, int indent) {
  // das sollte in print_dag_node_start von traditional rein
  const dag_node *orig = dag;
  if(temp) {
    orig = dag_deref(dag);
    if(orig != dag) out << "~";
  }
    
  int coref = get_coref_nr(dag);
  if(coref != 0) {
    if (coref < 0) { // dag is coreferenced, already printed
      out << "#" << - coref ;
      return;
    } else { // dag is coreferenced, not printed yet
      indent += 2 + width(coref);
      out << '#' << coref << ':' ;
    }
  }

  out << type_name(dag->type);
  if (_print_pointer) {
    out << "(" << hex;
    if(dag != orig) out << (size_t) orig << "->";
    out << (size_t) dag << ")" << dec ;
  }
  
  print_arcs(out, dag->arcs, temp, indent);
  if(temp) print_arcs(out, dag_get_comp_arcs(dag), temp, indent);
}

/** default printing for chart items: use a ReadableDagPrinter */
std::ostream &operator<<(std::ostream &out, const dag_node *dag) {
  ReadableDagPrinter rdp;
  rdp.print(out, dag);
  return out;
}

// ============================================================================
// CompactDagPrinter implementation
// ============================================================================

/** All non-human readable formats: LUI, Fegramed, JXCHG */
void CompactDagPrinter::
print_dag_rec(ostream &out, const dag_node *dag, bool temp) {
  // recursively print dag. requires `visit' field to be set up by
  // mark_coreferences. negative value in `visit' field means that node
  // is coreferenced, and already printed
  int coref = get_coref_nr(dag);
    
  if (coref != 0) {
    if(coref < 0) { // dag is coreferenced, already printed
      print_coref_reference(out, - coref) ;
      return;
    } else { // dag is coreferenced, not printed yet
      print_coref_definition(out, coref);
    }
  }

  print_dag_node_start(out, dag);
  for(dag_arc *arc = dag->arcs; arc != NULL; arc = arc->next) {
    print_arc(out, arc, temp);
  }
  print_dag_node_end(out, dag);
}




/* -*- Mode: C++ -*- */
/** \file vcg_print.h
 * Printing the type hierarchy in VCG tool syntax for flop AND cheap.
 */

#ifndef _VCG_PRINT_H
#define _VCG_PRINT_H

#include <fstream>

/** Return true if \a i is a proper type, not a leaf type. */
inline bool local_is_proper_type(type_t i) {
#ifdef FLOP
  return (leaftypeparent[i] == -1);
#else
  return ! is_leaftype(i);
#endif
}

/** Return the number of all types */
inline type_t local_ntypes() {
#ifdef FLOP
  return types.number();
#else
  return ntypes;
#endif
}

/** Return the type name of type \a i */
inline const char * local_typename(type_t i) {
#ifdef FLOP
  return types.name(i).c_str();
#else
  return type_name(i);
#endif
}

/** \brief Print the name of a type in VCG syntax (e.g. escape double quotes)
 */
inline void vcg_print_type_name(ofstream &out, type_t i) {
  const char *name = local_typename(i);
  if (name[0] == '"') out << "\"_" << name + 1 ; 
  else out << '"' << name << "\" ";
}

/** Print a hierarchy node in VCG syntax */
void vcg_print_node(ofstream &out, type_t i) {
  out << "node: { title: " ; vcg_print_type_name(out, i); 
  if (! local_is_proper_type(i)) out << " bordercolor: blue ";
  out << "}" << endl;
}

/** Print a hierarchy edge in VCG syntax */
void vcg_print_edge(ofstream &out, type_t from, type_t to) {
  out << "edge: { sourcename: " ; vcg_print_type_name(out, from) ;
  out << " targetname: " ; vcg_print_type_name(out, to) ;
  out << "}" << endl;
}

/** Print hierarchy in VCG syntax.
 *  (http://rw4.cs.uni-sb.de/users/sander/html/gsvcg1.html)
 */
void vcg_print_hierarchy(const char *filename, bool with_leaftypes) {
  ofstream out(filename);
  out << "graph: { orientation: left_to_right xspace: 10" << endl;
  
  // print all nodes
  for(type_t i = 0; i < local_ntypes(); i++)
    if (with_leaftypes || local_is_proper_type(i))
      vcg_print_node(out, i);

  // print all edges
  for(type_t i = 0; i < local_ntypes(); i++) {
    if (local_is_proper_type(i)) {
      const list< type_t > &supers = immediate_supertypes(i);
      for(list< type_t >::const_iterator it = supers.begin()
            ; it != supers.end(); it++) {
        vcg_print_edge(out, *it, i);
      }
    } else {
      if(with_leaftypes) 
        vcg_print_edge(out, leaftypeparent[i - first_leaftype], i);
    }
  }
  
  out << "}" << endl;
}

#endif

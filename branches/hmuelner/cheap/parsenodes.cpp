/* PET
* Platform for Experimentation with efficient HPSG processing Techniques
* (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
*   This file: 2010 Bernd Kiefer kiefer@dfki.de
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

#include "parsenodes.h"
#include "dag.h"
#include "settings.h"
#include "cheap.h"

#include "failure.h"
#include "dagprinter.h"
#include <iostream>

using namespace std;

extern bool dags_compatible_failures(dag_node *dag1, dag_node *dag2);

/** I'm not sure if we need the more complicated match functionality that
*  excludes the special features used for specifying label, prefix and
*  suffix. (see template-match-p)
*/
bool ParseNodes::dagsMatch(dag_node *pattern, dag_node *dag) {
  /*
  ReadableDagPrinter tcp;
  tcp.print(std::cerr, pattern);
  tcp.print(std::cerr, dag);
  */

  start_recording_failures(true);

  dag_arc *arc = dag->arcs;
  while (arc != NULL) {
    attr_t attr = arc->attr;
    if (attr != BIA_ARGS
      && (_label_path == NULL || attr != _label_path->val)
      && (_prefix_path == NULL || attr != _prefix_path->val)
      && (_suffix_path == NULL || attr != _suffix_path->val)) {
        dag_node *pattern_sub = dag_get_attr_value(pattern, attr);
        if (pattern_sub != FAIL && ! dags_compatible(pattern_sub, arc->val)) {
          const list<failure *> & failures = get_failures();
          for(list<failure *>::const_iterator it = failures.begin();
            it != failures.end();
            ++it) {
              (*it)->print(std::cerr);
          }
          return false;
        }
    }
    arc = arc->next;
  }

  return true;

}

/** get the main label for this node, something like XP */
string ParseNodes::matchLabel(dag_node *dag) {
  for(list< label_template >::iterator it = _labels.begin();
    it != _labels.end(); ++it) {
      if (dagsMatch(it->pattern, dag)) {
        return it->label;
      }
  }
  return "?";
}

/** possibly add markers for subcat/slash to the main category */
string ParseNodes::metaLabel(dag_node *dag) {
  // this tests for an empty diff list somewhere on the recursive_path, in
  // contrast to dag_get_path_value
  // An empty diff list is a cospec of list and last, but can still contain
  // a subdag, thus, an incorrect positiv match could occur
  dag_node *sub = dag_get_path_value_check_dlist(dag, _recursive_path);
  if (sub == FAIL) return "";
  for(list< meta_template >::iterator meta_it = _metas.begin();
    meta_it != _metas.end(); ++meta_it) {
      if (dagsMatch(meta_it->pattern, dag)) {
        // metaSubLabel function, included here
        for(list< label_template >::iterator it = _labels.begin();
          it != _labels.end(); ++it) {
            dag_node *patternDag = dag_get_path_value(it->pattern, _local_path);
            if (dagsMatch(patternDag, dag)) {
              return meta_it->prefix + it->label + meta_it->suffix;
            }
        }
        return meta_it->prefix + "?" + meta_it->suffix;
      }
  }
  return "";
}

void ParseNodes::initialize() {
  //label_path = get_opt<list_int *>("label-path");
  // \todo: we'll get them from settings for the moment, but settings should
  // go as soon as possible and be replaced by Configs
  const char* v = cheap_settings->value("pn-label-path");
  _label_path = path_to_lpath(v ? v : "");
  v = cheap_settings->value("pn-prefix-path");
  _prefix_path = path_to_lpath(v ? v : "");
  v = cheap_settings->value("pn-suffix-path");
  _suffix_path = path_to_lpath(v ? v : "");
  v = cheap_settings->value("pn-recursive-path");
  _recursive_path = path_to_lpath(v ? v : "");
  v = cheap_settings->value("pn-local-path");
  _local_path = path_to_lpath(v ? v : "");
  v = cheap_settings->value("pn-label-fs-path");
  _label_fs_path = path_to_lpath(v ? v : "");

  if (cheap_settings->value("pn-label-type") == NULL
    || cheap_settings->value("pn-meta-type") == NULL) {
      _label_type = _meta_type = T_BOTTOM;
      return;
  }

  v = cheap_settings->value("pn-label-type");
  _label_type = lookup_type(v ? v : "");
  v = cheap_settings->value("pn-meta-type");
  _meta_type = lookup_type(v ? v : "");

  for(int type = 0; type < nstatictypes; ++type) {
    if (type != _label_type && subtype(type, _label_type)) {
      // a label template
      dag_node *templ = type_dag(type);
      dag_node *name_dag = dag_get_path_value(templ, _label_path);
      // lkb checks if the type is a subtype of string
      if (name_dag != FAIL) {
        _labels.push_back(label_template(print_name(name_dag->type), templ));
      }
    }
    else {
      if (type != _meta_type && subtype(type, _meta_type)) {
        // a meta template
        dag_node *templ = type_dag(type);
        dag_node *prefix_dag = dag_get_path_value(templ, _prefix_path);
        // lkb checks if the type is a subtype of string
        dag_node *suffix_dag = dag_get_path_value(templ, _suffix_path);
        // lkb checks if the type is a subtype of string
        if (prefix_dag != FAIL && suffix_dag != FAIL) {
          _metas.push_back(meta_template(print_name(prefix_dag->type),
            print_name(suffix_dag->type),
            templ));
        }
      }
    }
  }
}

/** match the given dag node against the templates to compute a short label */
string ParseNodes::getLabel(dag_node *dag) {
  dag_node *sub = dag_get_path_value(dag, _label_fs_path);
  if (sub == FAIL)
    return "UNK";

  return matchLabel(sub) + metaLabel(sub);
}

/** needs (at least) the settings options:

;;; the path where the name string is stored
(defparameter *label-path* '(LNAME))

;;; the path for the meta prefix symbol
(defparameter *prefix-path* '(META-PREFIX))

;;; the path for the meta suffix symbol
(defparameter *suffix-path* '(META-SUFFIX))

;;; the path for the recursive category
(defparameter *recursive-path* '(SYNSEM NONLOC SLASH LIST FIRST))

;;; the path inside the node to be unified with the recursive node
(defparameter *local-path* '(SYNSEM LOCAL))

;;; the path inside the node to be unified with the label node
(defparameter *label-fs-path* '())

(defparameter *label-template-type* 'label)

and meta-template-type 'meta ??

* lots of this reworked after lkb/src/io-general/tree-nodes.lsp
* the top-level function is calculate-tdl-label
*/

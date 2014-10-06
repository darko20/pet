/* -*- mode: c++ -*- */
/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
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

/* A default parsenodes.set for the LinGO ERG
;;; the path where the name string is stored
pn-label-path := "LNAME".

;;; the path for the meta prefix symbol
pn-prefix-path := "META-PREFIX".

;;; the path for the meta suffix symbol
pn-suffix-path := "META-SUFFIX".

;;; the path for the recursive category
pn-recursive-path := "SYNSEM.NONLOC.SLASH.LIST.FIRST".

;;; the path inside the node to be unified with the recursive node
pn-local-path := "SYNSEM.LOCAL".

;;; the path inside the node to be unified with the label node
pn-label-fs-path := "".

;;; the common supertype of all label resp. meta parsenode templates
pn-label-type := label.
pn-meta-type := meta.

*/

/** \file parsenodes.h
 * interface to code that reduces feature structures in a parse tree to simple
 * labels
 */

#ifndef _PARSENODES_H_
#define _PARSENODES_H_

#include <string>
#include <list>


/** This class is meant to be used as a singleton instance in tGrammar to
 *  provide the functionality needed to compute simple parse tree nodes from
 *  the dags resulting from a parse.
 */
class ParseNodes {
public:

  /** Call this method after the grammar has been read. go through all (leaf?)
   *  types and check if the supertype is either 'label' or 'meta'. Extract the
   *  relevant information into either a label or meta template.
   */
  void initialize();

  /** match the given dag node against the templates to compute a short label */
  std::string getLabel(struct dag_node *dag);

private:

  /** the path where the name string of a template is stored */
  struct list_int *_label_path;

  /** the paths for the meta prefix and suffix */
  struct list_int *_prefix_path, *_suffix_path;

  /** the path for the recursive category for slash symbols */
  struct list_int *_recursive_path;

  /** the path to skip in templates when computing recursive categories */
  struct list_int *_local_path;

  /* the path inside the node to be unified with the label node */
  struct list_int *_label_fs_path;

  /* the types of label and meta label type specifications */
  int _label_type, _meta_type;


  struct label_template {
    label_template(const char *name, struct dag_node *pat)
      : label(name), pattern(pat) {}

    std::string label;
    struct dag_node *pattern;
  };

  struct meta_template {
    meta_template(const char *pref, const char *suff, struct dag_node *pat)
      : prefix(pref), suffix(suff), pattern(pat) {}

    std::string prefix;
    std::string suffix;
    struct dag_node *pattern;
  };

  std::list< label_template > _labels;

  std::list< meta_template > _metas;

  /** I'm not sure if we need the more complicated match functionality that
   *  excludes the special features used for specifying label, prefix and
   *  suffix. (see template-match-p)
   */
  bool dagsMatch(struct dag_node *pattern, struct dag_node *dag);

  std::string matchLabel(struct dag_node *dag);

  std::string metaLabel(struct dag_node *dag);

};

#endif

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

/* class to represent mrs */

#ifndef _MRS_H_
#define _MRS_H_

#include "cheap.h"
#include "fs.h"
#include "../common/utility.h"

//
// classes to deal with MRS structures
//

class mrs;
class mrs_rel;

class mrs_rel
{
 public:
  mrs_rel(mrs *, fs f);
  mrs_rel(mrs *, int type, int pred = 0);

  inline mrs_rel() : _fs(), _mrs(0), _id(0), _labels(), _senses() { }

  inline mrs_rel(const mrs_rel &f) { 
    _fs = f._fs; _mrs = f._mrs; _id = f._id; _rel = f._rel; _pred = f._pred;
    _labels = f._labels; _senses = f._senses; _cvalue = f._cvalue;
  }

  inline mrs_rel &operator=(const mrs_rel &f) { 
    _fs = f._fs; _mrs = f._mrs; _id = f._id; _rel = f._rel; _pred = f._pred;
    _labels = f._labels; _senses = f._senses;  _cvalue = f._cvalue;
    return *this; 
  }

  int value(char *attr);

  bool valid() { return _mrs != 0; }

  inline int id() { return _id; }

  inline int type() { return _rel; }
  inline char *printname() { return printnames[_rel]; }
  inline char *name() { return typenames[_rel]; }
  void name(char *s) { _rel = lookup_type(s); }
  inline char *pred() { return printnames[_pred]; }

  inline fs &get_fs() { return _fs; }
  inline int cvalue() {return _cvalue; }

  list<int> &labels() { return _labels; }
  list<string> &senses() { return _senses; }

  list<int> id_list(char *path);
  list<int> id_list_by_roles(char **paths);

  bool number_convert(void);

  void print(FILE *f);

 private:
  fs _fs;
  mrs *_mrs;
  int _id;
  int _rel;
  int _pred;
  list<int> _labels;
  list<string> _senses;
  int _cvalue;
};

class mrs_hcons
{
 public:
  mrs_hcons(mrs *m, fs f);
  inline mrs_hcons() {}

  int lookup(int);

  void print(FILE *f);

 private:
  map<int, int> _dict;
};

const int dummy_id = 1;

class mrs
{
 public:
  mrs(fs root);

  ~mrs();

  int id(fs f);

  void push_rel(mrs_rel const &r);

  mrs_rel rel(int id);

  list<int> rels(char *attr, int val, int subtypeof = 0);
  mrs_rel rel(char *attr, int val, int subtypeof = 0);

  void use_rel(int id, int clause = 0);
  bool used(int id, int clause = 0);

  inline int top() { return _top; }
  inline int index() { return _index; }

  inline int hcons(int lhs) { return _hcons.lookup(lhs); }
  
  void countpseudorel();

  void print(FILE *f);

 private:

  void sanitize(void);
  bool number_convert(void);

  fs _raw;
  int _top;
  int _index;
  list<mrs_rel> _rels;
  mrs_hcons _hcons;
  map<dag_node *, int> _id_dict;
  int _next_id;
  int _pseudorels;

  map<int, list_int *> _used_rels;
  
};

extern char *k2y_type_name(char *);
extern char *k2y_pred_name(char *);
extern char *k2y_role_name(char *);

//
// stamp id information into fs
//

void mrs_stamp_fs(fs &f, int id);

//
// inform k2y module of mapping so it can get back to the external ids from
// the ids in the relations
//

void mrs_map_id(int item_id, int ls_id);
void mrs_map_sense(int item_id, string sense);

void mrs_reset_mappings();

#endif


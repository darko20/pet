/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
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

  inline mrs_rel() : _fs(), _mrs(0), _id(0), _label() { }

  inline mrs_rel(const mrs_rel &f) { 
    _fs = f._fs; _mrs = f._mrs; _id = f._id; _rel = f._rel; _pred = f._pred;
    _label = f._label; _cvalue = f._cvalue;
  }

  inline mrs_rel &operator=(const mrs_rel &f) { 
    _fs = f._fs; _mrs = f._mrs; _id = f._id; _rel = f._rel; _pred = f._pred;
    _label = f._label; _cvalue = f._cvalue;
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

  list<int> &label() { return _label; };
  inline int span() { return _label.size(); }

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
  list<int> _label;
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

// keep LEDA happy

extern char *k2y_type_name(char *);
extern char *k2y_pred_name(char *);
extern char *k2y_role_name(char *);

inline istream &operator>>(istream &I, mrs_rel &) { return I; }
inline ostream &operator<<(ostream &O, const mrs_rel &) { return O; }

#endif


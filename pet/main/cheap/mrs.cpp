/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* classes to represent MRS */

#include "pet-system.h"
#include "mrs.h"
#include "types.h"
#include "tsdb++.h"

//
// class mrs
//

// construct an MRS from a given feature structure
// we expect the MRS under the path specified in mrs-path
mrs::mrs(fs root)
  : _next_id(dummy_id + 1), _used_rels()
{
  root.expand();

  _raw = root.get_path_value(cheap_settings->value("mrs-path"));
  // walk through LISZT and construct list of mrs_rel

  fs foo = _raw.get_path_value(k2y_role_name("k2y_liszt"));
  fs liszt = foo.get_attr_value(BIA_LIST);
  fs last = foo.get_attr_value(BIA_LAST);
  while(liszt.valid() && liszt != last )
    {
      fs rel = liszt.get_attr_value(BIA_FIRST);
      if(!rel.valid())
	throw error("invalid MRS LISZT");

      push_rel(mrs_rel(this, rel));

      liszt = liszt.get_attr_value(BIA_REST);
    }

  fs top = _raw.get_attr_value(k2y_role_name("k2y_top"));
  _top = id(top);
  fs index = _raw.get_attr_value(k2y_role_name("k2y_index"));
  _index = id(index);
  _hcons = mrs_hcons(this, _raw);

  sanitize();
  number_convert();

}

mrs::~mrs()
{
  for(map<int, list_int*>::iterator iter = _used_rels.begin();
      iter != _used_rels.end(); ++iter)
    {
      if(iter->second != 0)
	free_list(iter->second);
    }
}


void mrs::push_rel(mrs_rel const &r)
{
  _rels.push_front(r);
}

int mrs::id(fs f)
{
  if(!f.valid())
    return 0;

  if(_id_dict.find(f.dag()) == _id_dict.end())
    _id_dict[f.dag()] = _next_id++;

  return _id_dict[f.dag()];
}

mrs_rel mrs::rel(int id)
{
  if(!id) return mrs_rel();

  for(list<mrs_rel>::iterator iter = _rels.begin();
      iter != _rels.end(); ++iter)
    if(iter->id() == id)
      return *iter;
  
  return mrs_rel();
}

// return list of all rels with an `attr' of value `val'
list<int> mrs::rels(char *attr, int val, int subtypeof)
{
  list<int> l;

  if(val == 0) return l;

  for(list<mrs_rel>::iterator iter = _rels.begin();
      iter != _rels.end(); ++iter)
    {
      if(iter->value(attr) == val &&
	 (subtypeof == 0 || subtype(iter->type(), subtypeof)))
	{
	  l.push_front(iter->id());
	}
    }
  
  return l;
}

mrs_rel mrs::rel(char *attr, int val, int subtypeof)
{
  list<int> l = rels(attr, val, subtypeof);

  if(l.empty())
    return mrs_rel();
  else if(l.size() > 1)
    fprintf(ferr, "mrs::rel(): at most one rel that is subtype of %s and has a %s value %d expected (found %d)\n",
	    printnames[subtypeof], attr, val, l.size());

  return rel(l.front());
}

void mrs::use_rel(int id, int clause)
{
  _used_rels[clause] = cons(id, _used_rels[clause]);
}

bool mrs::used(int id, int clause)
{
  return contains(_used_rels[clause], id);
}


void mrs::sanitize(void) {

  for(list<mrs_rel>::iterator r = _rels.begin();
      r != _rels.end();
      r++) {
    //
    // 
    // If temporal locative PP ('in, on, at') or generic loc_rel 
    // with a temp_rel internal argument, change rel name to say so
    // 
    int intargid = r->value(k2y_role_name("k2y_arg3"));
    mrs_rel intargrel = rel(k2y_role_name("k2y_inst"), intargid);
    if(intargrel.valid() 
       && subtype(intargrel.type(), lookup_type(k2y_pred_name("k2y_temp_rel")))
       && (subtype(r->type(), lookup_type(k2y_pred_name("k2y_temploc_rel")))
           || subtype(r->type(), lookup_type(k2y_pred_name("k2y_loc_rel")))))
      r->name("temp_loc_rel");
    else if(intargrel.valid() 
            && subtype(r->type(), lookup_type(k2y_pred_name("k2y_loc_rel"))))
      r->name("unspec_loc_rel");

  } /* for */

} // mrs::sanitize()


bool mrs::number_convert(void) {

  for(list<mrs_rel>::iterator r = _rels.begin();
      r != _rels.end();
      r++) {
    r->number_convert();
  } /* for */

  return true;

} // mrs::number_convert()


void mrs::print(FILE *f)
{
  fprintf(f, "raw mrs:\n");
  _raw.print(f);
  fprintf(f, "\n");

  fprintf(f, "TOP: x%d\n", _top);
  fprintf(f, "INDEX: x%d\n", _index);
  fprintf(f, "LISZT:\n");
  
  for(list<mrs_rel>::iterator iter = _rels.begin();
      iter != _rels.end(); ++iter)
    {
      iter->print(f);
      fprintf(f, "\n");
    }
  
  fprintf(f, "H-CONS: ");
  _hcons.print(f);
  fprintf(f, "\n");
}

//
// class mrs_rel
//

// construct an MRS REL from a feature structure
mrs_rel::mrs_rel(mrs *m, fs f)
  : _fs(f), _mrs(m), _rel(f.type()), _label(0), _cvalue(-1)
{

  if(cheap_settings->member("k2y-disfavoured-relations", typenames[_rel]))
    {
      throw error(string("disfavoured relation `") + printnames[_rel] +
                  string("' in MRS"));
    }

  char *path = cheap_settings->value("label-path-tail");
  if(path == NULL) path = "LABEL";

  try {
    fs pred = _fs.get_attr_value(k2y_role_name("k2y_pred"));
    if(pred.valid()) _pred = pred.type();
    else _pred = _rel;
  } // try
  catch (error &condition) {
    _pred = _rel;
  } // catch


  fs label = _fs.get_attr_value(path);
  if(label.valid() && label.type() == BI_CONS)
    {
      for(; 
          label.valid() && label.type() == BI_CONS; 
          label = label.get_attr_value(BIA_REST))
	{
	  fs node = label.get_attr_value(BIA_FIRST);
	  if(node.valid())
	    {
	      int type = node.type();
	      if(type >= 0) _label.push_back(type + 1);
	    }
	}
    }
  _id = _mrs->id(f);
}

// construct an MRS REL from whole cloth
mrs_rel::mrs_rel(mrs *m, int type, int pred)
  : _fs(copy(fs(type))), _mrs(m), _rel(type), _label(0), _cvalue(-1)
{

  _pred = (pred ? pred : type);
  _id = _mrs->id(_fs);

}

int mrs_rel::value(char *attr)
{
  if(!_fs.valid())
  {
    fprintf(ferr, "no feature structure\n");
    return 0;
  }
  fs f = _fs.get_attr_value(attr);
  return _mrs->id(f);
}

list<int> mrs_rel::id_list_by_paths(char **paths)
{
  list<int> ids;

  for(int i = 0; paths[i] != 0; i++)
    {
      fs foo = _fs.get_path_value(paths[i]);
      if(foo.valid()) ids.push_front(_mrs->id(foo));
    }
 
 return ids;
}

list<int> mrs_rel::id_list(char *path)
{
  list<int> ids;

  fs diff = _fs.get_path_value(path);
  if(!diff.valid())
  {
    fprintf(ferr, "mrs_rel::id_list(): no list under %s\n", path);
    return ids;
  }

  fs list = diff.get_attr_value(BIA_LIST);
  fs last = diff.get_attr_value(BIA_LAST);

  while(list.valid() && list != last )
    {
      fs rel = list.get_attr_value(BIA_FIRST);

      if(!rel.valid())
	   throw error("invalid difflist in MRS");

      ids.push_front(_mrs->id(rel));

      list = list.get_attr_value(BIA_REST);
    }

  return ids;
}

bool mrs_rel::number_convert(void) {

  if(_cvalue >= 0) return true;

  if(!strcmp(name(), k2y_pred_name("k2y_quantity_rel"))) {
    int foo = value(k2y_role_name("k2y_amount"));
    if(foo) {
      mrs_rel amount 
        = _mrs->rel(k2y_role_name("k2y_hndl"), foo, 
                    lookup_type(k2y_pred_name("k2y_integer_rel")));
    
      if(amount.number_convert()) {
        _cvalue = amount.cvalue();
        return true;
      } // if
    } // if
    return false;
  } // if

  if(!strcmp(name(), k2y_pred_name("k2y_card_rel"))) {
    fs fs = get_fs().get_path_value(k2y_role_name("k2y_const_value"));
    if(!fs.valid()) {
      //
      // _fix_me_
      //
      return false;
    } /* if */
    try {
      _cvalue = strtoint(fs.name(), "mrs::number_convert()", true);
      return true;
    } /* try */
    catch (error &condition) {
      _cvalue = -1;
      return false;
    } /* catch */
  } /* if */

  int term1 = 0;
  int term2 = 0;
  if(!strcmp(name(), k2y_pred_name("K2y_plus_rel"))) {
    term1 = value(k2y_role_name("k2y_term1"));
    term2 = value(k2y_role_name("k2y_term2"));
  } /* if */
  else if(!strcmp(name(), k2y_pred_name("k2y_times_rel"))) {
    term1 = value(k2y_role_name("k2y_factor1"));
    term2 = value(k2y_role_name("k2y_factor2"));
  } /* if */
  if(!term1 || !term2) return false;

  mrs_rel operand1 
    = _mrs->rel(k2y_role_name("k2y_hndl"), term1, 
                lookup_type(k2y_pred_name("k2y_integer_rel")));
  mrs_rel operand2 
    = _mrs->rel(k2y_role_name("k2y_hndl"), term2, 
                lookup_type(k2y_pred_name("k2y_integer_rel")));
  if(!operand1.valid() || !operand1.number_convert() 
     || !operand2.valid() || !operand2.number_convert()) {
    //
    // _fix_me_
    //
    return false;
  } /* if */

  int value1 = operand1.cvalue();
  int value2 = operand2.cvalue();
  _mrs->use_rel(operand1.id());
  _mrs->use_rel(operand2.id());

  if(!strcmp(name(), k2y_pred_name("k2y_plus_rel"))) _cvalue = value1 + value2;
  else _cvalue = value1 * value2;

  //
  // collect raw atoms from both operands into operator
  //
  copy(operand1.label().begin(), operand1.label().end(), 
       back_inserter(_label));
  copy(operand2.label().begin(), operand2.label().end(), 
       back_inserter(_label));
  _label.sort();
  _label.unique();

  return true;

} // mrs_rel::number_convert()

void mrs_rel::print(FILE *f)
{
  fprintf(f, "%s (%s)\n  ID x%d\n", printnames[_rel], printnames[_pred], _id);
  fs handel =_fs.get_attr_value(k2y_role_name("k2y_hndl"));
  if(handel.valid()) fprintf(f, "  HANDEL h%d\n", _mrs->id(handel));
  if(_cvalue >= 0) 
    fprintf(f, 
            "  CVALUE %d [%d token%s]\n", 
            _cvalue, span(), (span() > 1 ? "s" : ""));
  if(!_label.empty()) {
    fprintf(f, "  RA < ");
    for(list<int>::iterator i = _label.begin(); 
        i != _label.end();
        i++) {
      fprintf(f, "%d ", *i);
    } /* for */
    fprintf(f, ">\n");
  } /* if */
}

//
// class mrs_hcons
//

mrs_hcons::mrs_hcons(mrs *m, fs f)
{
  // walk through HCONS and construct list of mrs_rel
  
  fs hcons = f.get_path_value(k2y_role_name("k2y_hcons"));
  if(hcons.valid()) {
    fs liszt = hcons.get_attr_value(BIA_LIST);
    fs last = hcons.get_attr_value(BIA_LAST);
    while(liszt.valid() && liszt != last )
      {
        fs pair = liszt.get_attr_value(BIA_FIRST);
        if(!pair.valid())
          throw error("invalid H-CONS value in MRS");

        fs sc_arg = pair.get_attr_value(k2y_role_name("k2y_sc_arg"));
        fs outscpd = pair.get_attr_value(k2y_role_name("k2y_outscpd"));

        int lhs = m->id(sc_arg);
        int rhs = m->id(outscpd);
        if(rhs == m->top()) {
          char foo[128];
          sprintf(foo, "MRS top handle (h%d) outscoped by h%d", rhs, lhs);
          throw error(foo);
        } /* if */
        _dict[lhs] =  rhs;

        liszt = liszt.get_attr_value(BIA_REST);
      }
  } // if
}

int mrs_hcons::lookup(int lhs)
{
  if(_dict.find(lhs) == _dict.end())
    return lhs;
  else
    return _dict[lhs];
}

void mrs_hcons::print(FILE *f)
{
  fprintf(f, "{");
  for(map<int,int>::iterator iter = _dict.begin(); iter != _dict.end(); ++iter)
    {
            fprintf(f, "x%d->x%d ",iter->first, iter->second);
    }
  fprintf(f, "}");
}


char *k2y_type_name(char *abstr_name)
{
  char *name = cheap_settings->assoc("k2y_type_names", abstr_name);
  if(name == 0) name = cheap_settings->assoc("k2y-type-names", abstr_name);
  if(name == 0)
    throw error("undefined k2y_type_name `" + string(abstr_name) + "'.");
  return name;

}

char *k2y_pred_name(char *abstr_name)
{
  char *name = cheap_settings->assoc("k2y_pred_names", abstr_name);
  if(name == 0) name = cheap_settings->assoc("k2y-pred-names", abstr_name);
  if(name == 0)
    throw error("undefined k2y_pred_name `" + string(abstr_name) + "'.");
  return name;

}

char *k2y_role_name(char *abstr_name)
{
  char *name = cheap_settings->assoc("k2y_role_names", abstr_name);
  if(name == 0) name = cheap_settings->assoc("k2y-role-names", abstr_name);
  if(name == 0)
    throw error("undefined k2y_role_name `" + string(abstr_name) + "'.");
  return name;
}

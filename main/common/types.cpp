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

/* operations on types  */

#include "pet-system.h"
#include "types.h"
#include "bitcode.h"
#include "dag.h"
#include "dumper.h"

#ifdef FLOP
#include "flop.h"
#else
#include "cheap.h"
#undef SUBTYPECACHE
#endif

int nleaftypes;
int *leaftypeparent = 0;

static vector<bitcode *> typecode;
static hash_map<bitcode, int> codetable;

static bitcode *temp_bitcode = NULL;
static int codesize;

int *apptype = 0;

// status
int nstatus;
char **statusnames = 0;

// types
int ntypes;
int first_leaftype;
char **typenames = 0;
char **printnames = 0;
int *typestatus = 0;

int BI_TOP, BI_SYMBOL, BI_STRING, BI_CONS, BI_LIST, BI_NIL, BI_DIFF_LIST;

// attributes
char **attrname = 0;
int nattrs;
int *attrnamelen = 0;
int BIA_FIRST, BIA_REST, BIA_LIST, BIA_LAST, BIA_ARGS;

void initialize_codes(int n)
{
  codesize = n;
  temp_bitcode = New bitcode(codesize);
  codetable[bitcode(codesize)] = -1;
  typecode.resize(n);
}

void resize_codes(int n)
{
  typecode.resize(n);
}

void register_codetype(const bitcode &b, int i)
{
  codetable[b] = i;
}

void register_typecode(int i, bitcode *b)
{
  typecode[i] = b;
}

#ifdef HASH_MAP_AVAIL
namespace HASH_SPACE {
template<> struct hash<string>
{
  inline size_t operator()(const string &key) const
  {
    int v = 0;
    for(unsigned int i = 0; i < key.length(); i++)
      v += key[i];

    return v;
  }
};
}
#endif

hash_map<string, int> _typename_memo;

int lookup_type(const char *s)
{
  static bool initialized_cache = false;

  if(!initialized_cache)
    {
      for(int i = 0; i < ntypes; i++)
	_typename_memo[typenames[i]] = i;
      initialized_cache = true;
    }
  
  hash_map<string, int>::iterator pos = _typename_memo.find(s);
  if(pos != _typename_memo.end())
    {
      return (*pos).second;
    }
  return -1;
}

// dyntypename contains all dynamically needed type names for a parse.
map<string, int> dyntypememo ;
vector<const char *> dyntypename;
vector<dag_node *> dyntypedag;
int last_dynamic;

int lookup_symbol(const char *s)
{
  int type = lookup_type(s);

  if (type == -1) {
    // is it registered as dynamic type?
    map<string, int>::iterator pos = dyntypememo.find(s);
    if(pos != dyntypememo.end())
      return (*pos).second;

    // None of the above: register it as new dynamic type
    dyntypememo[s] = last_dynamic ;
    dyntypename.push_back(s);
    // Create a dag for this dynamic symbol only containing its type
    dag_node *dyntype = new_dag(last_dynamic) ;
    dyntypedag.push_back(dyntype) ;
    last_dynamic++ ;
    return last_dynamic - 1 ;
  }
  else return type;
}

void clear_dynamic_symbols() {
  for (int i = ntypes; i < last_dynamic; i++) {
    delete dyntypedag[i] ; 
    dyntypedag.pop_back();
    dyntypename.pop_back();
  }
  last_dynamic = ntypes ;
  dyntypememo.erase(dyntypememo.begin(), dyntypememo.end()) ;
}

map<string, int> _attrname_memo; 

int lookup_attr(const char *s)
{
  map<string, int>::iterator pos = _attrname_memo.find(s);
  if(pos != _attrname_memo.end())
    return (*pos).second;

  for(int i = 0; i < nattrs; i++)
    {
      if(strcmp(attrname[i], s) == 0)
	{
	  _attrname_memo[s] = i;
	  return i;
	}
    }

  _attrname_memo[s] = -1;
  return -1;
}

int lookup_status(const char *s)
{
  for(int i = 0; i < nstatus; i++)
    {
      if(strcmp(statusnames[i], s) == 0)
	{
	  return i;
	}
    }
  return -1;
}

int lookup_code(const bitcode &b)
{
  hash_map<bitcode, int>::const_iterator pos = codetable.find(b);

  if(pos == codetable.end())
    return -1;
  else 
    return (*pos).second;
}

int get_special_name(settings *sett, char *suff, bool attr = false)
{
  char *buff = New char[strlen(suff) + 25];
  sprintf(buff, attr ? "special-name-attr-%s" : "special-name-%s", suff);
  char *v = sett->req_value(buff);
  delete[] buff;
  int id;
#ifdef FLOP
  if(attr)
    {
      id = attributes.id(v);
      if(id == -1)
	id = attributes.add(v);
    }
  else
    {
      id = types.id(v);
      if(id == -1)
	{
	  struct type *t = new_type(v, false);
	  t->def = sett->lloc();
	  t->implicit = true;
	  id = types.id(v);
	}
      
    }
#else 
 id = attr ? lookup_attr(v) : lookup_type(v);
  if(id == -1)
    throw error(string(buff) + " not defined (but referenced in settings file)");
#endif
  return id;
}

void initialize_specials(settings *sett)
{
  BI_TOP       = get_special_name(sett, "top");
  BI_SYMBOL    = get_special_name(sett, "symbol");
  BI_STRING    = get_special_name(sett, "string");
  BI_CONS      = get_special_name(sett, "cons");
  BI_LIST      = get_special_name(sett, "list");
  BI_NIL       = get_special_name(sett, "nil");
  BI_DIFF_LIST = get_special_name(sett, "difflist");

  BIA_FIRST = get_special_name(sett, "first", true);
  BIA_REST  = get_special_name(sett, "rest", true);
  BIA_LIST  = get_special_name(sett, "list", true);
  BIA_LAST  = get_special_name(sett, "last", true);
  BIA_ARGS  = get_special_name(sett, "args", true);
}

#ifndef FLOP

void undump_symbol_tables(dumper *f)
{
  // nstatus
  nstatus = f->undump_int();

  // npropertypes
  first_leaftype = f->undump_int();

  // nleaftypes
  nleaftypes = f->undump_int();
  ntypes = first_leaftype + nleaftypes;
  last_dynamic = ntypes;

  // nattrs
  nattrs = f->undump_int();

  statusnames = (char **) malloc(sizeof(char *) * nstatus);

  for(int i = 0; i < nstatus; i++)
    statusnames[i] = f->undump_string();

  typenames = (char **) malloc(sizeof(char *) * ntypes);
  typestatus = (int *) malloc(sizeof(int) * ntypes);

  for(int i = 0; i < ntypes; i++)
    {
      typenames[i] = f->undump_string();
      typestatus[i] = f->undump_int();
    }

  attrname = (char **) malloc(sizeof(char *) * nattrs);
  attrnamelen = (int *) malloc(sizeof(int) * nattrs);

  for(int i = 0; i < nattrs; i++)
    {
      attrname[i] = f->undump_string();
      attrnamelen[i] = strlen(attrname[i]);
    }

  initialize_specials(cheap_settings);
}

void undump_printnames(dumper *f)
{
  if(f == 0) // we have no printnames
    {
      printnames = typenames;
      return;
    }

  printnames = (char **) malloc(sizeof(char *) * ntypes);

  for(int i = 0; i < ntypes; i++)
    {
      printnames[i] = f->undump_string();
      if(printnames[i] == 0)
	printnames[i] = typenames[i];
    }
}

void free_type_tables()
{
  if(statusnames != 0)
  {
    free(statusnames);
    statusnames = 0;
  }
  if(typenames != 0)
  {
    free(typenames);
    typenames = 0;
  }
  if(typestatus != 0)
  {
    free(typestatus);
    typestatus = 0;
  }
  if(attrname != 0)
  {
    free(attrname);
    attrname = 0;
  }
  if(attrnamelen != 0)
  {
    free(attrnamelen);
    attrnamelen = 0;
  }
  if(printnames != 0)
  {
    free(printnames);
    printnames = 0;
  }

  delete temp_bitcode;
  delete[] leaftypeparent;
  delete[] apptype;
  delete[] featset;
  for(int i = 0; i < nfeatsets; i++)
    delete[] featsetdesc[i].attr;
  delete[] featsetdesc;
}

#endif

#ifdef FLOP

void dump_hierarchy(dumper *f)
{
  int i;

  // bitcodesize in bits
  f->dump_int(codesize);

  // bitcodes for all proper types
  for(i = 0; i < first_leaftype; i++)
    if(leaftypeparent[rleaftype_order[i]] == -1)
      {
        typecode[rleaftype_order[i]]->dump(f);
      }
    else
      fprintf(ferr, "leaf type conception error a: `%s' (%d -> %d)\n", 
              typenames[rleaftype_order[i]], i, rleaftype_order[i]);

  // parents for all leaf types
  for(i = first_leaftype; i < ntypes; i++)
    if(leaftypeparent[rleaftype_order[i]] != -1)
      {
	f->dump_int(rleaftype_order[leaftypeparent[leaftype_order[i]]]);
      }
    else
      fprintf(ferr, "leaf type conception error b: `%s' (%d -> %d)\n", 
              typenames[rleaftype_order[i]], i, rleaftype_order[i]);
}

#endif

void undump_hierarchy(dumper *f)
{
  codesize = f->undump_int();
  initialize_codes(codesize);
  resize_codes(first_leaftype);
  
  for(int i = 0; i < first_leaftype; i++)
    {
      temp_bitcode->undump(f);
      register_codetype(*temp_bitcode, i);
      register_typecode(i, temp_bitcode);
      temp_bitcode = New bitcode(codesize);
    }

  leaftypeparent = New int[nleaftypes];
  for(int i = 0; i < nleaftypes; i++)
    leaftypeparent[i] = f->undump_int();
}

void undump_tables(dumper *f)
{
  // read table mapping types to feature set

  int coding = f->undump_int();

  if(coding == 0)
    dag_nocasts = true;
  else if(coding == 1)
    dag_nocasts = false;
  else
    throw error("unknown encoding");

  featset = New int[first_leaftype];

  for(int i = 0; i < first_leaftype; i++)
    {
      featset[i] = f->undump_int();
    }

  // read feature sets

  nfeatsets = f->undump_int();
  featsetdesc = New featsetdescriptor[nfeatsets];
  
  for(int i = 0; i < nfeatsets; i++)
    {
      short int na = featsetdesc[i].n = f->undump_short();
      featsetdesc[i].attr = na > 0 ? New short int[na] : 0;

      for(int j = 0; j < na; j++)
	featsetdesc[i].attr[j] = f->undump_short();
    }

  // read appropriate sorts table

  apptype = New int[nattrs];
  for(int i = 0; i < nattrs; i++)
    apptype[i] = f->undump_int();
}

int core_glb(int a, int b)
{
  if(intersect_empty(*typecode[a], *typecode[b], temp_bitcode))
    return -1;
  else
    return lookup_code(*temp_bitcode);
}

bool core_subtype(int a, int b)
{
  if (a == b)
    return true;

  // could use subset_fast here, once it's debugged
  return typecode[a]->subset(*typecode[b]);
}

inline bool is_leaftype(int s)
{
  return s >= first_leaftype && s < ntypes;
}

#ifndef FLOP
#include <typecache.h>
typecache glbcache(0);

#ifdef SUBTYPECACHE
typecache subtypecache(-1);
#endif

void prune_glbcache()
{
  glbcache.prune();
#ifdef SUBTYPECACHE
  subtypecache.prune();
#endif
}

#endif


bool subtype(int a, int b)
// is `a' a subtype of `b'
{
  if(a == b) return true; // every type is subtype of itself
  if(b == BI_TOP) return true; // every type is a subtype of top

  if(a == -1) return true; // bottom is subtype of everything
  if(b == -1) return false; // no other type is a subtype of bottom
  
#ifdef FLOP
  if(leaftypeparent[a] != -1)
    return subtype(leaftypeparent[a], b);
  if(leaftypeparent[b] != -1)
    return false; // only leaftypes can be subtypes of a leaftype
#else
  if(is_leaftype(a))
    return subtype(leaftypeparent[a - first_leaftype], b);
  if(is_leaftype(b))
    return false; // only leaftypes can be subtypes of a leaftype

#ifdef SUBTYPECACHE
  // result is a _reference_ to the cache entry -> automatic writeback
  int &result = subtypecache[ (typecachekey_t) a*ntypes + b ];
  if(result != -1) return result;
  return (result = core_subtype(a, b));
#endif

#endif

  return core_subtype(a, b);
}

#ifndef FLOP
void
subtype_bidir(int A, int B, bool &a, bool &b)
{
    if(A == B) // every type is subtype of itself
    {
        a = b = true;
        return;
    }

    if(A == BI_TOP)
    {
        a = false;
        b = true;
        return;
    }

    if(B == BI_TOP)
    {
        a = true;
        b = false;
        return;
    }

    if(A == -1)
    {
        a = true;
        b = false;
        return;
    }

    if(B == -1)
    {
        a = false;
        b = true;
        return;
    }

    if(is_leaftype(A) || is_leaftype(B))
    {
        // _fixme_ this is inefficient
        a = subtype(A, B);
        b = subtype(B, A);
        return;
    }
    
    subset_bidir(*typecode[A], *typecode[B], a, b);
}
#endif

#ifndef FLOP
int leaftype_parent(int t)
{
  if(is_leaftype(t))
    return leaftypeparent[t - first_leaftype];
  else
    return t;
}
#endif

int glb(int s1, int s2)
{
  if(s1 == s2) return s1;

  if(s2 < s1)
    {
      int s;
      s = s1; s1 = s2; s2 = s;
    }

  // now we know that s1 < s2

  if(s1 == BI_TOP) return s2;
  if(s1 < 0) return -1;

#ifndef FLOP
  // result is a _reference_ to the cache entry -> automatic writeback
  int &result = glbcache[ (typecachekey_t) s1*ntypes + s2 ];
  if(result) return result;

  if(is_leaftype(s1))
    {
      if(subtype(s1, s2))
        return (result = s1);
      else if(subtype(s2, s1))
        return (result = s2);
      else 
        return (result = -1);
    }
  else if(is_leaftype(s2))
    {
      if(subtype(s2, s1))
        return (result = s2);
      else if(subtype(s1, s2))
        return (result = s1);
      else
        return (result = -1);
    }
#else
  int result;

  if(leaftypeparent[s1] != -1)
    {
      if(subtype(s1, s2))
        return s1;
      else if(subtype(s2, s1))
	return s2;
      else
	return -1;
    }
  else if(leaftypeparent[s2] != -1)
    {
      if(subtype(s2, s1))
        return s2;
      else if(subtype(s1, s2))
	return s1;
      else
	return 1;
    }
#endif

  return (result = core_glb(s1,s2));
}

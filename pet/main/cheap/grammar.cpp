/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* grammar, lexicon and lexicon entries */

#include <string.h>
#include <ctype.h>
#ifndef __BORLANDC__
#include <sys/time.h>
#endif

#include "cheap.h"
#include "utility.h"
#include "types.h"
#include "fs.h"
#include "grammar.h"
#include "dumper.h"
#include "grammar-dump.h"
#include "tsdb++.h"

#ifdef __BORLANDC__
#define strcasecmp strcmpi
#endif

int lex_entry::next_id = 0;
char *lex_entry::affix_path = 0;
int grammar_rule::next_id = 0;

lex_entry::lex_entry(dumper *f)
{
  _id = next_id++;
  
  _base = f->undump_int();
  _affix = f->undump_int();
  _inflpos = f->undump_char();
  _nwords = f->undump_char();

  _orth = new char*[_nwords];

  for(int j = 0; j < _nwords; j++)
    {
      _orth[j] = f->undump_string();
      strtolower(_orth[j]);
    }

  _p = 0;
  _invalid = false;

  char *v;
  if((v = cheap_settings->value("default-le-priority")) != 0)
    {
      _p = strtoint(v, "as value of default-le-priority");
    }

  if((v = cheap_settings->sassoc("likely-le-types", typenames[_base])) != 0)
    {
      _p = strtoint(v, "in value of likely-le-types");
    }

  if((v = cheap_settings->sassoc("unlikely-le-types", typenames[_base])) != 0)
    {
      _p = strtoint(v, "in value of unlikely-le-types");
    }
}

lex_entry::lex_entry(const lex_entry &le)
{
  throw error("unexpected call to copy constructor of lex_entry");
}

lex_entry::~lex_entry()
{
  // XXX - also delete elements
  delete[] _orth;
}

lex_entry& lex_entry::operator=(const lex_entry &le)
{
  throw error("unexpected call to assignment operator of lex_entry");
}

void lex_entry::print(FILE *f)
{
  fprintf(f, "%s[%s@%d] <%d>:", name(), affixname(), _inflpos, _p);
  for(int i = 0; i < _nwords; i++)
    fprintf(f, " \"%s\"", _orth[i]);
}

void lex_entry::print_derivation(FILE *f, bool quoted, 
                                 int start, int end, int id)
{

  if(_affix != -1)
    fprintf (f, "(%d %s %d %d %d ",
	     id, affixname(), _p, start, end);

  fprintf (f, 
           "(%d %s %d %d %d (%s\"", 
           id, name(), _p, start, end, quoted ? "\\" : "");

  for(int i = 0; i < _nwords; i++)
    fprintf(f, "%s%s", i == 0 ? "" : " ", _orth[i]);

  fprintf(f, "%s\" %d %d))", quoted ? "\\" : "", start, end);

  if(_affix != -1)
    fprintf(f, ")");
}

#ifdef TSDBAPI
void lex_entry::tsdb_print_derivation(int start, int end, int id)
{
  if(_affix != -1)
    capi_printf("(%d %s %d %d %d ",
		id, affixname(), _p, start, end);

  capi_printf("(%d %s %d %d %d (\\\"", id, name(), _p, start, end);

  for(int i = 0; i < _nwords; i++) {
    if(i) capi_putc(' ');
    //
    // _hack_
    // really, capi_printf() should do appropriate escaping, depending on the
    // context; but we would need to elaborate it significantly, i guess, so 
    // hack around it for the time being: it seems unlikely, we will see 
    // embedded quotes anyplace else.                   (1-feb-01  -  oe@yy)
    //
    for(char *foo = _orth[i]; *foo; foo++) {
      if(*foo == '"' || *foo == '\\') {
        capi_putc('\\');
        capi_putc('\\');
        capi_putc('\\');
      } /* if */
      capi_putc(*foo);
    } /* for */
  } /* for */

  capi_printf( "\\\" %d %d))", start, end);

  if(_affix != -1)
    capi_printf( ")");
}
#endif

bool lex_entry::matches0(int pos, tokenlist *input)
// assumes key position matches
{
  if(_nwords > 0)
    {
      if((pos - _inflpos >= 0) // within input boundaries?
         && (pos - _inflpos + _nwords <= input->length())) 
	{
	  int i; bool mismatch;
	  
	  for(i = 0, mismatch = false; i < _nwords && !mismatch; i++)
	    {
	      if(strcasecmp(_orth[i], (*input)[pos-_inflpos+i].c_str()) != 0)
		mismatch = true;
	    }
	  return !mismatch;
	}
      else
	return false;
    }
  else
    return true;
}

bool lex_entry::matches(int pos, tokenlist *input)
{
  if(strcasecmp((*input)[pos].c_str(), _orth[_inflpos]) == 0) // key position matches
    {
      return matches0(pos, input);
    }

  return false;
}


fs lex_entry::instantiate()
{
  if(_invalid) return fs();

  fs e(leaftype_parent(_base));

  fs expanded = unify(e, (fs(_base)), e);

  if(!expanded.valid())
    throw error("invalid lexentry " + description() + " (cannot expand)");

  if(_affix == -1)
    {
#ifdef PACKING
      return packing_partial_copy(expanded);
#else
      return expanded;
#endif
    }
  else
    {
      fs f = fs(_affix);
      fs arg = f.get_path_value(affix_path);

      if(!arg.valid())
        throw error("invalid lexentry " + description() + " (no affixpath)");

      fs le = unify(f, expanded, arg); 

      if(!le.valid())
	{
	  _invalid = true; // cache failure for further lookups
	  if(!cheap_settings->lookup("lex-entries-can-fail"))
	    throw error("invalid lexentry " + description() + " (incompatible affix)");
	}

#ifdef PACKING
      return packing_partial_copy(le);
#else
      return le;
#endif
    }
}

grammar_rule::grammar_rule(dumper *f)
  : _tofill(0), _hyper(true)
{
  int i;
  
  int keyarg, headdtr;
  
  _id = next_id++;

  _type = f->undump_int();
  _arity = f->undump_char();
  keyarg = f->undump_char();
  headdtr = f->undump_char();

  _qc_vector = new type_t *[_arity];

  // 0: key-driven, 1: l-r, 2: r-l, 3: head-driven

  if(opt_key == 0)
    {
      if(keyarg != -1) 
        _tofill = append(_tofill, keyarg);
    }
  else if(opt_key == 3)
    {
      if(headdtr != -1) 
        _tofill = append(_tofill, headdtr);
      else if(keyarg != -1) 
        _tofill = append(_tofill, keyarg);
    }

  if(opt_key != 2)
    {
      for(i = 1; i <= _arity; i++)
        if(!contains(_tofill, i)) _tofill = append(_tofill, i);
    }
  else
    {
      for(i = _arity; i >= 1; i--)
        if(!contains(_tofill, i)) _tofill = append(_tofill, i);
    }


  for(i = 0; i < _arity; i++)
    _qc_vector[i] = 0;

  // determine priority
  _p = 0;

  char *v;
  if((v = cheap_settings->value("default-rule-priority")) != 0)
    {
      _p = strtoint(v, "as value of default-rule-priority");
    }

  if((v = cheap_settings->sassoc("rule-priorities", typenames[_type])) != 0)
    {
      _p = strtoint(v, "in value of rule-priorities");
    }

  // hyper activity disabled for this rule?
  if(cheap_settings->member("depressive-rules", typenames[_type]))
    {
      _hyper = false;
    }
  
  if(_arity > 2)
    _hyper = false;
}

void grammar_rule::print(FILE *f)
{
  fprintf(f, "rule `%s' %s <%d> (", typenames[_type], _hyper == false ? "-HA" : "", _p);
  
  list_int *l = _tofill;
  while(l)
    {
      fprintf(f, " %d", first(l));
      l = rest(l);
    }

  fprintf(f, ")");
}

fs grammar_rule::instantiate()
{
#ifdef PACKING
  return packing_partial_copy(fs(_type));
#else
  return fs(_type); 
#endif
}

void grammar_rule::init_qc_vector()
{
  fs_alloc_state FSAS;

  fs f = instantiate();
  for(int i = 1; i <= _arity; i++)
    {
      fs arg = f.nth_arg(i);

      _qc_vector[i-1] = get_qc_vector(arg);
    }
}

void undump_toc(dumper *f)
{
  int toc_hierarchy, toc_tables, toc_rules, toc_lexicon, toc_constraints;

  toc_hierarchy = f->undump_int();
  toc_tables = f->undump_int();
  toc_rules = f->undump_int();
  toc_lexicon = f->undump_int();
  toc_constraints = f->undump_int();
  if(verbosity > 10)
    fprintf(stderr, "TOC:\n  hierarchy:   %d\n  tables:      %d\n   rules:       %d\n  lexicon:     %d\n  constraints: %d\n",
	    toc_hierarchy, toc_tables, toc_rules, toc_lexicon, toc_constraints);
}

// construct grammar object from binary representation in a file
grammar::grammar(char * filename)
  : _lexicon(), _dict_lexicon(), _rules(),
    _fsf(0), _qc_inst(0), _deleted_daughters(0)
{
  dumper dmp(filename);

  char *desc = undump_header(&dmp);
  if(desc) fprintf(fstatus, "(%s) ", desc);
  undump_toc(&dmp);

  // symbol tables
  _fsf = new fs_factory(&dmp);

  // hierarchy
  undump_hierarchy(&dmp);
  undump_tables(&dmp);

  if(dag_nocasts == false)
    fprintf(fstatus, "[ casts ] ");

  // call unifier specific initialization
  dag_initialize();

  init_parameters();

  // grammar rules
  _nrules = dmp.undump_int();
  for(int i = 0; i < _nrules; i++)
    {
      _rules.push_front(new grammar_rule(&dmp));
    }
  if(verbosity > 4) fprintf(fstatus, "%d rules ", _nrules);

  int nles = dmp.undump_int();
  for(int i = 0; i < nles; i++)
    {
      lex_entry *le = new lex_entry(&dmp); 
      _lexicon.push_front(le);

      _dict_lexicon.insert(pair<const string,lex_entry *>(le->key(), le));
    }

  if(verbosity > 4) fprintf(fstatus, "%d lexentries ", nles);

  _fsf->initialize(&dmp, _qc_inst);

  if(opt_nqc && _qc_inst == 0)
    {
      fprintf(fstatus, "[ disabling quickcheck ] ");
      opt_nqc = 0;
    }

  if(opt_nqc != 0)
    {
      if(opt_nqc > 0 && opt_nqc < qc_len)
	qc_len = opt_nqc;

      init_rule_qc();
    }

  if(opt_filter)
    {
      bool save = opt_compute_qc;
      opt_compute_qc = false;
      initialize_filter();
      opt_compute_qc = save;
    }
  else
    _filter = 0;

  init_grammar_info();

  get_unifier_stats();
}

void grammar::init_parameters()
{
  char *v;

  struct setting *set;
  if((set = cheap_settings->lookup("weighted-start-symbols")))
    {
      _root_insts = 0;
      for(int i = set->n - 2; i >= 0; i -= 2)
	{
	  _root_insts = cons(lookup_type(set->values[i]), _root_insts);
	  if(first(_root_insts) == -1)
	    throw error("undefined start symbol");

	  int weight = strtoint(set->values[i+1], "in weighted-start-symbols");
          _root_weight[first(_root_insts)] = weight;
	}
    }
  else if((set = cheap_settings->lookup("start-symbols")))
    {
      _root_insts = 0;
      for(int i = set->n-1; i >= 0; i--)
	{
	  _root_insts = cons(lookup_type(set->values[i]), _root_insts);
	  if(first(_root_insts) == -1)
	    throw error("undefined start symbol");
          _root_weight[first(_root_insts)] = 0;
	}
    }
  else
    _root_insts = 0;

  if((v = cheap_settings->value("qc-structure")))
    {
      _qc_inst = lookup_type(v);
      if(_qc_inst == -1) _qc_inst = 0;
    }
  else
    _qc_inst = 0;

  _deleted_daughters = 0;
  set = cheap_settings->lookup("deleted-daughters");
  if(set)
    {
      for(int i = 0; i < set->n; i++)
	{
	  int a;
	  if((a = lookup_attr(set->values[i])) != -1)
	    _deleted_daughters = cons(a, _deleted_daughters);
	  else
	    {
	      fprintf(ferr, "ignoring unknown attribute `%s' in deleted_daughters.\n",
		      set->values[i]);
	    }
	}
    }

  lex_entry::affix_path = cheap_settings->value("affixation-path");
}

void grammar::init_grammar_info()
{
  _info.version =
    _info.vocabulary =
    _info.ntypes =
    _info.ntemplates = "";

  int ind = lookup_type("grammar_info"); // XXX make configurable
  if(ind != -1)
    {
      fs f(ind);
      _info.version = f.get_attr_value("GRAMMAR_VERSION").name();
      _info.vocabulary = f.get_attr_value("GRAMMAR_VOCABULARY").name();
      _info.ntypes =  f.get_attr_value("GRAMMAR_NTYPES").name();
      _info.ntemplates =  f.get_attr_value("GRAMMAR_NTEMPLATES").name();
    }
}

void grammar::init_rule_qc()
{
  for(rule_iter iter(this); iter.valid(); iter++)
    {
      grammar_rule *rule = iter.current();

      rule->init_qc_vector();
    }
}

void grammar::initialize_filter()
{
  fs_alloc_state S0;
  _filter = new char[_nrules * _nrules];

  for(rule_iter daughters(this); daughters.valid(); daughters++)
    {
      fs_alloc_state S1;
      grammar_rule *daughter = daughters.current();
      fs daughter_fs = copy(daughter->instantiate());
      
      for(rule_iter mothers(this); mothers.valid(); mothers++)
	{
	  grammar_rule *mother = mothers.current();

	  _filter[daughter->id() + _nrules * mother->id()] = 0;

	  for(int arg = 1; arg <= mother->arity(); arg++)
	    {
	      fs_alloc_state S2;
	      fs mother_fs = mother->instantiate();
	      fs arg_fs = mother_fs.nth_arg(arg);

	      if(unify(mother_fs, daughter_fs, arg_fs).valid())
		{
		  _filter[daughter->id() + _nrules * mother->id()] |= (1 << (arg - 1));
		}
	    }
	}
    }

  S0.clear_stats();
}

grammar::~grammar()
{
  for(list<lex_entry *>::iterator pos = _lexicon.begin();
      pos != _lexicon.end(); ++pos)
    delete *pos;

  for(list<grammar_rule *>::iterator pos = _rules.begin();
      pos != _rules.end(); ++pos)
    delete *pos;

  if(_filter)
    delete[] _filter;

  free_list(_deleted_daughters);

  fs_alloc_state FSAS;
  FSAS.reset();
}

int grammar::nhyperrules()
{
  int n = 0;
  for(rule_iter iter(this); iter.valid(); iter++)
    if(iter.current()->hyperactive()) n++;

  return n;
}

bool grammar::root(fs &candidate, int &maxp)
{
  list_int *r;
  for(r = _root_insts; r != 0; r = rest(r))
    {
      if(compatible(fs(first(r)), candidate))
	{
	  maxp = _root_weight[first(r)];
	  return true;
	}
    }
  return false;
}

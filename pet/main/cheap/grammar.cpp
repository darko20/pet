/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* grammar, lexicon and lexicon entries */

#include "pet-system.h"

#include "grammar.h"
#include "cheap.h"
#include "fs.h"
#include "../common/utility.h"
#include "dumper.h"
#include "grammar-dump.h"
#include "tsdb++.h"
#ifdef ICU
#include "unicode.h"
#endif
#ifdef ONLINEMORPH
#include "morph.h"
#endif

int lex_stem::next_id = 0;
int grammar_rule::next_id = 0;

vector<string> lex_stem::get_stems()
{
  fs_alloc_state FSAS;
  vector <string> orth;
  struct dag_node *dag;
  fs e = instantiate();

  dag = dag_get_path_value(e.dag(),
			   cheap_settings->req_value("orth-path"));
  if(dag == FAIL)
    {
      fprintf(ferr, "no orth-path in `%s'\n", typenames[_type]);
      if(verbosity > 9)
	{
	  dag_print(stderr, e.dag());
	  fprintf(ferr, "\n");
	}

      orth.push_back("");
      return orth;
    }

  list <struct dag_node *> stemlist = dag_get_list(dag);

  if(stemlist.size() == 0)
    {
      // might be a singleton
      if(is_type(dag_type(dag)))
	{
	  string s(typenames[dag_type(dag)]);
	  orth.push_back(s.substr(1, s.length()-1));
	}
      else
	{
	  fprintf(ferr, "no valid stem in `%s'\n", typenames[_type]);
	}

      return orth;
    }

  int n = 0;

  for(list<dag_node *>::iterator iter = stemlist.begin();
      iter != stemlist.end(); ++iter)
    {
      dag = *iter;
      if(dag == FAIL)
	{
	  fprintf(ferr, "no stem %d in `%s'\n", n, typenames[_type]);
	  return vector<string>();
	}

      if(is_type(dag_type(dag)))
	{
	  string s(typenames[dag_type(dag)]);
	  orth.push_back(s.substr(1,s.length()-2));
	}
      else
	{
	  fprintf(ferr, "no valid stem %d in `%s'\n", n, typenames[_type]);
	  return vector<string>();
	}
      n++;
    }

  return orth;
}

bool lexentry_status(type_t t)
{
  return cheap_settings->statusmember("lexentry-status-values", typestatus[t]);
}

lex_stem::lex_stem(type_t t) :
  _id(next_id++), _type(t), _orth(0), _p(0)
{
  vector<string> orth = get_stems();
  _nwords = orth.size();

  if(_nwords == 0) // invalid entry
    return;

  _orth = New char*[_nwords];

  for(int j = 0; j < _nwords; j++)
    {
      _orth[j] = strdup(orth[j].c_str());
      strtolower(_orth[j]);
    }

  char *v;
  if((v = cheap_settings->value("default-le-priority")) != 0)
    {
      _p = strtoint(v, "as value of default-le-priority");
    }

  if((v = cheap_settings->sassoc("likely-le-types", _type)) != 0)
    {
      _p = strtoint(v, "in value of likely-le-types");
    }

  if((v = cheap_settings->sassoc("unlikely-le-types", _type)) != 0)
    {
      _p = strtoint(v, "in value of unlikely-le-types");
    }

  if(verbosity > 14)
    {
      print(fstatus); fprintf(fstatus, "\n");
    }
}

lex_stem::lex_stem(const lex_stem &le)
{
  throw error("unexpected call to copy constructor of lex_stem");
}

lex_stem& lex_stem::operator=(const lex_stem &le)
{
  throw error("unexpected call to assignment operator of lex_stem");
}

lex_stem::~lex_stem()
{
  for(int j = 0; j < _nwords; j++)
    free(_orth[j]);
  if(_orth)
    delete[] _orth;
}

void lex_stem::print(FILE *f) const
{
  fprintf(f, "%s <%d>:", printname(), _p);
  for(int i = 0; i < _nwords; i++)
    fprintf(f, " \"%s\"", _orth[i]);
}

fs lex_stem::instantiate()
{
  fs e(leaftype_parent(_type));

  fs expanded = unify(e, (fs(_type)), e);

  if(!expanded.valid())
    throw error(string("invalid lex_stem `") + printname() + "' (cannot expand)");

#ifdef PACKING
  return packing_partial_copy(expanded);
#else
  return expanded;
#endif
}

char *full_form::_affix_path = 0;

full_form::full_form(dumper *f, grammar *G)
{
  int preterminal = f->undump_int();
  _stem = G->lookup_stem(preterminal);
  
  int affix = f->undump_int();
  _affixes = (affix == -1) ? 0 : cons(affix, 0);

  _offset = f->undump_char();
  
  char *s = f->undump_string(); 
  _form = string(s);
  delete[] s;

  if(verbosity > 14)
    {
      print(fstatus); fprintf(fstatus, "\n");
    }
}

#ifdef ONLINEMORPH
full_form::full_form(lex_stem *st, morph_analysis a)
{
  _stem = st;

  _affixes = 0;
  for(list<type_t>::reverse_iterator it = a.rules().rbegin();
      it != a.rules().rend(); ++it)
    _affixes = cons(*it, _affixes);

  _offset = _stem->inflpos();

  _form = a.complex();
}
#endif

fs full_form::instantiate()
{
  if(!valid())
    throw error("trying to instantiate invalid full form");

  // get the base
  fs res = _stem->instantiate();

  if(!res.valid()) 
    throw error("cannot instantiate base of full form");

  // apply modifications
  if(!res.modify(_mods))
    throw error("failure applying modifications");
 
  return res;
}

void full_form::print(FILE *f)
{
  if(valid())
    {
      fprintf(f, "(");
      for(list_int *affix = _affixes; affix != 0; affix = rest(affix))
	{
	  fprintf(f, "%s@%d ", printnames[first(affix)], offset());
	}

      fprintf(f, "(");
      _stem->print(f);
      fprintf(f, ")");

      for(int i = 0; i < length(); i++)
	fprintf(f, " \"%s\"", orth(i).c_str());

      fprintf(f, ")");
    }
  else
    fprintf(f, "(invalid full form)");
}

string full_form::affixprintname()
{
  string name;

  name += string("[");
  for(list_int *affix = _affixes; affix != 0; affix = rest(affix))
    {
      name += printnames[first(affix)];
    }
  name += string("]");

  return name;
}

string full_form::description()
{
  string desc;

  if(valid())
  {
    desc = string(stemprintname());
    desc += affixprintname();
  }
  else
  {
    desc = string("(invalid full form)");
  }

  return desc;
}

bool rule_status(type_t t)
{
  return cheap_settings->statusmember("rule-status-values", typestatus[t])
    || cheap_settings->statusmember("lexrule-status-values", typestatus[t]);
}

grammar_rule::grammar_rule(type_t t)
  : _tofill(0), _hyper(true), _spanningonly(false)
{
  _id = next_id++;
  _type = t;

  if(cheap_settings->statusmember("lexrule-status-values", typestatus[t]))
    _trait = LEX_TRAIT;
  else
    _trait = SYNTAX_TRAIT;

  struct dag_node *dag, *head_dtr;
  list <struct dag_node *> argslist;
      
  dag = dag_get_path_value(typedag[t],
			   cheap_settings->req_value("rule-args-path"));

  argslist = dag_get_list(dag);

  _arity = argslist.size();

  head_dtr = dag_get_path_value(typedag[t],
				cheap_settings->req_value("head-dtr-path"));

  int n = 1, keyarg = -1, head = -1;
  for(list<dag_node *>::iterator currentdag = argslist.begin();
      currentdag != argslist.end(); ++ currentdag)
    {
      if(cheap_settings->lookup("keyarg-marker-path"))
	{
	  struct dag_node *k;

	  k = dag_get_path_value(*currentdag,
				 cheap_settings->value("keyarg-marker-path"));
	  
	  if(k != FAIL && dag_type(k) ==
	     lookup_type(cheap_settings->req_value("true-type")))
	    keyarg = n;
	}
	  
      if(*currentdag == head_dtr)
	head = n;

      n++;
    }

  if(cheap_settings->lookup("rule-keyargs"))
    {
      if(keyarg != -1)
	{
	  fprintf(ferr, "warning: both keyarg-marker-path and rule-keyargs "
		  "supply information on key argument...\n");
	}
      char *s = cheap_settings->assoc("rule-keyargs", typenames[t]);
      if(s && strtoint(s, "in `rule-keyargs'"))
	{
	  keyarg = strtoint(s, "in `rule-keyargs'");
	}
    }

  _qc_vector = New type_t *[_arity];

  // 0: key-driven, 1: l-r, 2: r-l, 3: head-driven

  if(opt_key == 0)
    {
      if(keyarg != -1) 
        _tofill = append(_tofill, keyarg);
    }
  else if(opt_key == 3)
    {
      if(head != -1) 
        _tofill = append(_tofill, head);
      else if(keyarg != -1) 
        _tofill = append(_tofill, keyarg);
    }

  if(opt_key != 2)
    {
      for(int i = 1; i <= _arity; i++)
        if(!contains(_tofill, i)) _tofill = append(_tofill, i);
    }
  else
    {
      for(int i = _arity; i >= 1; i--)
        if(!contains(_tofill, i)) _tofill = append(_tofill, i);
    }

  for(int i = 0; i < _arity; i++)
    _qc_vector[i] = 0;

  // determine priority
  _p = 0;

  char *v;
  if((v = cheap_settings->value("default-rule-priority")) != 0)
    {
      _p = strtoint(v, "as value of default-rule-priority");
    }

  if((v = cheap_settings->sassoc("rule-priorities", _type)) != 0)
    {
      _p = strtoint(v, "in value of rule-priorities");
    }

  // hyper activity disabled for this rule?
  if(cheap_settings->member("depressive-rules", typenames[_type]))
    {
      _hyper = false;
    }

  if(cheap_settings->member("spanning-only-rules", typenames[_type]))
    {
      _spanningonly = true;
    }
  
  if(_arity > 2)
    _hyper = false;

  if(verbosity > 14)
    {
      print(fstatus); fprintf(fstatus, "\n");
    }
}

void grammar_rule::print(FILE *f)
{
  fprintf(f, "%s/%d%s <%d> (", printnames[_type], _arity,
	  _hyper == false ? "[-HA]" : "", _p);
  
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

bool grammar::punctuationp(const string &s)
{
#ifndef ICU
  if(_punctuation_characters.empty())
    return false;

  for(string::size_type i = 0; i < s.length(); i++)
    if(_punctuation_characters.find(s[i]) == STRING_NPOS)
      return false;
  
  return true;
#else
  UnicodeString U = Conv->convert(s);
  StringCharacterIterator it(U);

  UChar32 c;
  while(it.hasNext())
    {
      c = it.next32PostInc();
      if(_punctuation_characters.indexOf(c) == -1)
	return false;
    }

  return true;
#endif
}

bool genle_status(type_t t)
{
  return cheap_settings->statusmember("generic-lexentry-status-values",
				      typestatus[t]);
}

// this typedef works around a bug in Borland C++ Builder
typedef constraint_info *constraint_info_p;

void undump_dags(dumper *f, int qc_inst)
{
  struct dag_node *dag;
  
  initialize_dags(ntypes);

#ifdef CONSTRAINT_CACHE
  constraint_cache = New constraint_info_p [ntypes];
#endif

  for(int i = 0; i < ntypes; i++)
    {
      if(i == qc_inst)
	{
	  if(verbosity > 4) fprintf(fstatus, "qc structure `%s' ",
				    printnames[qc_inst]);
	  qc_paths = dag_read_qc_paths(f, opt_nqc, qc_len);
	  dag = 0;
	}
      else
	dag = dag_undump(f);

      register_dag(i, dag);
#ifdef CONSTRAINT_CACHE
      constraint_cache[i] = 0;
#endif
    }
}

#ifdef CONSTRAINT_CACHE
void free_constraint_cache()
{
  for(int i = 0; i < ntypes; i++)
  {
    constraint_info *c = constraint_cache[i];
    while(c != 0)
    {
      constraint_info *n = c->next;
      delete c;
      c = n;
    }
  }

  delete[] constraint_cache;
}
#endif

// construct grammar object from binary representation in a file
grammar::grammar(const char * filename)
  : _nrules(0), _weighted_roots(false), _root_insts(0),
    _generics(0), _filter(0), _qc_inst(0), _deleted_daughters(0) 
{
  initialize_encoding_converter(cheap_settings->req_value("encoding"));

  dumper dmp(filename);

  char *s = undump_header(&dmp);
  if(s) fprintf(fstatus, "(%s) ", s);

  dump_toc toc(&dmp);

  // symbol tables
  toc.goto_section(SEC_SYMTAB);
  undump_symbol_tables(&dmp);
  toc.goto_section(SEC_PRINTNAMES);
  undump_printnames(&dmp);

  // hierarchy
  toc.goto_section(SEC_HIERARCHY);
  undump_hierarchy(&dmp);

  toc.goto_section(SEC_FEATTABS);
  undump_tables(&dmp);

  // call unifier specific initialization
  dag_initialize();

  init_parameters();

  // constraints
  toc.goto_section(SEC_CONSTRAINTS);
  undump_dags(&dmp, _qc_inst);
  stop_creating_permanent_dags();

  if(opt_nqc && _qc_inst == 0)
    {
      fprintf(fstatus, "[ disabling quickcheck ] ");
      opt_nqc = 0;
    }

  // find grammar rules and stems
  for(int i = 0; i < ntypes; i++)
    {
      if(lexentry_status(i))
	{
	  lex_stem *st = New lex_stem(i);
	  _lexicon[i] = st;
	  _stemlexicon.insert(make_pair(st->orth(st->inflpos()), st));
	}
      else if(rule_status(i))
	{
	  _rules.push_front(New grammar_rule(i));
	}
      else if(genle_status(i))
	{
	  _generics = cons(i, _generics);
	  _lexicon[i] = New lex_stem(i);
	}
    }
  _nrules = _rules.size();
  if(verbosity > 4) fprintf(fstatus, "%d+%d stems, %d rules ", nstems(), 
			    length(_generics), _nrules);

  // full forms
  if(toc.goto_section(SEC_FULLFORMS))
    {
      int nffs = dmp.undump_int();
      for(int i = 0; i < nffs; i++)
	{
	  full_form *ff = New full_form(&dmp, this);
	  if(ff->valid())
	    _fullforms.insert(make_pair(ff->key(), ff));
	  else
	    delete ff;
	}

      if(verbosity > 4) fprintf(fstatus, "%d full form entries ", nffs);
    }

#ifdef ONLINEMORPH
  // inflectional rules
  _morph = New morph_analyzer(this);

  if(cheap_settings->lookup("irregular-forms-only"))
    _morph->set_irregular_only(true);

  if(toc.goto_section(SEC_INFLR))
    {
      int ninflrs = dmp.undump_int();
      for(int i = 0; i < ninflrs; i++)
	{
	  int t = dmp.undump_int();
	  char *r = dmp.undump_string();

	  if(r == 0)
	    continue;
	  
	  if(t == -1)
	    _morph->add_global(string(r));
	  else
	    {
	      _morph->add_rule(t, string(r));

	      bool found = false;
	      for(rule_iter iter(this); iter.valid(); iter++)
		{
		  if(iter.current()->type() == t)
		    {
		      if(iter.current()->trait() == SYNTAX_TRAIT)
			fprintf(ferr, "warning: found syntax `%s' rule with "
				"attached infl rule `%s'\n", printnames[t], r);
		      
		      iter.current()->trait(INFL_TRAIT);
		      found = true;
		    }
		}
	      
	      if(!found)
		fprintf(ferr, "warning: rule `%s' with infl annotation `%s' "
			"doesn't correspond to any of the parser's rules\n",
			printnames[t], r);
	    }

	  delete[] r;
	}
      if(verbosity > 4) fprintf(fstatus, "%d infl rules ", ninflrs);
      if(verbosity >14) _morph->print(fstatus);
    }
  // irregular forms
  if(toc.goto_section(SEC_IRREGS))
    {
      int nirregs = dmp.undump_int();
      for(int i = 0; i < nirregs; i++)
	{
	  char *form = dmp.undump_string();
	  char *infl = dmp.undump_string();
	  char *stem = dmp.undump_string();

	  strtolower(infl);
	  type_t inflr = lookup_type(infl);
	  if(inflr == -1)
	    {
	      fprintf(ferr, "Ignoring entry with unknown rule `%s' "
		      "in irregular forms\n", infl);
	      delete[] form; delete[] infl; delete[] stem;
	      continue;
	    }
	  
	  _morph->add_irreg(string(stem), inflr, string(form));
	  delete[] form; delete[] infl; delete[] stem;
	}
    }
#endif

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

  s = cheap_settings->value("punctuation-characters");
  string pcs;
  if(s == 0)
    pcs = " \t?!.:;,()-+*$\n";
  else
    pcs = convert_escapes(string(s));

#ifndef ICU
  _punctuation_characters = pcs;
#else
  _punctuation_characters = Conv->convert(pcs);
#endif  


  get_unifier_stats();
}

void grammar::init_parameters()
{
  char *v;

  struct setting *set;
  if((set = cheap_settings->lookup("weighted-start-symbols")) != 0)
    {
      _root_insts = 0;
      for(int i = set->n - 2; i >= 0; i -= 2)
	{
	  _root_insts = cons(lookup_type(set->values[i]), _root_insts);
	  if(first(_root_insts) == -1)
	    throw error(string("undefined start symbol `") +
			string(set->values[i]) + string("'"));

	  int weight = strtoint(set->values[i+1], "in weighted-start-symbols");
          _root_weight[first(_root_insts)] = weight;
	  if(weight != 0)
	    _weighted_roots = true;
	}
    }
  else if((set = cheap_settings->lookup("start-symbols")) != 0)
    {
      _root_insts = 0;
      for(int i = set->n-1; i >= 0; i--)
	{
	  _root_insts = cons(lookup_type(set->values[i]), _root_insts);
	  if(first(_root_insts) == -1)
	    throw error(string("undefined start symbol `") +
			string(set->values[i]) + string("'"));
          _root_weight[first(_root_insts)] = 0;
	}
    }
  else
    _root_insts = 0;

  if((v = cheap_settings->value("qc-structure")) != 0)
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
  full_form::_affix_path = cheap_settings->value("affixation-path");
}

void grammar::init_grammar_info()
{
  _info.version =
    _info.ntypes =
    _info.ntemplates = "";

  int ind = lookup_type("grammar_info"); // XXX make configurable
  if(ind != -1)
    {
      fs f(ind);
      _info.version = f.get_attr_value("GRAMMAR_VERSION").printname();
      _info.ntypes =  f.get_attr_value("GRAMMAR_NTYPES").printname();
      _info.ntemplates =  f.get_attr_value("GRAMMAR_NTEMPLATES").printname();
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
  _filter = New char[_nrules * _nrules];

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
  for(map<type_t, lex_stem *>::iterator pos = _lexicon.begin();
      pos != _lexicon.end(); ++pos)
    delete pos->second;

  for(ffdict::iterator pos = _fullforms.begin();
      pos != _fullforms.end(); ++pos)
    delete pos->second;

  for(list<grammar_rule *>::iterator pos = _rules.begin();
      pos != _rules.end(); ++pos)
    delete *pos;

  if(_filter)
    delete[] _filter;

#ifdef ONLINEMORPH
  if(_morph)
    delete _morph;
#endif

  dag_qc_free();

#ifdef CONSTRAINT_CACHE
  free_constraint_cache();
#endif

  free_list(_deleted_daughters);
  free_type_tables();

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

bool grammar::root(fs &candidate, type_t &rule, int &maxp)
{
  list_int *r; bool good = false;
  for(r = _root_insts, maxp = 0, rule = -1; r != 0; r = rest(r))
    {
      if(compatible(fs(first(r)), candidate))
	{
	  if(_weighted_roots == false)
	    {
	      rule = first(r);
	      return true;
	    }

	  good = true;
	  int weight = _root_weight[first(r)];
	  if(weight > maxp)
	    {
	      rule = first(r);
	      maxp = weight;
	    }
	}
    }
  return good;
}

lex_stem *grammar::lookup_stem(type_t inst_key)
{
  map<type_t, lex_stem *>::iterator it = _lexicon.find(inst_key);
  if (it != _lexicon.end())
    return it->second;
  else
    return 0;
}

list<lex_stem *> grammar::lookup_stem(string s)
{
  list<lex_stem *> results;
 
  pair<multimap<string, lex_stem *>::iterator,
       multimap<string, lex_stem *>::iterator> eq =
    _stemlexicon.equal_range(s);

  for(multimap<string, lex_stem *>::iterator it = eq.first;
      it != eq.second; ++it)
    {
      results.push_back(it->second);
    }

  return results;
}

list<full_form> grammar::lookup_form(const string form)
{
  list<full_form> result;

#ifdef ONLINEMORPH
  list<morph_analysis> m = _morph->analyze(form);
  for(list<morph_analysis>::iterator it = m.begin(); it != m.end(); ++it)
    {
      for(list<lex_stem *>::iterator st_it = it->stems().begin();
	  st_it != it->stems().end(); ++st_it)
	result.push_back(full_form(*st_it, *it));
    }
#else
  pair<ffdict::iterator,ffdict::iterator> p = _fullforms.equal_range(form);
  for(ffdict::iterator it = p.first; it != p.second; ++it)
    {
      result.push_back(*it->second);
    }
#endif

  return result;
}

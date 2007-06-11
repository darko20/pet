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

/* grammar, lexicon and lexicon entries */

#include "grammar.h"

#include <sys/param.h>

#include "cheap.h"
#include "fs.h"
#include "parse.h"
#include "utility.h"
#include "dumper.h"
#include "grammar-dump.h"
#include "tsdb++.h"
#include "sm.h"
#include "restrictor.h"
#ifdef HAVE_ICU
#include "unicode.h"
#endif

#include "item-printer.h"

int grammar_rule::next_id = 0;

bool
lexentry_status(type_t t)
{
    return cheap_settings->statusmember("lexentry-status-values",
                                        typestatus[t]);
}

bool
genle_status(type_t t)
{
    return cheap_settings->statusmember("generic-lexentry-status-values",
                                        typestatus[t]);
}

bool
predle_status(type_t t)
{
  return cheap_settings->statusmember("predict-lexentry-status-values",
                                      typestatus[t]);
}

bool
rule_status(type_t t)
{
  return cheap_settings->statusmember("rule-status-values", typestatus[t]);
}

bool 
lex_rule_status(type_t t)
{
  return cheap_settings->statusmember("lexrule-status-values", typestatus[t]);
}

bool 
infl_rule_status(type_t t)
{
  return 
    cheap_settings->statusmember("infl-rule-status-values", typestatus[t]);
}

grammar_rule::grammar_rule(type_t t)
    : _tofill(0), _hyper(true), _spanningonly(false)
{
    _id = next_id++;
    _type = t;
    
    if(cheap_settings->statusmember("infl-rule-status-values", typestatus[t]))
        _trait = INFL_TRAIT;
    else if(cheap_settings->statusmember("lexrule-status-values", typestatus[t]))
        _trait = LEX_TRAIT;
    else
        _trait = SYNTAX_TRAIT;
    
    if(opt_packing)
        _f_restriced = packing_partial_copy(fs(_type),
                                            Grammar->packing_restrictor(),
                                            true);
    /* // Code to test restriction
      string name = (string) "/tmp/restrict/" + print_name(t);
      tFegramedPrinter fp(name.c_str());
      fp.print(_f_restriced.dag());
    } else {
      string name = (string) "/tmp/full/" + print_name(t);
      tFegramedPrinter fp(name.c_str());
      fp.print(fs(_type).dag());
    }
    */

    //
    // Determine arity, determine head and key daughters.
    // 
    
    struct dag_node *dag, *head_dtr;
    list <struct dag_node *> argslist;

    dag = dag_get_path_value(type_dag(t),
                             cheap_settings->req_value("rule-args-path"));
   
    if(dag == FAIL)
    {
        throw tError("Feature structure of rule " + string(type_name(t))
                    + " does not contain "
                    + string(cheap_settings->req_value("rule-args-path")));
    }	    

    argslist = dag_get_list(dag);
    
    _arity = argslist.size();

    if(_arity == 0) {
        throw tError("Rule " + string(type_name(t)) + " has arity zero");
    }
    
    head_dtr = dag_get_path_value(type_dag(t),
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
        char *s = cheap_settings->assoc("rule-keyargs", type_name(t));
        if(s && strtoint(s, "in `rule-keyargs'"))
        {
            keyarg = strtoint(s, "in `rule-keyargs'");
        }
    }
    
    _qc_vector_unif = new type_t *[_arity];
    
    //
    // Build the _tofill list which determines the order in which arguments
    // are filled. The opt_key option determines the strategy:
    // 0: key-driven, 1: l-r, 2: r-l, 3: head-driven
    //

    // _fix_me_
    // this is wrong for more than binary branching rules, 
    // since adjacency is not guarantueed.
    
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
        _qc_vector_unif[i] = 0;
    
    //
    // Disable hyper activity if this is a more than binary-branching
    // rule or if it's explicitely disabled by a setting.
    //

    if(_arity > 2
       || cheap_settings->member("depressive-rules", type_name(_type)))
    {
        _hyper = false;
    }

    if(cheap_settings->member("spanning-only-rules", type_name(_type)))
    {
        _spanningonly = true;
    }
    
    if(verbosity > 14)
    {
        print(fstatus); fprintf(fstatus, "\n");
    }
}

grammar_rule *
grammar_rule::make_grammar_rule(type_t t) {
  try {
    return new grammar_rule(t);
  }
  catch (tError e) {
    fprintf(ferr, "%s\n", e.getMessage().c_str());
  }
  return NULL;
}

void
grammar_rule::print(FILE *f)
{
    fprintf(f, "%s/%d%s (", print_name(_type), _arity,
            _hyper == false ? "[-HA]" : "");
    
    list_int *l = _tofill;
    while(l)
    {
        fprintf(f, " %d", first(l));
        l = rest(l);
    }
    
    fprintf(f, ")");
}

void
grammar_rule::lui_dump(const char *path) {

  if(chdir(path)) {
    fprintf(ferr, 
            "grammar_rule::lui_dump(): invalid target directory `%s'.\n",
            path);
    return;
  } // if
  char name[MAXPATHLEN + 1];
  sprintf(name, "rule.%d.lui", _id);
  FILE *stream;
  if((stream = fopen(name, "w")) == NULL) {
    fprintf(ferr, 
            "grammar_rule::lui_dump(): unable to open `%s' (in `%s').\n",
            name, path);
    return;
  } // if
  fprintf(stream, "avm -%d ", _id);
  fs foo = instantiate(true);
  foo.print(stream, DAG_FORMAT_LUI);
  fprintf(stream, " \"Rule # %d (%s)\"\f\n", _id, printname());
  fclose(stream);

} // grammar_rule::lui_dump()


fs
grammar_rule::instantiate(bool full)
{
  if(opt_packing && !full)
    return _f_restriced;
  else
    return fs(_type);
}

void
grammar_rule::init_qc_vector_unif()
{
    fs_alloc_state FSAS;
    
    fs f = instantiate();
    for(int i = 1; i <= _arity; i++)
    {
        fs arg = f.nth_arg(i);
        
        _qc_vector_unif[i-1] = get_qc_vector(qc_paths_unif, qc_len_unif, arg);
    }
}


void
undump_dags(dumper *f, int qc_inst_unif, int qc_inst_subs) {
  struct dag_node *dag;
  // allocate an array holding ntypes pointers to the typedags
  initialize_dags(ntypes);
    
#ifdef CONSTRAINT_CACHE
  // allocate an array holding ntypes pointers to typedag caches
  init_constraint_cache(ntypes);
#endif
 
  for(int i = 0; i < ntypes; i++) {
    if(qc_inst_unif != 0 && i == qc_inst_unif) {
      if(verbosity > 4) fprintf(fstatus, "[qc unif structure `%s'] ",
                                print_name(qc_inst_unif));
      qc_paths_unif = dag_read_qc_paths(f, opt_nqc_unif, qc_len_unif);
      dag = 0;
    }
    else if(qc_inst_subs && i == qc_inst_subs) {
      if(verbosity > 4) fprintf(fstatus, "[qc subs structure `%s'] ",
                                print_name(qc_inst_subs));
      qc_paths_subs = dag_read_qc_paths(f, opt_nqc_subs, qc_len_subs);
      dag = 0;
    }
    else
      dag = dag_undump(f);
        
    register_dag(i, dag);
  }

  if(qc_inst_unif != 0 && qc_inst_unif == qc_inst_subs) {
    qc_paths_subs = qc_paths_unif;
    qc_len_subs = qc_len_unif;
  }
}

// Construct a grammar object from binary representation in a file
tGrammar::tGrammar(const char * filename)
    : _properties(), _nrules(0), _root_insts(0), _generics(0),
      _filter(0), _subsumption_filter(0), _qc_inst_unif(0), _qc_inst_subs(0),
      _deleted_daughters(0), _packing_restrictor(0),
      _sm(0), _lexsm(0)
{
#ifdef HAVE_ICU
    initialize_encoding_converter(cheap_settings->req_value("encoding"));
#endif

    // _fix_me_
    // This is ugly. All grammar objects (rules, stems etc) should have a 
    // pointer back to the grammar. Until we have that, we use a global 
    // variable, which we have to initialize here so that it's already
    // set when stems and rules are constructed.
    Grammar = this;

    dumper dmp(filename);
    
    int version;
    char *s = undump_header(&dmp, version);
    if(s) fprintf(fstatus, "(%s) ", s);
    
    dump_toc toc(&dmp);
    
    // properties
    if(toc.goto_section(SEC_PROPERTIES))
        undump_properties(&dmp);

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
    
    // supertypes
    if(toc.goto_section(SEC_SUPERTYPES))
        undumpSupertypes(&dmp);

    // call unifier specific initialization
    dag_initialize();
    
    init_parameters();
    
    // constraints
    toc.goto_section(SEC_CONSTRAINTS);
    undump_dags(&dmp, _qc_inst_unif, _qc_inst_subs);
    initialize_maxapp();

    // Tell unifier that all dags created from now an are not considered to
    // be part of the grammar. This is needed for the smart copying algorithm.
    // _fix_me_
    // This only works when MARK_PERMANENT is defined. Otherwise, the decision
    // is hard-coded by using t_alloc or p_alloc. Ugly? Yes.
    stop_creating_permanent_dags();

    if(opt_nqc_unif && _qc_inst_unif == 0)
    {
        fprintf(fstatus, "[ disabling unification quickcheck ] ");
        opt_nqc_unif = 0;
    }

    if(opt_nqc_subs && _qc_inst_subs == 0)
    {
        fprintf(fstatus, "[ disabling subsumption quickcheck ] ");
        opt_nqc_subs = 0;
    }

    // find grammar rules and stems
    for(int i = 0; i < ntypes; i++)
    {
        if(lexentry_status(i))
        {
            lex_stem *st = new lex_stem(i);
            _lexicon[i] = st;
            _stemlexicon.insert(make_pair(st->orth(st->inflpos()), st));
#if defined(YY)
            if(opt_yy && st->length() > 1) 
                // for multiwords, insert additional index entry
            {
                string full = st->orth(0);
                for(int i = 1; i < st->length(); i++)
                    full += string(" ") + string(st->orth(i));
                _stemlexicon.insert(make_pair(full.c_str(), st));
            }
#endif
        }
        else if(rule_status(i))
        {
            grammar_rule *R = grammar_rule::make_grammar_rule(i);
            if (R != NULL) {
              _syn_rules.push_front(R);
              _rule_dict[i] = R;
            }
        }
        else if(lex_rule_status(i))
        {
            grammar_rule *R = grammar_rule::make_grammar_rule(i);
            if (R != NULL) {
              _lex_rules.push_front(R);
              _rule_dict[i] = R;
            }
        }
        else if(infl_rule_status(i))
        {
            grammar_rule *R = grammar_rule::make_grammar_rule(i);
            if (R != NULL) {
              _infl_rules.push_front(R);
              _rule_dict[i] = R;
            }
        }
        else if(genle_status(i))
        {
            _generics = cons(i, _generics);
            _lexicon[i] = new lex_stem(i);
        }
        else if (predle_status(i))
        {
          _predicts = cons(i, _predicts);
          _lexicon[i] = new lex_stem(i);
        }
    }

#ifdef DEBUG
    // print the rule dictionary
    fprintf(stderr, "rule dictionary:\n");
    for (map<type_t, grammar_rule*>::iterator it = _rule_dict.begin();
         it != _rule_dict.end(); it++) {
      fprintf(stderr, "%d %s %s\n", (*it).first, print_name((*it).first)
             , type_name((*it).first));
    }
#endif

    // Activate all rules for initialization etc.
    activate_all_rules();
    // The number of all rules for the unification and subsumption rule
    // filtering
    _nrules = _rules.size();
    if(verbosity > 4) fprintf(fstatus, "%d+%d stems, %d rules", nstems(), 
                              length(_generics), _nrules);

/*
// obsoleted by lexparser
#if 0
    // full forms
    if(toc.goto_section(SEC_FULLFORMS))
    {
        int nffs = dmp.undump_int();
        for(int i = 0; i < nffs; i++)
        {
            full_form *ff = new full_form(&dmp, this);
            if(ff->valid())
                _fullforms.insert(make_pair(ff->key(), ff));
            else
                delete ff;
        }

        if(verbosity > 4) fprintf(fstatus, ", %d full form entries", nffs);
    }

    if(_fullforms.size() == 0)
        opt_fullform_morph = false;

#ifdef ONLINEMORPH
    // inflectional rules
    _morph = new tMorphAnalyzer();

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
                            fprintf(ferr, "warning: found syntax `%s' rule "
                                    "with attached infl rule `%s'\n",
                                    print_name(t), r);
                        
                        iter.current()->trait(INFL_TRAIT);
                        found = true;
                    }
                }
                
                if(!found)
                    fprintf(ferr, "warning: rule `%s' with infl annotation "
                            "`%s' doesn't correspond to any of the parser's "
                            "rules\n", print_name(t), r);
            }

            delete[] r;
        }

        if(_morph->empty())
            opt_online_morph = false;
        
        if(verbosity > 4) fprintf(fstatus, ", %d infl rules", ninflrs);
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
#else
    _morph = NULL;  // this should be there anyway
#endif // endif to: if ONLINEMORPH
#endif // endif to: if 0
*/

    if(opt_nqc_unif != 0)
    {
        if(opt_nqc_unif > 0 && opt_nqc_unif < qc_len_unif)
            qc_len_unif = opt_nqc_unif;

        init_rule_qc_unif();
    }

    if(opt_nqc_subs != 0)
    {
        if(opt_nqc_subs > 0 && opt_nqc_subs < qc_len_subs)
            qc_len_subs = opt_nqc_subs;
    }

    if(opt_filter)
    {
      //char *save = opt_compute_qc;
      bool qc_subs = opt_compute_qc_subs;
      bool qc_unif = opt_compute_qc_unif;
      //opt_compute_qc = 0;
      initialize_filter();
      //opt_compute_qc = save;
      opt_compute_qc_subs = qc_subs;
      opt_compute_qc_unif = qc_unif;
    }

    char *sm_file;
    if((sm_file = cheap_settings->value("sm")) != 0)
    {
        // _fix_me_
        // Once we have more than just MEMs we need to add a dispatch facility
        // here, or have a factory build the models.
      try { _sm = new tMEM(this, sm_file, filename); }
      catch(tError &e)
      {
        fprintf(ferr, "\n%s", e.getMessage().c_str());
        _sm = 0;
      }
    }
    char *lexsm_file;
    if ((lexsm_file = cheap_settings->value("lexsm")) != 0) {
      try { _lexsm = new tMEM(this, lexsm_file, filename); }
      catch(tError &e) {
        fprintf(ferr, "\n%s", e.getMessage().c_str());
        _lexsm = 0;
      }
    }


#ifdef HAVE_EXTDICT
    try
    {
        _extdict_discount = 0;
    
        char *v;
        if((v = cheap_settings->value("extdict-discount")) != 0)
        {
            _extdict_discount = strtoint(v, "as value of extdict-discount");
        }

        char *extdictpath = cheap_settings->value("extdict-path");
        char *mappath = cheap_settings->value("extdict-mapping");
        if(extdictpath != 0 && mappath)
        {
            fprintf(fstatus, "\n");
            _extDict = new extDictionary(extdictpath, mappath);
        }
    }
    catch(tError &e)
    {
        fprintf(fstatus, "EXTDICT disabled: %s\n", e.msg().c_str());
        _extDict = 0;
    }
#endif

    get_unifier_stats();

    if(property("unfilling") == "true" && opt_packing)
    {
        fprintf(ferr, "warning: cannot using packing on unfilled grammar -"
                " packing disabled\n");
        opt_packing = 0;
    }

#ifdef LUI
    for(list<grammar_rule *>::iterator rule = _rules.begin();
        rule != _rules.end(); 
        ++rule)
      (*rule)->lui_dump();
#endif
}

void
tGrammar::undump_properties(dumper *f)
{
    if(verbosity > 4)
        fprintf(fstatus, " [");

    int nproperties = f->undump_int();
    for(int i = 0; i < nproperties; i++)
    {
        char *key, *val;
        key = f->undump_string();
        val = f->undump_string();
        _properties[key] = val;
        if(verbosity > 4)
            fprintf(fstatus, "%s%s=%s", i ? ", " : "", key, val);
        delete[] key;
        delete[] val;
    }

    if(verbosity > 4)
        fprintf(fstatus, "]");
}

string
tGrammar::property(string key)
{
    map<string, string>::iterator it = _properties.find(key);
    if (it != _properties.end())
        return it->second;
    else
        return string();
}

void
tGrammar::init_parameters()
{
    char *v;

    struct setting *set;
    if((set = cheap_settings->lookup("start-symbols")) != 0)
    {
        _root_insts = 0;
        for(int i = set->n-1; i >= 0; i--)
        {
            _root_insts = cons(lookup_type(set->values[i]), _root_insts);
            if(first(_root_insts) == -1)
                throw tError(string("undefined start symbol `") +
                            string(set->values[i]) + string("'"));
        }
    }
    else
        _root_insts = 0;

    if((v = cheap_settings->value("qc-structure-unif")) != 0
       || (v = cheap_settings->value("qc-structure")) != 0)
    {
        _qc_inst_unif = lookup_type(v);
        if(_qc_inst_unif == -1) _qc_inst_unif = 0;
    }
    else
        _qc_inst_unif = 0;

    if((v = cheap_settings->value("qc-structure-subs")) != 0
       || (v = cheap_settings->value("qc-structure")) != 0)
    {
        _qc_inst_subs = lookup_type(v);
        if(_qc_inst_subs == -1) _qc_inst_subs = 0;
    }
    else
        _qc_inst_subs = 0;

    // _fix_me_
    // the following two sections should be unified

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

    _packing_restrictor = 0;
    set = cheap_settings->lookup("packing-restrictor");
    if(set)
    {
      list< list_int * > del_paths;
      list_int *del_attrs = NULL;
      // if extended is true at the end of the loop, restrictor paths of
      // length > 1 have been specified
      bool extended = false;
      for(int i = 0; i < set->n; i++)
        {
          if((del_attrs = path_to_lpath(set->values[i])) != NULL) {
            // is there a path with length > 1 ??
            extended = extended || (rest(del_attrs) != NULL) ;
            del_paths.push_front(del_attrs);
          }
          else {
            fprintf(ferr, "ignoring path with unknown attribute `%s' "
                    "in packing_restrictor.\n",
                    set->values[i]);
          }
        }
      if (extended) {
        // use the extended iterator
        _packing_restrictor = new path_restrictor(del_paths);
      } else {
        // append the lists of length one into a single list for the simple
        // iterator
        del_attrs = NULL;
        for(list< list_int * >::iterator it = del_paths.begin()
              ; it != del_paths.end(); it++) {
          (*it)->next = del_attrs;
          del_attrs = *it;
        }
        _packing_restrictor = new list_int_restrictor(del_attrs);
        free_list(del_attrs);
      }
    }
    else 
    {
      char *restname = cheap_settings->value("dag-restrictor");
      if ((restname != NULL) && is_type(lookup_type(restname))) {
        _packing_restrictor = 
          new dag_restrictor(type_dag(lookup_type(restname)));
      }
    }
    if(opt_packing && (_packing_restrictor == NULL)) {
      fprintf(ferr, "\nWarning: packing enabled but no packing restrictor: "
              "packing disabled\n");
      opt_packing = 0;
    }
}

void
tGrammar::init_rule_qc_unif()
{
    for(rule_iter iter(this); iter.valid(); iter++)
    {
        grammar_rule *rule = iter.current();

        rule->init_qc_vector_unif();
    }
}

void
tGrammar::initialize_filter()
{
    fs_alloc_state S0;
    _filter = new char[_nrules * _nrules];
    _subsumption_filter = new char[_nrules * _nrules];

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
                list_int_restrictor restr(Grammar->deleted_daughters());
                
                if(arg == 1)
                {
                    bool forward = true, backward = false;
                    fs a = packing_partial_copy(daughter_fs, restr, false);
                    fs b = packing_partial_copy(mother_fs, restr, false);
                    
                    subsumes(a, b, forward, backward);
                    
                    _subsumption_filter[daughter->id() + _nrules * mother->id()] = forward;
                    
#ifdef DEBUG
                    fprintf(stderr, "SF %s %s %c\n",
                            daughter->printname(), mother->printname(),
                            forward ? 't' : 'f');
#endif
                }
                
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

tGrammar::~tGrammar()
{
    for(map<type_t, lex_stem *>::iterator pos = _lexicon.begin();
        pos != _lexicon.end(); ++pos)
        delete pos->second;

/*
// obsoleted by lexparser
#if 0
    for(ffdict::iterator pos = _fullforms.begin();
        pos != _fullforms.end(); ++pos)
        delete pos->second;
#endif
*/

    activate_all_rules();
    for(list<grammar_rule *>::iterator pos = _rules.begin();
        pos != _rules.end(); ++pos)
        delete *pos;

    if(_filter)
        delete[] _filter;

    if(_subsumption_filter)
        delete[] _subsumption_filter;

#ifdef ONLINEMORPH
    if(_morph)
        delete _morph;
#endif

    dag_qc_free();

#ifdef CONSTRAINT_CACHE
    free_constraint_cache(ntypes);
#endif

    free_list(_deleted_daughters);
    delete _packing_restrictor;
    free_type_tables();

#ifdef HAVE_EXTDICT
    clear_dynamic_stems();
#endif

    fs_alloc_state FSAS;
    FSAS.reset();
}

int
tGrammar::nhyperrules()
{
    int n = 0;
    for(rule_iter iter(this); iter.valid(); iter++)
        if(iter.current()->hyperactive()) n++;

    return n;
}

bool
tGrammar::root(fs &candidate, type_t &rule)
{
    list_int *r;
    for(r = _root_insts, rule = -1; r != 0; r = rest(r))
    {
        if(compatible(fs(first(r)), candidate))
        {
            rule = first(r);
            return true;
        }
    }
    return false;
}

lex_stem *
tGrammar::find_stem(type_t inst_key)
{
    map<type_t, lex_stem *>::iterator it = _lexicon.find(inst_key);
    if (it != _lexicon.end())
        return it->second;
    else
        return 0;
}

list<lex_stem *>
tGrammar::lookup_stem(string s)
{
    list<lex_stem *> results;
#ifdef HAVE_EXTDICT
    set<type_t> native_types;
#endif
  
    pair<multimap<string, lex_stem *>::iterator,
        multimap<string, lex_stem *>::iterator> eq =
        _stemlexicon.equal_range(s);
  
    for(multimap<string, lex_stem *>::iterator it = eq.first;
        it != eq.second; ++it)
    {
        results.push_back(it->second);
#ifdef HAVE_EXTDICT
        if(_extDict)
        {
            native_types.insert(_extDict->equiv_rep(leaftype_parent(it->second->type())));
        }
#endif
    }
  
#ifdef HAVE_EXTDICT
    if(!_extDict)
        return results;

    list<extDictMapEntry> extDictMapped;
    _extDict->getMapped(s, extDictMapped);

    if(verbosity > 2)
        fprintf(fstatus, "[EXTDICT] %s:", s.c_str());

    for(list<extDictMapEntry>::iterator it = extDictMapped.begin(); it != extDictMapped.end(); ++it)
    {
        type_t t = it->type();

        // Create stem if not blocked by entry from native lexicon.
        if(native_types.find(_extDict->equiv_rep(t)) != native_types.end())
        {
            if(verbosity > 2)
                fprintf(fstatus, " (%s)", type_name(t));
            continue;
        }
        else
        {
            if(verbosity > 2)
                fprintf(fstatus, " %s", type_name(t));
        }

        modlist mods;
        for(list<pair<string, string> >::const_iterator m = it->paths().begin();
            m != it->paths().end(); ++m)
        {
            type_t v = lookup_type(m->second.c_str());
            mods.push_back(make_pair(m->first, v));
        }

        list<string> orths;
        orths.push_back(s);

        lex_stem *st = new lex_stem(t, mods, orths, _extdict_discount);
        _dynamicstems.push_back(st);

        results.push_back(st);
    }

    if(verbosity > 2)
        fprintf(fstatus, "\n");
#endif

    return results;
}

#ifdef HAVE_EXTDICT
void
tGrammar::clear_dynamic_stems()
{
    for(list<lex_stem *>::iterator it = _dynamicstems.begin();
        it != _dynamicstems.end(); ++it)
        delete *it;

    _dynamicstems.clear();
}
#endif

/*
// obsoleted by lexparser
#if 0
list<full_form>
tGrammar::lookup_form(const string form)
{
    list<full_form> result;
  
#ifdef ONLINEMORPH
    if(opt_online_morph)
    {
        list<tMorphAnalysis> m = _morph->analyze(form);
        for(list<tMorphAnalysis>::iterator it = m.begin(); it != m.end(); ++it)
        {
            list<lex_stem *> stems = lookup_stem(it->base());
            for(list<lex_stem *>::iterator st_it = stems.begin();
                st_it != stems.end(); ++st_it)
                result.push_back(full_form(*st_it, *it));
        }
    }
#endif
    if(opt_fullform_morph)
    {
        pair<ffdict::iterator,ffdict::iterator> p = _fullforms.equal_range(form);
        for(ffdict::iterator it = p.first; it != p.second; ++it)
        {
            result.push_back(*it->second);
        }
    }
  
    return result;
}
#endif
*/

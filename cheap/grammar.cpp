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
#include <sys/stat.h>

#include "cheap.h"
#include "fs.h"
#include "fs-chart.h"
#include "chart-mapping.h"
#include "input-modules.h"
#include "parse.h"
#include "utility.h"
#include "dumper.h"
#include "grammar-dump.h"
#include "tsdb++.h"
#include "sm.h"
#include "restrictor.h"
#include "settings.h"
#include "configs.h"
#include "logging.h"
#ifdef HAVE_ICU
#include "unicode.h"
#endif

#include "item-printer.h"
#include "dagprinter.h"
#include <fstream>

using namespace std;

static int init();
static bool static_init = init();
static int init() {
  managed_opt("opt_filter",
              "Use the static rule filter (see Kiefer et al 1999)", true);
  managed_opt("opt_key",
              "What is the key daughter used in parsing?"
              "0: key-driven, 1: l-r, 2: r-l, 3: head-driven", (int) 0);
  managed_opt("opt_sm",
              "parse selection model (`null' for none)",
              std::string(""));
  managed_opt("opt_chart_pruning",
              "enables the chart pruning agenda",
              0);
  managed_opt("opt_chart_pruning_strategy",
              "determines the chart pruning strategy: 0=all tasks; 1=all successful tasks; 2=all passive items (default)",
              2);
  return true;
}

// defined in parse.cpp
extern int opt_packing;

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
inpmap_rule_status(type_t t)
{
  return cheap_settings->statusmember("token-mapping-rule-status-values",
        typestatus[t]);
}

bool
lexmap_rule_status(type_t t)
{
  return cheap_settings->statusmember("lexical-filtering-rule-status-values",
        typestatus[t]);
}

grammar_rule::grammar_rule(type_t t)
    : _tofill(0), _hyper(true), _spanningonly(false)
{
    _type = t;

    // _trait = INFL_TRAIT will be set by tLKBMorphology::undump_inflrs() later

    if(cheap_settings->statusmember("lexrule-status-values", typestatus[t]))
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

        ++n;
    }

    if(cheap_settings->lookup("rule-keyargs"))
    {
        if(keyarg != -1)
        {
          LOG(logGrammar, WARN,
              "warning: both keyarg-marker-path and rule-keyargs "
              "supply information on key argument...");
        }
        const char *s = cheap_settings->assoc("rule-keyargs", type_name(t));
        if(s && strtoint(s, "in `rule-keyargs'"))
        {
            keyarg = strtoint(s, "in `rule-keyargs'");
        }
    }

    _qc_vector_unif = new qc_vec[_arity];

    //
    // Build the _tofill list which determines the order in which arguments
    // are filled. The opt_key option determines the strategy:
    // 0: key-driven, 1: l-r, 2: r-l, 3: head-driven
    //

    // \todo
    // this is wrong for more than binary branching rules,
    // since adjacency is not guarantueed.

    int opt_key;
    get_opt("opt_key", opt_key);
    if(opt_key == 0) // take key arg specified in fs
    {
        if(keyarg != -1)
            _tofill = append(_tofill, keyarg);
    }
    else if(opt_key == 3) // take head daughter
    {
        if(head != -1)
            _tofill = append(_tofill, head);
        else if(keyarg != -1)
            _tofill = append(_tofill, keyarg);
    }

    if(opt_key != 2) // right to left, 2 is left to right
    {
        for(int i = 1; i <= _arity; ++i)
            if(!contains(_tofill, i)) _tofill = append(_tofill, i);
    }
    else
    {
        for(int i = _arity; i >= 1; i--)
            if(!contains(_tofill, i)) _tofill = append(_tofill, i);
    }

    for(int i = 0; i < _arity; ++i)
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

    LOG(logGrammar, DEBUG, *this);

}

// PCFG rule constructor
grammar_rule::grammar_rule(std::vector<type_t> v)
  : _tofill(0), _hyper(true), _spanningonly(false) {

  _type = v[0];
  _trait = PCFG_TRAIT;

  //
  // Determine arity, determine head and key daughters.
  //

  _arity = v.size() - 1;

  if(_arity == 0) {
    throw tError("Rule " + string(type_name(v[0])) + " has arity zero");
  }

  for(int i = 1; i <= _arity; ++i) {
    _tofill = append(_tofill, i);
    _pcfg_args.push_back(v[i]);
  }

}

grammar_rule::~grammar_rule() {
  free_list(_tofill);
  if (_trait != PCFG_TRAIT) {
    for (int i = 0; i < _arity; ++i) delete[] _qc_vector_unif[i];
    delete[] _qc_vector_unif;
  }
}

grammar_rule *
grammar_rule::make_grammar_rule(type_t t) {
  try {
    return new grammar_rule(t);
  }
  catch (tError e) {
    LOG(logGrammar, ERROR, e.getMessage());
  }
  return NULL;
}

grammar_rule *
grammar_rule::make_pcfg_grammar_rule(std::vector<type_t> v) {
  try {
    return new grammar_rule(v);
  }
  catch (tError e) {
    LOG(logGrammar, ERROR, e.getMessage());
  }
  return NULL;
}

void
grammar_rule::print(ostream &out) const {
  out << print_name(_type) << "/" << _arity;
  if (_hyper == false) out << "[-HA]" ;
  for (list_int *l = _tofill; l != NULL; l = rest(l))
    out << " " << first(l);
  out << ")";
}

void
grammar_rule::lui_dump(const char *path) {
  struct stat st;
  if(stat(path,&st) != 0) {
    LOG(logGrammar, ERROR,
        "grammar_rule::lui_dump(): invalid target directory `"
        << path << "'.)");
    return;
  } // if
  char name[MAXPATHLEN + 1];
  sprintf(name, "%s/rule.%d.lui", path, _id);
  ofstream stream(name);
  if(! stream) { //(stream = fopen(name, "w")) == NULL) {
    LOG(logGrammar, ERROR,
        "grammar_rule::lui_dump(): unable to open `" << name
        << "' (in `" << path << "').");
    return;
  } // if
  stream << "avm -" << _id << " ";
  LUIDagPrinter ldp;
  instantiate(true).print(stream, ldp);
  stream << " \"Rule # " << _id << " (" << printname() << ")\"\f\n";
  stream.close();
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
grammar_rule::init_qc_vector_unif() {
  fs_alloc_state FSAS;
  fs f = instantiate();
  for(int i = 1; i <= _arity; ++i) {
    fs arg = f.nth_arg(i);
    _qc_vector_unif[i-1] = arg.get_unif_qc_vector();
  }
}

/** pass T_BOTTOM for an invalid/unavailable qc structure */
void
undump_dags(dumper *f) {
  dag_node *dag;
  // allocate an array holding nstatictypes pointers to the typedags
  initialize_dags(nstatictypes);

#ifdef CONSTRAINT_CACHE
  // allocate an array holding nstatictypes pointers to typedag caches
  init_constraint_cache(nstatictypes);
#endif

  const char *v;
  type_t qc_inst_unif = T_BOTTOM;
  if((v = cheap_settings->value("qc-structure-unif")) != 0
     || (v = cheap_settings->value("qc-structure")) != 0)
    qc_inst_unif = lookup_type(v);
  if(get_opt_int("opt_nqc_unif") != 0 && qc_inst_unif == T_BOTTOM) {
    LOG(logAppl, INFO, "[ disabling unification quickcheck ] ");
    set_opt("opt_nqc_unif", 0);
  }

  type_t qc_inst_subs = T_BOTTOM;
  if((v = cheap_settings->value("qc-structure-subs")) != 0
     || (v = cheap_settings->value("qc-structure")) != 0)
    qc_inst_subs = lookup_type(v);
  if(get_opt_int("opt_nqc_subs") != 0 && qc_inst_subs == T_BOTTOM) {
    LOG(logAppl, INFO, "[ disabling subsumption quickcheck ] ");
    set_opt("opt_nqc_subs", 0);
  }

  for(int i = 0; i < nstatictypes; ++i) {
    if(i == qc_inst_unif) {
      LOG(logGrammar, DEBUG,
          "[qc unif structure `" << print_name(qc_inst_unif) << "'] ");
      fs::init_qc_unif(f, i == qc_inst_subs);
      dag = 0;
    }
    else if(i == qc_inst_subs) {
      LOG(logGrammar, DEBUG,
          "[qc subs structure `" << print_name(qc_inst_subs) << "'] ");
      fs::init_qc_subs(f);
      dag = 0;
    }
    else
      dag = dag_undump(f);

    register_dag(i, dag);
  }
}

// Construct a grammar object from binary representation in a file
tGrammar::tGrammar(const char * filename)
    : _properties(), _root_insts(0), _generics(0),
      _deleted_daughters(0), _packing_restrictor(0),
      _sm(0), _lexsm(0), _pcfgsm(0), _gm(0)
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
    const char *s = undump_header(&dmp, version);
    if(s) LOG(logApplC, INFO, "(" << s << ") ");
    delete[] s;

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

    // initialize cm-specific parameters:
    if ((get_opt<tokenizer_id>("opt_tok") == TOKENIZER_FSC)
        || get_opt_int("opt_chart_mapping"))
      tChartUtil::initialize();

    // constraints
    toc.goto_section(SEC_CONSTRAINTS);
    undump_dags(&dmp);
    initialize_maxapp();

    // Tell unifier that all dags created from now an are not considered to
    // be part of the grammar. This is needed for the smart copying algorithm.
    // _fix_me_
    // This only works when MARK_PERMANENT is defined. Otherwise, the decision
    // is hard-coded by using t_alloc or p_alloc. Ugly? Yes.
    stop_creating_permanent_dags();

    // find grammar rules and stems
    for(type_t i = 0; i < nstatictypes; ++i)
    {
        if(lexentry_status(i))
        {
            lex_stem *st = new lex_stem(i);
            _lexicon[i] = st;
            _stemlexicon.insert(make_pair(st->orth(st->inflpos()), st));
        }
        else if(rule_status(i))
        {
            grammar_rule *R = grammar_rule::make_grammar_rule(i);
            if (R != NULL) {
              _syn_rules.push_back(R);
              _rule_dict[i] = R;
            }
        }
        else if(lex_rule_status(i))
        {
            grammar_rule *R = grammar_rule::make_grammar_rule(i);
            if (R != NULL) {
              _lex_rules.push_back(R);
              _rule_dict[i] = R;
            }
        }
        else if (inpmap_rule_status(i) && (get_opt_int("opt_chart_mapping") > 0))
        {
            tChartMappingRule *R = tChartMappingRule::create(i,
                tChartMappingRule::TMR_TRAIT);
            if (R != NULL) {
              _tokmap_rules.push_back(R);
            }
        }
        else if (lexmap_rule_status(i) && (get_opt_int("opt_chart_mapping") > 0))
        {
            tChartMappingRule *R = tChartMappingRule::create(i,
                tChartMappingRule::LFR_TRAIT);
            if (R != NULL) {
              _lexfil_rules.push_back(R);
            }
        }
        else if(genle_status(i))
        {
            _generics = cons(i, _generics);
            _lexicon[i] = new lex_stem(i);
        }
        else if (predle_status(i)) {
          _predicts = cons(i, _predicts);
          _lexicon[i] = new lex_stem(i);
        }
    }

    int id = 0;
    // number the rules such that the lexical rules have the lower IDs
    for(ruleiter ru = _lex_rules.begin(); ru != _lex_rules.end(); ++ru)
      (*ru)->_id = id++;

    for(ruleiter ru = _syn_rules.begin(); ru != _syn_rules.end(); ++ru)
      (*ru)->_id = id++;

    /*
    // print the rule dictionary
    for (map<type_t, grammar_rule*>::iterator it = _rule_dict.begin();
         it != _rule_dict.end(); ++it) {
      printf("%d %s %s\n", (*it).first, print_name((*it).first)
             , type_name((*it).first));
    }
    */

    // Activate all rules for initialization etc.
    activate_all_rules();
    // The number of all rules for the unification and subsumption rule
    // filtering
    LOG(logGrammar, DEBUG, nstems() << "+" << length(_generics)
        << " stems, " << _rules.size() << " rules");


    for(ruleiter ri = _rules.begin(); ri != _rules.end(); ++ri)
      (*ri)->init_qc_vector_unif();

    // _filter.valid() will be false if not initialized
    if(get_opt_bool("opt_filter")) {
      initialize_filter();
    }

    //
    // avoid costly load of the various statistical models, when unneeded
    //
    if(get_opt_string("opt_preprocess_only").empty()) {
      //
      // a parse selection model can be supplied on the command line or through
      // the settings file.  and furthermore, even when a setting is present,
      // the command line can take precedence, including disabling parse
      // ranking by virtue of a special `null' model.
      //
      const std::string opt_sm = get_opt_string("opt_sm");
      if (opt_sm.empty() || opt_sm != "null") {
        const char *sm_file
        = (opt_sm.size() ? opt_sm.c_str() : cheap_settings->value("sm"));
        if(sm_file != 0) {
          // _fix_me_
          // Once we have more than just MEMs we will need to add a dispatch
          // facility here, or have a factory build the models.
          try { _sm = new tMEM(this, sm_file, filename); }
          catch(tError &e) {
            LOG(logGrammar, ERROR, e.getMessage());
            _sm = 0;
          }
        }
      } // if
      const char *pcfg_file = cheap_settings->value("pcfg");
      if (pcfg_file != 0) {
        try {
          _pcfgsm = new tPCFG(this, pcfg_file, filename);
          // delete pcfgsm;
          // only pcfg rules are loaded, not their weights
          // TODO: what was happening here?
        } catch (tError &e) {
          LOG(logGrammar, ERROR, e.getMessage());
          // LOG(logGrammar, ERROR, e.getMessage());
          _pcfgsm = 0;
        }
      }

      const char *gm_file = cheap_settings->value("gm");
      if (gm_file != 0) {
        try {
          _gm = new tGM(this, gm_file, filename);
        } catch (tError &e) {
          LOG(logGrammar, ERROR, e.getMessage());
          _gm = 0;
        }
      }

      const char *lexsm_file = cheap_settings->value("lexsm");
      if (lexsm_file != 0) {
        try { _lexsm = new tMEM(this, lexsm_file, filename); }
        catch(tError &e) {
          LOG(logGrammar, ERROR, e.getMessage());
          _lexsm = 0;
        }
      }
    } // if

    // check validity of cm-specific parameters:
    if ((get_opt<tokenizer_id>("opt_tok") == TOKENIZER_FSC)
        || get_opt_int("opt_chart_mapping"))
      tChartUtil::check_validity();

    get_unifier_stats(stats);

    if(property("unfilling") == "true" && opt_packing)
    {
      LOG(logGrammar, WARN,
            "cannot use packing on unfilled grammar - packing disabled");
      opt_packing = 0;
    }

#ifdef LUI
    for(ruleiter rule = _rules.begin(); rule != _rules.end(); ++rule)
      (*rule)->lui_dump();
#endif

}

void
tGrammar::undump_properties(dumper *f)
{
    LOG(logGrammar, DEBUG, '[');
    int nproperties = f->undump_int();
    for(int i = 0; i < nproperties; ++i)
    {
        char *key, *val;
        key = f->undump_string();
        val = f->undump_string();
        _properties[key] = val;
        LOG(logGrammar, DEBUG, (i ? ", " : "") << key << '=' << val);
        delete[] key;
        delete[] val;
    }

    LOG(logGrammar, DEBUG, "]");
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
    setting *set = cheap_settings->lookup("start-symbols");
    if(set != 0)
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


    // _fix_me_
    // the following two sections should be unified

    _deleted_daughters = 0;
    set = cheap_settings->lookup("deleted-daughters");
    if(set)
    {
        for(int i = 0; i < set->n; ++i)
        {
            int a = lookup_attr(set->values[i]);
            if(a != -1)
                _deleted_daughters = cons(a, _deleted_daughters);
            else
              LOG(logGrammar, WARN, "ignoring unknown attribute `"
                  << set->values[i] << "' in deleted_daughters.");
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
      for(int i = 0; i < set->n; ++i)
        {
          del_attrs = path_to_lpath(set->values[i]);
          if(del_attrs != NULL) {
            // is there a path with length > 1 ??
            extended = extended || (rest(del_attrs) != NULL) ;
            del_paths.push_front(del_attrs);
          }
          else {
            LOG(logGrammar, WARN, "ignoring path with unknown attribute `"
                << set->values[i] << "' in packing_restrictor.");
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
              ; it != del_paths.end(); ++it) {
          (*it)->next = del_attrs;
          del_attrs = *it;
        }
        _packing_restrictor = new list_int_restrictor(del_attrs);
        free_list(del_attrs);
      }
    }
    else
    {
      const char *restname = cheap_settings->value("dag-restrictor");
      if ((restname != NULL) && is_type(lookup_type(restname))) {
        _packing_restrictor =
          new dag_restrictor(type_dag(lookup_type(restname)));
        dag_restrictor::DEL_TYPE
          = lookup_type(cheap_settings->value("restrictor-delete"));
        dag_restrictor::ONLY_TYPE
          = lookup_type(cheap_settings->value("restrictor-only"));
      }
    }
    if(opt_packing && (_packing_restrictor == NULL)) {
      LOG(logGrammar, WARN,
          "packing enabled but no restrictor - packing disabled");
      opt_packing = 0;
    }
}

void
tGrammar::initialize_filter() {
  fs_alloc_state S0;
  int nrules = _rules.size();
  _filter.resize(nrules);
  _subsumption_filter.resize(nrules);

  for(ruleiter daughters = _rules.begin(); daughters != _rules.end();
      ++daughters) {
    fs_alloc_state S1;
    grammar_rule *daughter = *daughters;
    fs daughter_fs = copy(daughter->instantiate());

    for(ruleiter mothers = _rules.begin(); mothers != _rules.end();
        ++mothers) {
      grammar_rule *mother = *mothers;

      for(int arg = 1; arg <= mother->arity(); ++arg) {
        fs_alloc_state S2;
        fs mother_fs = mother->instantiate();
        list_int_restrictor restr(Grammar->deleted_daughters());

        if(arg == 1) {
          bool forward = true, backward = false;
          fs a = packing_partial_copy(daughter_fs, restr, false);
          fs b = packing_partial_copy(mother_fs, restr, false);

          subsumes(a, b, forward, backward);

          if(forward)
            _subsumption_filter.set(mother, daughter);

          /*
            fprintf(stderr, "SF %s %s %c\n",
            daughter->printname(), mother->printname(),
            forward ? 't' : 'f');
          */
        }

        fs arg_fs = mother_fs.nth_arg(arg);

        if(unify(mother_fs, daughter_fs, arg_fs).valid()) {
          _filter.set(mother, daughter, arg);
        }
      }
    }
  }

  S0.clear_stats();
}


tGrammar::~tGrammar()
{
#ifdef HAVE_ICU
    finalize_encoding_converter();
#endif
    for(map<type_t, lex_stem *>::iterator pos = _lexicon.begin();
        pos != _lexicon.end(); ++pos)
        delete pos->second;

    for(ruleiter pos = _syn_rules.begin(); pos != _syn_rules.end(); ++pos) {
      grammar_rule *r = *pos;
      delete r;
    }

    for(ruleiter pos = _lex_rules.begin(); pos != _lex_rules.end(); ++pos) {
      grammar_rule *r = *pos;
      delete r;
    }

    for(ruleiter pos = _pcfg_rules.begin(); pos != _pcfg_rules.end(); ++pos) {
      grammar_rule *r = *pos;
      delete r;
    }

    dag_qc_free();

    delete _sm;
    delete _pcfgsm;
    delete _lexsm;

#ifdef CONSTRAINT_CACHE
    free_constraint_cache(nstatictypes);
#endif

    free_list(_deleted_daughters);
    delete _packing_restrictor;
    free_type_tables();

    fs_alloc_state FSAS;
    FSAS.reset();
}

int
tGrammar::nhyperrules() {
  int n = 0;
  for(ruleiter iter = _rules.begin(); iter != _rules.end(); ++iter)
    if((*iter)->hyperactive()) ++n;

  return n;
}

bool
tGrammar::root(const fs &candidate, type_t &rule) const
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

bool
tGrammar::root(int type) const
{
  list_int *roots;
  for(roots = _root_insts; roots != 0; roots = rest(roots)) {
    if(first(roots) == type) return true;
  } // for
  return false;
} // tGrammar::root()

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

tGrammarUpdate::tGrammarUpdate(tGrammar *grammar, std::string &input)
  : _grammar(grammar), _original_roots(grammar->_root_insts), _update(0)
{

  if(!input.empty()) {
    _update = new settings(input);
    cheap_settings->install(_update);
    setting *set = _update->lookup("start-symbols");
    if(set != 0) {
      grammar->_root_insts = 0;
      for(int i = set->n-1; i >= 0; --i) {
        grammar->_root_insts
          = cons(lookup_type(set->values[i]), grammar->_root_insts);
        if(first(grammar->_root_insts) == -1)
          throw tError(string("undefined start symbol `")
                       + string(set->values[i]) + string("'"));
      } // for
    } // if
    else
      _original_roots = NULL;
  } // if
  else
    _original_roots = NULL;

} // tGrammarUpdate::tGrammarUpdate()


tGrammarUpdate::~tGrammarUpdate() {

  if(_original_roots != NULL) {
    free_list(_grammar->_root_insts);
    _grammar->_root_insts = _original_roots;
  } // if

  if(_update != 0) {
    cheap_settings->uninstall(_update);
    delete _update;
  } // if

} // tGrammarUpdate::~tGrammarUpdate()

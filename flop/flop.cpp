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

/* main module (preprocessor) */

#include <fcntl.h>
#include <errno.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "flop.h"
#include "hierarchy.h"
#include "settings.h"
#include "symtab.h"
#include "options.h"
#include "version.h"
#include "list-int.h"
#include "dag.h"
#include "utility.h"
#include "grammar-dump.h"

#include "pet-config.h"
#include "logging.h"
#include "configs.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using boost::lexical_cast;

/*** global variables ***/

const char * version_string = VERSION ;
const char * version_change_string = VERSION_CHANGE " " VERSION_DATETIME ;

symtab<Type *> types;
symtab<int> statustable;
symtab<Templ *> templates;
symtab<int> attributes;

string global_inflrs;
// Grammar properties - these are dumped into the binary representation
std::map<std::string, std::string> grammar_properties;

settings *flop_settings = 0;

/*** variables local to module */

static std::string grammar_version;

void
mem_checkpoint(const char *where)
{
    static size_t last = 0;
#ifndef WIN32
    size_t current = (size_t) sbrk(0);
    LOG(logAppl, DEBUG, 
        "Memory delta " << (current - last) / 1024 << "k (total "
        << current / 1024 << "k) [" << where << "]");

    last = current;
#endif
}

void
check_undefined_types()
{
  for(int i = 0; i < types.number(); i++)
    {
      if(types[i]->implicit)
        {
          Lex_location loc = types[i]->def;

          if(loc.fname.empty())
            loc = Lex_location("unknown", 0, 0);

          LOG(logSyntax, WARN, loc.fname << ":" << loc.linenr 
              << ": warning: type `" << types.name(i) << "' has no definition");
        }
    }
}

void process_conjunctive_subtype_constraints()
{
  Type *t;
  Conjunction *c;
  
  for(int i = 0; i<types.number(); i++)
    if((c = (t = types[i])->constraint) != 0)
      for(int j = 0; j < c->n(); j++)
        if( c->term[j]->tag == TYPE)
          {
            t->parents = cons(c->term[j]->type, t->parents);
            subtype_constraint(t->id, c->term[j]->type);
            c->term[j--] = c->term[c->n()-1];
            c->term.resize(c->n()-1);
          }
}

string synth_type_name(int offset = 0)
{
  string name = "synth" + lexical_cast<string>(types.number() + offset);

  if(types.id(name) != -1)
    return synth_type_name(offset + (int) (10.0*rand()/(RAND_MAX+1.0)));

  return name;
}

string typelist2string(list_int *l) {
  ostringstream out;
  for(; l != 0; l = rest(l)) out << " " << types.name(first(l));
  return out.str();
}

void process_multi_instances()
{
  int i, n = types.number();
  Type *t;

  map<list_int *, int, list_int_compare> ptype;

  for(i = 0; i < n; i++)
    if((t = types[i])->tdl_instance && length(t->parents) > 1)
      {
        LOG(logSemantic, DEBUG, "TDL instance `" << types.name(i) 
            << "' has multiple parents: " << typelist2string(t->parents));

        if(ptype[t->parents] == 0)
          {
            string name = synth_type_name();
            Type *p = new_type(name, false);
            p->def.assign("synthesized", 0, 0);
            p->parents = t->parents;
            
            for(list_int *l = t->parents; l != 0; l = rest(l))
              subtype_constraint(p->id, first(l));

            ptype[t->parents] = p->id;

            LOG(logSemantic, INFO,
                "Synthesizing new parent type `" << p->id << "'"); 
          }

        undo_subtype_constraints(t->id);
        subtype_constraint(t->id, ptype[t->parents]);
        
        t->parents = cons(ptype[t->parents], 0);
      }
}

/** The reorder_leaftypes functions fills the following arrays such that:
 *  - given a cheap type code i, cheap2flop[i] is the corresponding flop type
 *    code
 *  - given a flop type code j, flop2cheap[j] is the corresponding cheap type
 */
std::vector<int> cheap2flop;
std::vector<int> flop2cheap;

/**
 * Returns true iff type t is a proper type.
 */
inline bool
is_proper(int t) {
  return (leaftypeparent[t] == -1);
}

#include <stdio.h>
/**
 * Order all leaftypes in one consecutive range after all nonleaftypes.
 */
void reorder_leaftypes()
{
  cheap2flop.reserve(nstatictypes);
  for(int i = 0; i < nstatictypes; i++) {
    cheap2flop.push_back(i);
  }
  stable_partition(cheap2flop.begin(), cheap2flop.end(), is_proper);

  flop2cheap.resize(nstatictypes);
  for(int i = 0; i < nstatictypes; i++) {
    flop2cheap[cheap2flop[i]] = i;
  }
}

void assign_printnames()
{
  for(int i = 0; i < types.number(); i++)
    if(types[i]->printname.empty())
      types[i]->printname = types.name(i);
}

void preprocess_types()
{
  expand_templates();

  find_corefs();
  process_conjunctive_subtype_constraints();
  process_multi_instances();

  process_hierarchy(get_opt_bool("opt_propagate_status"));

  assign_printnames();
}

void log_types(const char *title)
{
  fprintf(stderr, "------ %s\n", title);
  for(int i = 0; i < types.number(); i++)
    {
      fprintf(stderr, "\n--- %s[%d]:\n", type_name(i), i);
      dag_print(stderr, types[i]->thedag);
    }
}

void demote_instances()
{
  // for TDL instances we want the parent's type in the fs
  for(int i = 0; i < types.number(); i++)
    {
      if(types[i]->tdl_instance)
        {
          // all instances should be leaftypes
          if(leaftypeparent[i] == -1)
            {
              fprintf(stderr, "warning: tdl instance `%s' is not a leaftype\n",
                      types.name(i).c_str());
              // assert(leaftypeparent[i] != -1);
            }
          else
            dag_set_type(types[i]->thedag, leaftypeparent[i]);
        }
    }
}

void process_types()
{
  LOG(logAppl, INFO, "- building dag representation");
  unify_wellformed = false;
  dagify_symtabs();
  dagify_types();
  reorder_leaftypes();

  if(LOG_ENABLED(logSemantic, DEBUG))
    log_types("after creation");

  if(!compute_appropriateness())
    {
      throw tError("non maximal introduction of features");
    }
  
  if(!apply_appropriateness())
    {
      throw tError("non well-formed feature structures");
    }

  LOG(logApplC, INFO, "- delta");
  if(!delta_expand_types())
    exit(1);

  LOG(logApplC, INFO, " / full");
  if(!fully_expand_types(get_opt_bool("opt_full_expansion")))
    exit(1);

  LOG(logAppl, INFO, " expansion for types");
  compute_maxapp();
  
  if(get_opt_bool("opt_unfill"))
    unfill_types();

  demote_instances();

  if(LOG_ENABLED(logSemantic, DEBUG))
    log_types("before dumping");

  compute_feat_sets(get_opt_bool("opt_minimal"));
}

void
fill_grammar_properties() {
  std::string s;
  std::ostringstream ss(s);

  grammar_properties["version"] = grammar_version;
    
  ss << templates.number();
  grammar_properties["ntemplates"] = ss.str();

  grammar_properties["unfilling"] =
    get_opt_bool("opt_unfill") ? "true" : "false";

  grammar_properties["full-expansion"] =
    get_opt_bool("opt_full_expansion") ? "true" : "false";
}

/*
void print_infls() {
  FILE *f = stdout;
  fprintf(f, ";; Morphological information\n");
  // find all infl rules 
  for(int i = 0; i < nstatictypes; i++) {
    if(types[i]->inflr != NULL) {
      //if(flop_settings->statusmember("infl-rule-status-values",
      //                                typestatus[i])) {
      fprintf(f, "%s:%d\n", type_name(flop2cheap[i])
              , typestatus[flop2cheap[i]]);
    }
  }
}
*/

void
print_morph_info(std::ostream &out)
{
    const char* path = flop_settings->value("morph-path");
    out << ";; Morphological information" << endl;
    // find all infl rules 
    for(int i = 0; i < nstatictypes; i++)
    {
        if(flop_settings->statusmember("infl-rule-status-values",
                                        typestatus[i]))
        {
            out << type_name(i) << ":" << std::endl;
            dag_node *dag = dag_copy(types[i]->thedag);
            
            if(dag != FAIL)
            {
                fully_expand(dag, true);
                dag_invalidate_visited(); 
            } 
            if(dag != FAIL)
                dag = dag_get_path_value(dag, path);

            if(dag != FAIL) 
              out << dag;
            else
              out << "(no structure under " << path << ")";
            out << std::endl;
        }
    }
    //    print_all_subleaftypes(f, lookup_type("gen-dat-val"));
}

string parse_version()
{
  string version;

  const char *fname_set = flop_settings->value("version-file");
  if(fname_set) {
    string fname = find_file(fname_set, SET_EXT);
    if(fname.empty()) return 0;

    push_file(fname, "reading");
    while(LA(0)->tag != T_EOF) {
      if(LA(0)->tag == T_ID 
         && flop_settings->member("version-string", LA(0)->text.c_str())) {
        consume(1);
        if(LA(0)->tag != T_STRING) {
          LOG(logSyntax, WARN, LA(0)->loc.fname << ":" <<  LA(0)->loc.linenr
              << ": warning: string expected for version");
        }
        else {
          version = LA(0)->text;
        }
      } 
      consume(1);
    }
    consume(1);
  }
  else {
    return 0;
  }

  return version;
}

void initialize_status()
{
  // NO_STATUS
  statustable.add("*none*");
  // ATOM_STATUS
  statustable.add("*atom*");
}

extern int dag_dump_grand_total_nodes, dag_dump_grand_total_atomic,
  dag_dump_grand_total_arcs;

#define SYNTAX_ERRORS 2
#define FILE_NOT_FOUND 3


int process(const std::string& ofname) {
  int res = 0;

  clock_t t_start = clock();

  mem_checkpoint("start");

  string fname = find_file(ofname, TDL_EXT);
  
  if(fname.empty()) {
    LOG(logSyntax, WARN,
        "warning: file `" << ofname << "' not found - skipping...");
    return FILE_NOT_FOUND ;
  }

  /* Initialize the builtin types with the topmost type in the hierarchy */
  BI_TOP = new_bi_type("*top*");

  flop_settings = new settings("flop", fname);

  grammar_version = parse_version();
  if(grammar_version.empty()) grammar_version = "unknown";

  bool pre_only = get_opt_bool("opt_pre");
  string outfname 
    = output_name(fname, TDL_EXT, pre_only ? PRE_EXT : GRAMMAR_EXT);
  FILE *outf = fopen(outfname.c_str(), "wb");
  
  if(outf) {
    setting *set;
    int i;

    initialize_specials(flop_settings);
    initialize_status();

    LOG(logAppl, INFO, std::endl << "converting `" << fname << 
        "' (" << grammar_version << ") into `" << outfname << "' ...");
      
    if((set = flop_settings->lookup("postload-files")) != 0)
      for(i = set->n() - 1; i >= 0; i--) {
        string fname = find_file(set->values[i], TDL_EXT);
        if(! fname.empty()) push_file(fname, "postloading");
      }
      
    push_file(fname, "loading");
      
    if((set = flop_settings->lookup("preload-files")) != 0)
      for(i = set->n() - 1; i >= 0; i--) {
        string fname = find_file(set->values[i], TDL_EXT);
        if(! fname.empty()) push_file(fname, "preloading");
      }
      
    mem_checkpoint("before parsing TDL files");

    tdl_start(1);

    mem_checkpoint("after parsing TDL files");
        
    const char *fffname = flop_settings->value("fullform-file");
    if(fffname != 0) {
      string fffnamestr = find_file(fffname, VOC_EXT);
      if(! fffnamestr.empty())
        read_morph(fffnamestr.c_str());
    }

    mem_checkpoint("after reading full form file");
        
    const char *irregfname = flop_settings->value("irregs-file");
    if(irregfname != 0) {
      string irregfnamestr = find_file(irregfname, IRR_EXT);
      if(! irregfnamestr.empty())
        read_irregs(irregfnamestr.c_str());
    }

    if(!pre_only)
      check_undefined_types();

    LOG(logAppl, INFO, std::endl << std::endl << "finished parsing - " 
        << syntax_errors << " syntax errors, "
        << total_lexed_lines << " lines in " << std::setprecision(3)
        << (clock() - t_start) / (float) CLOCKS_PER_SEC << " s");

    if (syntax_errors > 0) res = SYNTAX_ERRORS;

    mem_checkpoint("before preprocessing types");

    LOG(logAppl, INFO,
        "processing type constraints (" << types.number() << " types):");
      
    t_start = clock();

    //if(flop_settings->value("grammar-info") != 0)
    // create_grammar_info(flop_settings->value("grammar-info"),
    // grammar_version);
      
    preprocess_types();
    mem_checkpoint("after preprocessing types");

    if(!pre_only)
      process_types();

    mem_checkpoint("after processing types");

    fill_grammar_properties();        

    if(pre_only) {
      write_pre_header(outf, outfname, fname, grammar_version);
      write_pre(outf);
    } else {
      dumper dmp(outf, true);
      LOG(logApplC, INFO, "dumping grammar (");
      dump_grammar(&dmp, grammar_version.c_str());
      LOG(logAppl, INFO, ")");
      LOG(logAppl, DEBUG, dag_dump_grand_total_nodes << "[" 
          << dag_dump_grand_total_atomic << "]/"
          << dag_dump_grand_total_arcs << " (" << std::setprecision(2)
          << double(dag_dump_grand_total_arcs)/dag_dump_grand_total_atomic
          << "total grammar nodes [atoms]/arcs (ratio) dumped");
    }
      
    fclose(outf);
    
    LOG(logAppl, INFO, "finished conversion - output generated in " 
        << std::setprecision(3) 
        << (clock() - t_start) / (float) CLOCKS_PER_SEC << " s" << std::endl);

    if(get_opt_int("opt_cmi") > 0) {
      string moifile = output_name(fname, TDL_EXT, ".moi");
      std::ofstream moif(moifile.c_str());
      LOG(logAppl, INFO,
          "Extracting morphological information into `" << moifile << "'...");
      print_morph_info(moif);
      if(get_opt_int("opt_cmi") > 1) {
        LOG(logApplC, INFO, " type hierarchy...");
        moif << std::endl;
        print_hierarchy(moif);
      }
      moif.close();
    }
  }
  else {
    LOG(logAppl, WARN, "couldn't open output file `" << outfname << "' for `"
        << fname << "' - skipping...");
    res = FILE_NOT_FOUND;
  }
  return res;
}

void cleanup() {
}

void init() {
  managed_opt("opt_pre",
    "perform only the preprocessing stage (set local variable in fn process)",
    false);
  managed_opt("opt_cmi",
              "print information about morphological processing "
              "(different types depending on value)",  (int) 0);
  managed_opt("opt_full_expansion",
    "expand the feature structures fully to find possible inconsistencies",
    false);
  managed_opt("opt_unfill", "Remove dag nodes whose information is subsumed by the type feature structure of one of its enclosing nodes", false);
  managed_opt("opt_propagate_status", "", false);

}

int main(int argc, char* argv[])
{
  int retval = 0;
  // set up the streams for error and status reports

  setlocale(LC_ALL, "" );

  // initialization: global options
  init();
  // initialization: logging
  init_logging(argv[argc-1]);

  try {  
      std::string grammar_file_name = parse_options(argc, argv);
      if(grammar_file_name.empty())
      {
        cleanup(); exit(1);
      }

    //setup_io();
    
    retval = process(grammar_file_name);
  }

  catch(ConfigException &e)
    {
      LOG(logAppl, FATAL, e.getMessage());
      cleanup(); exit(1);
    }

  catch(tError &e)
    {
      LOG(logAppl, FATAL, e.getMessage());
      cleanup(); exit(1);
    }

  catch(bad_alloc)
    {
      LOG(logAppl, FATAL, "out of memory");
      cleanup(); exit(1);
    }
  catch(logic_error)
  {
      // wrong program options
      cleanup(); exit(1); 
  }
  catch(...)
    {
      LOG(logAppl, FATAL, "unknown exception");
      cleanup(); exit(1);
    }
  
  cleanup();
  return retval;
}

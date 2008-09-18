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

#if HAVE_LIBLOG4CXX
using namespace log4cxx;

const int logBufferSize = 65536;
char logBuffer[logBufferSize];

LoggerPtr loggerUncategorized = Logger::getLogger("uncategorized");
LoggerPtr loggerExpand = Logger::getLogger("expand");
LoggerPtr loggerFs = Logger::getLogger("fs");
LoggerPtr loggerGrammar = Logger::getLogger("grammar");
LoggerPtr loggerHierarchy = Logger::getLogger("hierarchy");
LoggerPtr loggerLexproc = Logger::getLogger("lexproc");
LoggerPtr loggerParse = Logger::getLogger("parse");
LoggerPtr loggerTsdb = Logger::getLogger("tsdb");

const int  defaultPbSize = 65536;
char defaultPb[defaultPbSize];

#endif // HAVE_LIBLOG4CXX

using namespace std;

/*** global variables ***/

const char * version_string = VERSION ;
const char * version_change_string = VERSION_CHANGE " " VERSION_DATETIME ;

symtab<struct type *> types;
symtab<int> statustable;
symtab<struct templ *> templates;
symtab<int> attributes;

char *global_inflrs = 0;
// Grammar properties - these are dumped into the binary representation
std::map<std::string, std::string> grammar_properties;

settings *flop_settings = 0;

/*** variables local to module */

static const char *grammar_version;

void
mem_checkpoint(const char *where)
{
    static size_t last = 0;

    size_t current = (size_t) sbrk(0);
    LOG(logAppl, DEBUG, 
        "Memory delta " << (current - last) / 1024 << "k (total "
        << current / 1024 << "k) [" << where << "]");

    last = current;
}

void
check_undefined_types()
{
  for(int i = 0; i < types.number(); i++)
    {
      if(types[i]->implicit)
        {
          lex_location *loc = types[i]->def;

          if(loc == 0)
            loc = new_location("unknown", 0, 0);

          LOG(logSyntax, WARN, loc->fname << ":" << loc->linenr 
              << ": warning: type `" << types.name(i) << "' has no definition");
        }
    }
}

void process_conjunctive_subtype_constraints()
{
  int i, j;
  struct type *t;
  struct conjunction *c;
  
  for(i = 0; i<types.number(); i++)
    if((c = (t = types[i])->constraint) != 0)
      for(j = 0; j < c->n; j++)
        if( c->term[j]->tag == TYPE)
          {
            t->parents = cons(c->term[j]->type, t->parents);
            subtype_constraint(t->id, c->term[j]->type);
            c->term[j--] = c->term[--c->n];
          }
}

char *synth_type_name(int offset = 0)
{
  char name[80];
  sprintf(name, "synth%d", types.number() + offset);

  if(types.id(name) != -1)
    return synth_type_name(offset + (int) (10.0*rand()/(RAND_MAX+1.0)));

  return strdup(name);
}

string typelist2string(list_int *l) {
  ostringstream out;
  for(; l != 0; l = rest(l)) out << " " << types.name(first(l));
  return out.str();
}

void process_multi_instances()
{
  int i, n = types.number();
  struct type *t;

  map<list_int *, int, list_int_compare> ptype;

  for(i = 0; i < n; i++)
    if((t = types[i])->tdl_instance && length(t->parents) > 1)
      {
        LOG(logSemantic, DEBUG, "TDL instance `" << types.name(i) 
            << "' has multiple parents: " << typelist2string(t->parents));

        if(ptype[t->parents] == 0)
          {
            char *name = synth_type_name();
            struct type *p = new_type(name, false);
            p->def = new_location("synthesized", 0, 0);
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

int *leaftype_order = 0;
int *rleaftype_order = 0;

void reorder_leaftypes()
// we want all leaftypes in one consecutive range after all nonleaftypes
{
  int i;

  leaftype_order = (int *) malloc(nstatictypes * sizeof(int));
  for(i = 0; i < nstatictypes; i++)
    leaftype_order[i] = i;

  int curr = nstatictypes - 1;

  for(i = 0; i < nstatictypes && i < curr; i++)
    {
      // look for next non-leaftype starting downwards from curr
      while(leaftypeparent[curr] != -1 && curr > i) curr--;

      if(leaftypeparent[i] != -1)
        {
          leaftype_order[i] = curr;
          leaftype_order[curr] = i;
          curr --;
        }
    }

  rleaftype_order = (int *) malloc(nstatictypes * sizeof(int));
  for(i = 0; i < nstatictypes; i++) rleaftype_order[i] = -1;

  for(i = 0; i < nstatictypes; i++) rleaftype_order[leaftype_order[i]] = i;

  for(i = 0; i < nstatictypes; i++)
    {
      if(rleaftype_order[i] == -1)
        {
          fprintf(stderr, "conception error in leaftype reordering\n");
          exit(1);
        }
      if(rleaftype_order[i] != leaftype_order[i])
        fprintf(stderr, "gotcha: %d: %d <-> %d", i, leaftype_order[i], rleaftype_order[i]);
    }
}

void assign_printnames()
{
  for(int i = 0; i < types.number(); i++)
    if(types[i]->printname == 0)
      types[i]->printname = strdup(types.name(i).c_str());
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

  LOG(logAppl, INFO, "- delta");
  if(!delta_expand_types())
    exit(1);

  LOG(logApplC, INFO, " / full");
  if(!fully_expand_types(get_opt_bool("opt_full_expansion")))
    exit(1);

  LOG(logApplC, INFO, " expansion for types");
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
      fprintf(f, "%s:%d\n", type_name(rleaftype_order[i])
              , typestatus[rleaftype_order[i]]);
    }
  }
}
*/

void
print_morph_info(FILE *f)
{
    char *path = flop_settings->value("morph-path");
    fprintf(f, ";; Morphological information\n");
    // find all infl rules 
    for(int i = 0; i < nstatictypes; i++)
    {
        if(flop_settings->statusmember("infl-rule-status-values",
                                        typestatus[i]))
        {
            fprintf(f, "%s:\n", type_name(i));
            dag_node *dag = dag_copy(types[i]->thedag);
            
            if(dag != FAIL)
            {
                fully_expand(dag, true);
                dag_invalidate_visited(); 
            } 
            if(dag != FAIL)
                dag = dag_get_path_value(dag, path);

            if(dag != FAIL) 
              dag_print(f, dag);
            else
              fprintf(f, "(no structure under %s)\n", path);
            fprintf(f, "\n");
        }
    }
    //    print_all_subleaftypes(f, lookup_type("gen-dat-val"));
}

char *parse_version() {
  char *version = 0;

  char *fname_set = flop_settings->value("version-file");
  if(fname_set) {
    string fname = find_file(fname_set, SET_EXT);
    if(fname.empty()) return 0;

    push_file(fname, "reading");
    while(LA(0)->tag != T_EOF) {
      if(LA(0)->tag == T_ID 
         && flop_settings->member("version-string", LA(0)->text)) {
        consume(1);
        if(LA(0)->tag != T_STRING) {
          LOG(logSyntax, WARN, LA(0)->loc->fname << ":" <<  LA(0)->loc->linenr
              << ": warning: string expected for version");
        }
        else {
          version = LA(0)->text; LA(0)->text = 0;
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


int process(char *ofname) {
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

  flop_settings = new settings("flop", fname.c_str());

  grammar_version = parse_version();
  if(grammar_version == 0) grammar_version = "unknown";

  string outfname = output_name(fname, TDL_EXT,
           get_opt_bool("opt_pre") ? PRE_EXT : GRAMMAR_EXT);
  FILE *outf = fopen(outfname.c_str(), "wb");
  
  if(outf) {
    struct setting *set;
    int i;

    initialize_specials(flop_settings);
    initialize_status();

    LOG(logAppl, INFO, "converting `" << fname << "' (" << grammar_version 
        << ") into `" << outfname << "' ...");
      
    if((set = flop_settings->lookup("postload-files")) != 0)
      for(i = set->n - 1; i >= 0; i--) {
        string fname = find_file(set->values[i], TDL_EXT);
        if(! fname.empty()) push_file(fname, "postloading");
      }
      
    push_file(fname, "loading");
      
    if((set = flop_settings->lookup("preload-files")) != 0)
      for(i = set->n - 1; i >= 0; i--) {
        string fname = find_file(set->values[i], TDL_EXT);
        if(! fname.empty()) push_file(fname, "preloading");
      }
      
    mem_checkpoint("before parsing TDL files");

    tdl_start(1);

    mem_checkpoint("after parsing TDL files");
        
    char *fffname;
    if((fffname = flop_settings->value("fullform-file")) != 0) {
      string fffnamestr = find_file(fffname, VOC_EXT);
      if(! fffnamestr.empty())
        read_morph(fffnamestr.c_str());
    }

    mem_checkpoint("after reading full form file");
        
    char *irregfname;
    if((irregfname = flop_settings->value("irregs-file")) != 0) {
      string irregfnamestr = find_file(irregfname, IRR_EXT);
      if(! irregfnamestr.empty())
        read_irregs(irregfnamestr.c_str());
    }

    if(!get_opt_bool("opt_pre"))
      check_undefined_types();

    LOG(logAppl, INFO, std::endl << "finished parsing - " 
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

    if(!get_opt_bool("opt_pre"))
      process_types();

    mem_checkpoint("after processing types");

    fill_grammar_properties();        

    if(get_opt_bool("opt_pre")) {
      write_pre_header(outf, outfname.c_str(), fname.c_str(), grammar_version);
      write_pre(outf);
    } else {
      dumper dmp(outf, true);
      LOG(logAppl, INFO, "dumping grammar (");
      dump_grammar(&dmp, grammar_version);
      LOG(logApplC, INFO, ")");
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
      FILE *moif = fopen(moifile.c_str(), "wb");
      LOG(logAppl, INFO,
          "Extracting morphological information into `" << moifile << "'...");
      print_morph_info(moif);
      if(get_opt_int("opt_cmi") > 1) {
        LOG(logAppl, INFO, " type hierarchy...");
        fprintf(moif, "\n");
        print_hierarchy(moif);
      }
      fclose(moif);
    }
  }
  else {
    LOG(logAppl, WARN, "couldn't open output file `" << outfname << "' for `"
        << fname << "' - skipping...");
    res = FILE_NOT_FOUND;
  }
  return res;
}

FILE *fstatus, *ferr;

void setup_io()
{
  // connect fstatus to fd 2, and ferr to fd errors_to, unless -1, 2 otherwise

  int val;

  val = fcntl(2, F_GETFL, 0);
  if(val < 0)
    {
      perror("setup_io() [status of fd 2]");
      exit(1);
    }

  fstatus = stderr;
  setvbuf(fstatus, 0, _IONBF, 0);

  if(errors_to != -1)
    {
      val = fcntl(errors_to, F_GETFL, 0);
      if(val < 0 && errno == EBADF)
        {
          ferr = stderr;
        }
      else
        {
          if(val < 0)
            {
              perror("setup_io() [status of fd errors_to]");
              exit(1);
            }
          if((val & O_ACCMODE) == O_RDONLY)
            {
              perror("setup_io(): fd errors_to is read only");
              exit(1);
            }
          ferr = fdopen(errors_to, "w");
        }
    }
  else
    ferr = stderr;

  setvbuf(ferr, 0, _IONBF, 0);
}

void cleanup() {
}

int main(int argc, char* argv[])
{
  int retval;
  // set up the streams for error and status reports

  ferr = fstatus = stderr; // preliminary setup

  setlocale(LC_ALL, "" );

  // initialization of log4cxx
#if HAVE_LIBLOG4CXX
  log4cxx::BasicConfigurator::resetConfiguration();
  log4cxx::PropertyConfigurator::configure(std::string("logging.conf"));
#endif // HAVE_LIBLOG4CXX

  if(!parse_options(argc, argv))
    {
      usage(stderr);
      cleanup(); exit(1);
    }

  setup_io();

  try { retval = process(grammar_file_name); }
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

  catch(...)
    {
      LOG(logAppl, FATAL, "unknown exception");
      cleanup(); exit(1);
    }
  
  cleanup();
  return retval;
}

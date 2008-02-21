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

/* main module (standalone parser) */

#include "pet-config.h"
#include "cheap.h"
#include "parse.h"
#include "chart.h"
#include "fs.h"
#include "tsdb++.h"
#include "grammar-dump.h"
#include "lexparser.h"
#include "morph.h"
#include "yy-tokenizer.h"
#include "lingo-tokenizer.h"
#ifdef HAVE_XML
#include "pic-tokenizer.h"
#include "smaf-tokenizer.h"
#endif
#include "item-printer.h"
#include "version.h"

#ifdef QC_PATH_COMP
#include "qc.h"
#endif

#ifdef YY
#include "yy.h"
#endif

#ifdef HAVE_ECL
#include "petecl.h"
#endif
#ifdef HAVE_MRS
#include "petmrs.h"
#include "cppbridge.h"
#endif
#ifdef HAVE_PREPROC
#include "eclpreprocessor.h"
#endif

#include <fstream>
#include <iostream>

#include "logging.h"

#ifdef HAVE_LIBLOG4CXX
using namespace log4cxx;
#endif // HAVE_LIBLOG4CXX

char * version_string = VERSION ;
char * version_change_string = VERSION_CHANGE " " VERSION_DATETIME ;

FILE *ferr, *fstatus, *flog;
#if HAVE_LIBLOG4CXX

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
LoggerPtr loggerXml = Logger::getLogger("xml");

const int  defaultPbSize = 65536;
char defaultPb[defaultPbSize];

#endif // HAVE_LIBLOG4CXX

// global variables for parsing

tGrammar *Grammar = 0;
settings *cheap_settings = 0;
bool XMLServices = false;


struct passive_weights : public unary_function< tItem *, unsigned int > {
  unsigned int operator()(tItem * i) {
    // return a weight close to infinity if this is not a phrasal item
    // thus, we guarantee that phrasal items are drastically preferred
    if (dynamic_cast<tPhrasalItem *>(i) != NULL) return 1;
    else if (dynamic_cast<tLexItem *>(i) != NULL) return 1000;
    else return 1000000;
  }
};

struct input_only_weights : public unary_function< tItem *, unsigned int > {
  unsigned int operator()(tItem * i) {
    // return a weight close to infinity if this is not an input item
    // prefer shorter items over longer ones
    if (dynamic_cast<tInputItem *>(i) != NULL)
      return i->span();
    else
      return 1000000;
  }
};

string get_surface_string(chart *ch) {
  list< tItem * > inputs;
  input_only_weights io;

  ch->shortest_path<unsigned int>(inputs, io, false);
  string surface;
  for(list<tItem *>::iterator it = inputs.begin()
        ; it != inputs.end(); it++) {
    tInputItem *inp = dynamic_cast<tInputItem *>(*it);
    if (inp != NULL) {
      surface = surface + inp->orth() + " ";
    }
  }

  int len = surface.length();
  if (len > 0) {
    surface.erase(surface.length() - 1);
  }
  return surface;
}


void dump_jxchg(string surface, chart *current) {
  if (! Config::get<std::string>("opt_jxchg_dir").empty()) {
    string yieldname = surface;
    replace(yieldname.begin(), yieldname.end(), ' ', '_');
    yieldname = Config::get<std::string>("opt_jxchg_dir") + yieldname;
    ofstream out(yieldname.c_str());
    if (! out) {
      LOG(loggerUncategorized, Level::WARN,
          "Can not open file %s", yieldname.c_str());
    } else {
      out << "0 " << current->rightmost() << endl;
      tJxchgPrinter chp(out);
      current->print(&chp);
    }
  }
}


void interactive() {
  string input;
  int id = 1;

  //tFegramedPrinter chp("/tmp/fed-");
  //chp.print(type_dag(lookup_type("quant-rel")));
  //exit(1);

  tTsdbDump tsdb_dump(Config::get<std::string>("opt_tsdb_dir"));
  if (tsdb_dump.active()) {
    Config::set("opt_tsdb", 1);
  } else {
    if (! Config::get<std::string>("opt_tsdb_dir").empty())
      LOG_ERROR(loggerUncategorized,
                "Could not open TSDB dump files in directory %s\n",
                Config::get<std::string>("opt_tsdb_dir").c_str());
  }

  while(!(input = read_line(stdin,
      Config::get<bool>("opt_comment_passthrough") ? 1 : 0)).empty()) {
    chart *Chart = 0;

    tsdb_dump.start();

    try {
      fs_alloc_state FSAS;

      list<tError> errors;
      analyze(input, Chart, FSAS, errors, id);
      if(!errors.empty())
        throw errors.front();
                
      if(verbosity == -1)
        fprintf(stdout, "%d\t%d\t%d\n",
                stats.id, stats.readings, stats.pedges);

      string surface = get_surface_string(Chart);

      printf("(%d) `%s' [%d] --- %d (%.2f|%.2fs) <%d:%d> (%.1fK) [%.1fs]\n",
             stats.id, surface.c_str(), 
             Config::get<int>("pedgelimit"), stats.readings, 
             stats.first/1000., stats.tcpu / 1000.,
             stats.words, stats.pedges, stats.dyn_bytes / 1024.0,
             TotalParseTime.elapsed_ts() / 10.);

      if(verbosity > 0) stats.print(fstatus);

      tsdb_dump.finish(Chart, surface);

      //tTclChartPrinter chp("/tmp/final-chart-bernie", 0);
      //tFegramedPrinter chp("/tmp/fed-");
      //Chart->print(&chp);

      //dump_jxchg(surface, Chart);

      if(verbosity > 1 || Config::get<char*>("opt_mrs")) {
        int nres = 0;
                
        list< tItem * > results(Chart->readings().begin()
                                , Chart->readings().end());
        // sorting was done already in parse_finish
        // results.sort(item_greater_than_score());
        for(list<tItem *>::iterator iter = results.begin()
              ; (iter != results.end())
              && ((Config::get<int>("opt_nresults") == 0)
                   || (Config::get<int>("opt_nresults") > nres))
              ; ++iter) {
          //tFegramedPrinter fedprint("/tmp/fed-");
          //tDelegateDerivationPrinter deriv(fstatus, fedprint);
          tCompactDerivationPrinter deriv(std::cerr);
          tItem *it = *iter;
                    
          nres++;
          fprintf(fstatus, "derivation[%d]", nres);
          fprintf(fstatus, " (%.4g)", it->score());
          fprintf(fstatus, ":%s", it->get_yield().c_str());
          fprintf(fstatus, "\n");
          if(verbosity > 2) {
            deriv.print(it);
            fprintf(fstatus, "\n");
          }
#ifdef HAVE_MRS
          if(Config::get<char*>("opt_mrs")) {
            string mrs;
            mrs = ecl_cpp_extract_mrs(it->get_fs().dag(),
                                      Config::get<char*>("opt_mrs"));
            if (mrs.empty()) {
              if (strcmp(Config::get<char*>("opt_mrs"), "xml") == 0)
                LOG(loggerUncategorized, Level::INFO,
                    "<rmrs cfrom='-2' cto='-2'>\n</rmrs>");
              else
                LOG(loggerUncategorized, Level::INFO, "No MRS");
            } else {
              LOG(loggerUncategorized, Level::INFO, "%s", mrs.c_str());
            }
          }
#endif
        }

#ifdef HAVE_MRS
        if(Config::get<bool>("opt_partial")
           && (Chart->readings().empty()))
        {
          list< tItem * > partials;
          passive_weights pass;
          Chart->shortest_path<unsigned int>(partials, pass, true);
          bool rmrs_xml = (strcmp(Config::get<char*>("opt_mrs"), "xml")
                           == 0);
          if (rmrs_xml)
            LOG(loggerUncategorized, Level::INFO, "<rmrs-list>");
          for(list<tItem *>::iterator it = partials.begin()
                ; it != partials.end(); ++it) {
            if(Config::get<char*>("opt_mrs")) {
              tPhrasalItem *item = dynamic_cast<tPhrasalItem *>(*it);
              if (item != NULL) {
                string mrs;
                mrs = ecl_cpp_extract_mrs(item->get_fs().dag(),
                                          Config::get<char*>("opt_mrs"));
                if (! mrs.empty()) {
                  LOG(loggerUncategorized, Level::INFO,
                      "%s", mrs.c_str());
                }
              }
            }
          }
          if (rmrs_xml)
            LOG(loggerUncategorized, Level::INFO, "</rmrs-list>");
          else 
            LOG(loggerUncategorized, Level::INFO, "EOM");
        }
#endif           
      }
      fflush(fstatus);
    } /* try */
        
    catch(tError e) {
      LOG_ERROR(loggerUncategorized, "%s", e.getMessage().c_str());
      if(verbosity > 0) stats.print(fstatus);
      stats.readings = -1;

      string surface = get_surface_string(Chart);
      dump_jxchg(surface, Chart);
      tsdb_dump.error(Chart, surface, e);

    }

    if(Chart != 0) delete Chart;

    id++;
  } /* while */

#ifdef QC_PATH_COMP
  if(Config::get<char *>("opt_compute_qc")) {
    FILE *qc = fopen(Config::get<char *>("opt_compute_qc"), "w");
    compute_qc_paths(qc);
    fclose(qc);
  }
#endif
}

void nbest() {
  string input;

  while(!feof(stdin)) {
    int id = 0;
    int selected = -1;
    int time = 0;
        
    while(!(input = read_line(stdin,
        Config::get<bool>("opt_comment_passthrough") ? 1 : 0)).empty()) {
      if(selected != -1)
        continue;

      chart *Chart = 0;
      try {
        fs_alloc_state FSAS;
                
        list<tError> errors;
        analyze(input, Chart, FSAS, errors, id);
        if(!errors.empty())
          throw errors.front();
                
        if(stats.readings > 0) {
          selected = id;
          fprintf(stdout, "[%d] %s\n", selected, input.c_str());
        }
                
        stats.print(fstatus);
        fflush(fstatus);
      } /* try */
            
      catch(tError e) {
        LOG_ERROR(loggerUncategorized, "%s", e.getMessage().c_str());
        stats.print(fstatus);
        fflush(fstatus);
        stats.readings = -1;
      }
            
      if(Chart != 0) delete Chart;
            
      time += stats.tcpu;
            
      id++;
    }
    if(selected == -1)
      fprintf(stdout, "[]\n");

    fflush(stdout);

    LOG(loggerUncategorized, Level::INFO, "ttcpu: %d", time);
  }
}

void interactive_morphology() {

  string input;
  bool comment_passthrough;
  Config::get("opt_comment_passthrough", comment_passthrough);
  while(!(input = read_line(stdin, comment_passthrough)).empty()) {
    timer clock;
    list<tMorphAnalysis> res = Lexparser.morph_analyze(input);
    
    for(list<tMorphAnalysis>::iterator it = res.begin(); 
        it != res.end(); 
        ++it) {
      fprintf(stdout, "%s\t", it->base().c_str());
      it->print_lkb(stdout);
      fprintf(stdout, "\n");
    } // for
    fprintf(fstatus,
            "\n%d chains in %0.2g s\n",
            res.size(), clock.convert2ms(clock.elapsed()) / 1000.);
  } // while

} // interactive_morphology()


void dump_glbs(FILE *f) {
  int i, j;
  for(i = 0; i < ntypes; i++) {
    prune_glbcache();
    for(j = 0; j < i; j++)
      if(glb(i,j) != -1) fprintf(f, "%d %d %d\n", i, j, glb(i,j));
  }
}

void print_symbol_tables(FILE *f) {
  fprintf(f, "type names (print names)\n");
  for(int i = 0; i < ntypes; i++) {
    fprintf(f, "%d\t%s (%s)\n", i, type_name(i), print_name(i));
  }

  fprintf(f, "attribute names\n");
  for(int i = 0; i < nattrs; i++) {
    fprintf(f, "%d\t%s\n", i, attrname[i]);
  }
}

void print_grammar(FILE *f) {
  if(verbosity > 10)
    dump_glbs(f);

  print_symbol_tables(f);
}


void process(const char *s) {
  timer t_start;
  
  try {
    string base = raw_name(s);
    cheap_settings = new settings(base.c_str(), s, "reading");
    fprintf(fstatus, "\n");
    LOG(loggerUncategorized, Level::INFO, "loading `%s' ", s);
    Grammar = new tGrammar(s); 
  }
  catch(tError &e) {
    LOG_FATAL(loggerUncategorized, "aborted\n%s",
              e.getMessage().c_str());
    delete cheap_settings;
    return;
  }

#ifdef HAVE_ECL
  char *cl_argv[] = {"cheap", 0};
  ecl_initialize(1, cl_argv);
  // make the redefinition warnings go away
  ecl_eval_sexpr("(setq cl-user::erroutsave cl-user::*error-output* "
                       "cl-user::*error-output* nil)");
#endif  // HAVE_ECL

  try {
#ifdef DYNAMIC_SYMBOLS
    init_characterization();
#endif
    Lexparser.init();

    dumper dmp(s);
    tFullformMorphology *ff = tFullformMorphology::create(dmp);
    if (ff != NULL) {
      Lexparser.register_morphology(ff);
      // ff->print(fstatus);
    }
    if(Config::get<bool>("opt_online_morph")) {
      tLKBMorphology *lkbm = tLKBMorphology::create(dmp);
      if (lkbm != NULL)
        Lexparser.register_morphology(lkbm);
      else
        Config::set("opt_online_morph", false);
    }
    Lexparser.register_lexicon(new tInternalLexicon());

    
    // TODO: this yells for a separate tokenizer factory
    tTokenizer *tok;
    switch (Config::get<tokenizer_id>("opt_tok")) {
    case TOKENIZER_YY: 
    case TOKENIZER_YY_COUNTS: 
      {
        char *classchar = cheap_settings->value("class-name-char");
        if (classchar != NULL)
          tok = new tYYTokenizer(
            (Config::get<tokenizer_id>("opt_tok") == TOKENIZER_YY_COUNTS
               ? STANDOFF_COUNTS : STANDOFF_POINTS), classchar[0]);
        else
          tok = new tYYTokenizer((
            Config::get<tokenizer_id>("opt_tok") == TOKENIZER_YY_COUNTS
              ? STANDOFF_COUNTS : STANDOFF_POINTS));
      }
      break;
    case TOKENIZER_STRING: 
    case TOKENIZER_INVALID: 
      tok = new tLingoTokenizer(); break;

    case TOKENIZER_PIC:
    case TOKENIZER_PIC_COUNTS: 
#ifdef HAVE_XML
      xml_initialize();
      XMLServices = true;
      tok = new tPICTokenizer((
        Config::get<tokenizer_id>("opt_tok") == TOKENIZER_PIC_COUNTS
        ? STANDOFF_COUNTS : STANDOFF_POINTS)); break;
#else
      LOG_FATAL(loggerUncategorized,
                "No XML input mode compiled into this cheap");
      exit(1);
#endif

    case TOKENIZER_FSR:
#ifdef HAVE_PREPROC
#ifdef HAVE_ICU
      tok = new tFSRTokenizer(s); break;
#endif
#else
      LOG_FATAL(loggerUncategorized, "No ecl/Lisp based FSR preprocessor "
                "compiled into this cheap");
      exit(1);
#endif

    case TOKENIZER_SMAF:
#ifdef HAVE_XML
#ifdef HAVE_ICU
      xml_initialize();
      XMLServices = true;
      tok = new tSMAFTokenizer(); break;
#else
      LOG_FATAL(loggerUncategorized,
               "No ICU (Unicode) support compiled into this cheap.");
      exit(1);
#endif
#else
      LOG_FATAL(loggerUncategorized,
                "No XML support compiled into this cheap.");
      exit(1);
#endif

    default:
      tok = new tLingoTokenizer(); break;
    }
    Lexparser.register_tokenizer(tok);
  }
    
  catch(tError &e) {
    LOG_FATAL(loggerUncategorized,
              "aborted\n%s", e.getMessage().c_str());
    delete Grammar;
    delete cheap_settings;
    return;
  }

#ifdef HAVE_MRS
  //
  // when requested, initialize the MRS variable property mapping from a file
  // specified in the grammar configuration.                   (24-aug-06; oe)
  //
  char *name = cheap_settings->value("vpm");
  if(name) {
    string file = find_file(name, ".vpm", s);
    mrs_initialize(s, file.c_str());
  } else mrs_initialize(s, NULL);

#endif
#ifdef HAVE_ECL
  // reset the error stream so warnings show up again
  ecl_eval_sexpr("(setq cl-user::*error-output* cl-user::erroutsave)");
#endif // HAVE_ECL

  LOG(loggerUncategorized, Level::INFO, "%d types in %0.2g s",
          ntypes, t_start.convert2ms(t_start.elapsed()) / 1000.);

  if(Config::get<bool>("opt_pg")) {
    print_grammar(stdout);
  }
  else {
    initialize_version();
        
#if defined(YY) && defined(SOCKET_INTERFACE)
    int server_mode;
    Config::get("opt_server", server_mode);
    if(server_mode != 0)
      cheap_server(server_mode);
    else 
#endif
#ifdef TSDBAPI
      if(Config::get<int>("opt_tsdb"))
        tsdb_mode();
      else
#endif
        {
          if(Config::get<bool>("opt_interactive_morph"))
            interactive_morphology();
          else if(Config::get<bool>("opt_nbest"))
            nbest();
          else
            interactive();
        }
  }

#ifdef HAVE_XML
  if (XMLServices) xml_finalize();
#endif
  delete Grammar;
  delete cheap_settings;
}


void main_init() {
  //2004/03/12 Eric Nichols <eric-n@is.naist.jp>: new option for input comments
  Config::addOption("opt_comment_passthrough",
    "Ignore/repeat input classified as comment: starts with '#' or '//'",
    false);
  Config::addOption("opt_tsdb",
    "enable [incr tsdb()] slave mode (protocol version = n)",
    0);
  Config::addOption("opt_tsdb_dir",
     "write [incr tsdb()] item, result and parse files to this directory",
     ((std::string) ""));
  Config::addOption("opt_server",
    "go into server mode, bind to port `n' (default: 4711)",
    0);
}

#ifdef __BORLANDC__
int real_main(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  ferr = stderr;
  fstatus = stderr;
  flog = (FILE *)NULL;

  setlocale(LC_ALL, "C" );
  
  // initialization of log4cxx
#if HAVE_LIBLOG4CXX
  BasicConfigurator::resetConfiguration();
  PropertyConfigurator::configure(std::string("logging.conf"));
#endif // HAVE_LIBLOG4CXX

  // Initialize global options
  main_init();

#ifndef __BORLANDC__
  if(!parse_options(argc, argv)) {
    usage(ferr);
    exit(1);
  }
#else
  if(argc > 1)
    grammar_file_name = argv[1];
  else
    grammar_file_name = "english";
#endif

#if defined(YY) && defined(SOCKET_INTERFACE)
  int server_mode;
  Config::get("opt_server", server_mode);
  if(server_mode != 0) {
    if(cheap_server_initialize(server_mode))
      exit(1);
  }
#endif

  string grammar_name = find_file(grammar_file_name, GRAMMAR_EXT);
  if(grammar_name.empty()) {
    LOG_FATAL(loggerUncategorized, "Grammar not found");
    exit(1);
  }

  try { process(grammar_name.c_str()); }

  catch(tError &e) {
    LOG_FATAL(loggerUncategorized, "%s", e.getMessage().c_str());
    exit(1);
  }

  catch(bad_alloc) {
    LOG_FATAL(loggerUncategorized, "out of memory");
    exit(1);
  }

  if(flog != NULL) fclose(flog);
  return 0;
}

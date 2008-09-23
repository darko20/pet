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

#include <iostream>

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
#include "mrs.h"
#include "vpm.h"
#include "qc.h"
#include "config.h"
#include "options.h" // verbosity and many other things

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

const char * version_string = VERSION ;
const char * version_change_string = VERSION_CHANGE " " VERSION_DATETIME ;

FILE *ferr, *fstatus, *flog;

// global variables for parsing

tGrammar *Grammar = 0;
settings *cheap_settings = 0;
bool XMLServices = false;
tVPM *vpm = 0;

struct passive_weights : public unary_function< tItem *, unsigned int > {
  unsigned int operator()(tItem * i) {
    // return a weight close to infinity if this is not a phrasal item
    // thus, we guarantee that phrasal items are drastically preferred
    if (dynamic_cast<tPhrasalItem *>(i) != NULL) return 1;
    else if (dynamic_cast<tLexItem *>(i) != NULL) return 1000;
    else return 1000000;
  }
};

void dump_jxchg(string surface, chart *current) {
  if (! get_opt_string("opt_jxchg_dir").empty()) {
    string yieldname = surface;
    replace(yieldname.begin(), yieldname.end(), ' ', '_');
    yieldname = get_opt_string("opt_jxchg_dir") + yieldname;
    ofstream out(yieldname.c_str());
    if (! out) {
      LOG(logAppl, WARN, "Can not open file " << yieldname);
    } else {
      out << "0 " << current->rightmost() << endl;
      tJxchgPrinter chp(out);
      current->print(out, &chp, true, false); // print only passive edges
    }
  }
}


void interactive() {
  string input;
  int id = 1;

  //tFegramedPrinter chp("/tmp/fed-");
  //chp.print(type_dag(lookup_type("quant-rel")));
  //exit(1);

  tTsdbDump tsdb_dump(get_opt_string("opt_tsdb_dir"));
  if (tsdb_dump.active()) {
    set_opt("opt_tsdb", 1);
  } else {
    if (! get_opt_string("opt_tsdb_dir").empty())
      LOG(logAppl, ERROR, "Could not open TSDB dump files in directory "
          << get_opt_string("opt_tsdb_dir"));
  }

  while(Lexparser.next_input(std::cin, input)) {
    chart *Chart = 0;

    tsdb_dump.start();

    try {
      fs_alloc_state FSAS;

      list<tError> errors;
      analyze(input, Chart, FSAS, errors, id);
      if(!errors.empty())
        throw errors.front();
                
      // TODO Who needs this? Can we remove it? (pead 01.04.2008)
      if(verbosity == -1)
        fprintf(stdout, "%d\t%d\t%d\n",
                stats.id, stats.readings, stats.pedges);

      string surface = Chart->get_surface_string();

      fprintf(fstatus, 
              "(%d) `%s' [%d] --- %d (%.2f|%.2fs) <%d:%d> (%.1fK) [%.1fs]\n",
              stats.id, surface.c_str(), 
              get_opt_int("opt_pedgelimit"), stats.readings, 
              stats.first/1000., stats.tcpu / 1000.,
              stats.words, stats.pedges, stats.dyn_bytes / 1024.0,
              TotalParseTime.elapsed_ts() / 10.);

      if(verbosity > 0) stats.print(fstatus);

      tsdb_dump.finish(Chart, surface);
      dump_jxchg(surface, Chart);
      
      //tTclChartPrinter chp("/tmp/final-chart-bernie", 0);
      //tFegramedPrinter chp("/tmp/fed-");
      //Chart->print(cerr, &chp, true, true);

      const char * opt_mrs = get_opt_string("opt_mrs").c_str();
      if (strlen(opt_mrs) == 0) opt_mrs = NULL;
      if(verbosity > 1 || opt_mrs) {
        int nres = 0;

        item_list results(Chart->readings().begin()
                                , Chart->readings().end());
        // sorting was done already in parse_finish
        // results.sort(item_greater_than_score());
        for(item_iter iter = results.begin();
            (iter != results.end())
              && ((get_opt_int("opt_nresults") == 0)
                   || (get_opt_int("opt_nresults") > nres))
              ; ++iter) {
          //tFegramedPrinter fedprint("/tmp/fed-");
          //tDelegateDerivationPrinter deriv(fstatus, fedprint);
          tCompactDerivationPrinter deriv(std::cerr);
          tItem *it = *iter;

          nres++;
          fprintf(fstatus, "derivation[%d]", nres);
          fprintf(fstatus, " (%.4g)", it->score());
          fprintf(fstatus, ":%s\n", it->get_yield().c_str());
          if(verbosity > 2) {
            deriv.print(it);
            fprintf(fstatus, "\n");
          }
#ifdef HAVE_MRS
          if(opt_mrs && (strcmp(opt_mrs, "new") != 0)) {
            string mrs;
            mrs = ecl_cpp_extract_mrs(it->get_fs().dag(), opt_mrs);
            if (mrs.empty()) {
              fprintf(fstatus, "\n%s\n" 
                      ((strcmp(opt_mrs, "rmrx") == 0)
                       ? "<rmrs cfrom='-2' cto='-2'>\n</rmrs>"
                       : "No MRS"));
            } else {
              fprintf(fstatus, "%s\n", mrs.c_str());
            }
          }
#endif
          if (opt_mrs && (strcmp(opt_mrs, "new") == 0)) {
            mrs::tPSOA* mrs = new mrs::tPSOA(it->get_fs().dag());
            if (mrs->valid()) {
              mrs::tPSOA* mapped_mrs = vpm->map_mrs(mrs, true); 
              if (mapped_mrs->valid()) {
                fprintf(fstatus, "\n");
                mapped_mrs->print(fstatus);
                fprintf(fstatus, "\n");
              }
              delete mapped_mrs;
            }
            delete mrs;
          }
        }

#ifdef HAVE_MRS
        if(get_opt_bool("opt_partial") && (Chart->readings().empty())) {
          list< tItem * > partials;
          passive_weights pass;
          Chart->shortest_path<unsigned int>(partials, pass, true);
          bool rmrs_xml = (opt_mrs != NULL && strcmp(opt_mrs, "rmrx") == 0);
          if (rmrs_xml) fprintf(fstatus, "\n<rmrs-list>\n");
          for(item_iter it = partials.begin(); it != partials.end(); ++it) {
            if(opt_mrs) {
              tPhrasalItem *item = dynamic_cast<tPhrasalItem *>(*it);
              if (item != NULL) {
                string mrs = ecl_cpp_extract_mrs(item->get_fs().dag(), opt_mrs);
                if (! mrs.empty()) {
                  fprintf(fstatus, "%s\n", mrs.c_str());
                }
              }
            }
          }
          if (rmrs_xml) fprintf(fstatus, "</rmrs-list>\n");
          else fprintf(fstatus, "EOM\n");
        }
#endif
      }
    } /* try */
        
    catch(tError e) {
      // shouldn't this be fstatus?? it's a "return value"
      fprintf(ferr, "%s\n", e.getMessage().c_str());
      if (verbosity > 0)
        stats.print(fstatus);
      stats.readings = -1;

      if (Chart != NULL) {
        string surface = Chart->get_surface_string();
        dump_jxchg(surface, Chart);
        tsdb_dump.error(Chart, surface, e);
      }
    }

    fflush(fstatus);

    if(Chart != 0) delete Chart;

    id++;
  } /* while */

  if(get_opt_charp("opt_compute_qc") != NULL) {
    ofstream qc(get_opt_charp("opt_compute_qc"));
    compute_qc_paths(qc);
  }
}

void interactive_morphology() {
  string input;

  while(Lexparser.next_input(std::cin, input)) {
    timer clock;
    list<tMorphAnalysis> res = Lexparser.morph_analyze(input);
    
    for(list<tMorphAnalysis>::iterator it = res.begin(); 
        it != res.end(); 
        ++it) {
      cout << it->base() << "\t";
      it->print_lkb(cout);
      cout << endl;
    } // for
    LOG(logAppl, INFO, endl << res.size() << " chains in "
        << std::setprecision(2) << clock.convert2ms(clock.elapsed()) / 1000.
        << " s\n");
  } // while

} // interactive_morphology()


void print_grammar(int what, ostream &out) {
  if(what == 1 || what == 4) {
    out << ";; TYPE NAMES (PRINT NAMES) ==========================" << endl;
    for(int i = 0; i < nstatictypes; i++) {
      out << i << "\t" << type_name(i) << " (" << print_name(i) << ")" << endl;
    }

    out << ";; ATTRIBUTE NAMES ===================================" << endl;
    for(int i = 0; i < nattrs; i++) {
      out << i << "\t" << attrname[i] << endl;
    }
  }
  
  out << ";; GLBs ================================================" << endl;
  if(what == 2 || what == 4) {
    int i, j;
    for(i = 0; i < nstatictypes; i++) {
      prune_glbcache();
      for(j = 0; j < i; j++)
        if(glb(i,j) != -1) out << i << ' ' << j << ' ' << glb(i,j) << endl;
    }
  }

  if(what == 3 || what == 4) {
    out << endl << " ;; TYPE DAGS ================================" << endl;
    ReadableDagPrinter dp;
    for(int i = 0; i < nstatictypes; i++) {
      out << '(' << i << ") " ;
      if (type_name(i)[0] == '$')
        out << "[" << type_name(i) << ']' << endl ;
      dp.print(out, type_dag(i));
      out << endl;
      // \todo this has to be replaced, but too much for now
      //dag_print_safe(f, type_dag(i), false, 0);
    }
  }
}

void cleanup() {
#ifdef HAVE_XML
  if (XMLServices) xml_finalize();
#endif
  delete Grammar;
  delete cheap_settings;
  delete vpm;
}

void process(const char *s) {
  timer t_start;
  
  try {
    cheap_settings = new settings(raw_name(s), s, "reading");
    LOG(logAppl, INFO, "loading `" << s << "' ");
    Grammar = new tGrammar(s);

#ifdef HAVE_ECL
    char *cl_argv[] = {"cheap", 0};
    ecl_initialize(1, cl_argv);
    // make the redefinition warnings go away
    ecl_eval_sexpr("(setq cl-user::erroutsave cl-user::*error-output* "
                   "cl-user::*error-output* nil)");
#endif  // HAVE_ECL

#ifdef DYNAMIC_SYMBOLS
    init_characterization();
#endif
    Lexparser.init();

    dumper dmp(s);
    tFullformMorphology *ff = tFullformMorphology::create(dmp);
    if (ff != NULL) {
      Lexparser.register_morphology(ff);
    }
    if(get_opt_bool("opt_online_morph")) {
      tLKBMorphology *lkbm = tLKBMorphology::create(dmp);
      if (lkbm != NULL)
        Lexparser.register_morphology(lkbm);
      else
        set_opt("opt_online_morph", false);
    }
    Lexparser.register_lexicon(new tInternalLexicon());

    
    // \todo this cries for a separate tokenizer factory
    tTokenizer *tok;
    switch (get_opt<tokenizer_id>("opt_tok")) {
    case TOKENIZER_YY: 
    case TOKENIZER_YY_COUNTS: 
      {
        char *classchar = cheap_settings->value("class-name-char");
        if (classchar != NULL)
          tok = new tYYTokenizer(
            (get_opt<tokenizer_id>("opt_tok") == TOKENIZER_YY_COUNTS
               ? STANDOFF_COUNTS : STANDOFF_POINTS), classchar[0]);
        else
          tok = new tYYTokenizer(
                                 (get_opt<tokenizer_id>("opt_tok") == TOKENIZER_YY_COUNTS
             ? STANDOFF_COUNTS : STANDOFF_POINTS));
      }
      break;

    case TOKENIZER_PIC:
    case TOKENIZER_PIC_COUNTS: 
#ifdef HAVE_XML
      xml_initialize();
      XMLServices = true;
      tok = new tPICTokenizer((get_opt<tokenizer_id>("opt_tok")
                               == TOKENIZER_PIC_COUNTS
                               ? STANDOFF_COUNTS : STANDOFF_POINTS));
      break;
#else
      LOG(logAppl, FATAL, "No XML input mode compiled into this cheap");
      exit(1);
#endif

    case TOKENIZER_FSR:
#ifdef HAVE_PREPROC
#ifdef HAVE_ICU
      tok = new tFSRTokenizer(s); break;
#endif
#else
      LOG(logAppl, FATAL,
          "No ecl/Lisp based FSR preprocessor compiled into this cheap");
      exit(1);
#endif

    case TOKENIZER_SMAF:
#ifdef HAVE_XML
#ifdef HAVE_ICU
      xml_initialize();
      XMLServices = true;
      tok = new tSMAFTokenizer(); break;
#else
      LOG(logAppl, FATAL, "No ICU (Unicode) support compiled into this cheap.");
      exit(1);
#endif
#else
      LOG(logAppl, FATAL, "No XML support compiled into this cheap.");
      exit(1);
#endif

    case TOKENIZER_INVALID: 
      LOG(logAppl, WARN, "unknown tokenizer mode \"" << optarg
          <<"\": using 'tok=string'");
    case TOKENIZER_STRING: 
    default:
      tok = new tLingoTokenizer(); break;
    }
    tok->set_comment_passthrough(get_opt_bool("opt_comment_passthrough"));
    Lexparser.register_tokenizer(tok);
  }
    
  catch(tError &e) {
    LOG(logAppl, FATAL, "aborted" << endl << e.getMessage());
    cleanup();
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

  vpm = new tVPM();
  char *name2 = cheap_settings->value("vpm");
  if (name2) {
    string file = find_file(name2, ".vpm", s);
    vpm->read_vpm(file);
  }

#ifdef HAVE_ECL
  // reset the error stream so warnings show up again
  ecl_eval_sexpr("(setq cl-user::*error-output* cl-user::erroutsave)");
#endif // HAVE_ECL

  LOG(logAppl, INFO, nstatictypes << " types in " << std::setprecision(2) 
      << t_start.convert2ms(t_start.elapsed()) / 1000. << " s" << endl);
  fflush(fstatus);

  if(get_opt_char("opt_pg") != '\0') {
    print_grammar(get_opt_char("opt_pg"), cout);
  }
  else {
    initialize_version();
        
#if defined(YY) && defined(SOCKET_INTERFACE)
    if(get_opt_int("opt_server") != 0)
      cheap_server(get_opt_int("opt_server"));
    else 
#endif
#ifdef TSDBAPI
      if(get_opt_int("opt_tsdb"))
        tsdb_mode();
      else
#endif
        {
          if(get_opt_bool("opt_interactive_morph"))
            interactive_morphology();
          else
            interactive();
        }
  }
  cleanup();
}


void main_init() {
  //2004/03/12 Eric Nichols <eric-n@is.naist.jp>: new option for input comments
  managed_opt("opt_comment_passthrough",
    "Ignore/repeat input classified as comment: starts with '#' or '//'",
    false);
  managed_opt("opt_tsdb",
    "enable [incr tsdb()] slave mode (protocol version = n)",
    0);
  managed_opt("opt_tsdb_dir",
     "write [incr tsdb()] item, result and parse files to this directory",
     ((std::string) ""));
  managed_opt("opt_server",
    "go into server mode, bind to port `n' (default: 4711)",
    0);
  managed_opt("opt_interactive_morph",
     "make cheap only run interactive morphology (only morphological rules, "
     "without lexicon)", false);
  managed_opt("opt_jxchg_dir",
              "write parse charts in jxchg format to the given directory",
              std::string());
  managed_opt("opt_mrs",
              "determines if and which kind of MRS output is generated. "
              "(modes: C implementation, LKB bindings via ECL; default: no)",
              std::string());
  
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
  try { 
    setlocale(LC_ALL, "C" );
    
    // initialization of logging
    init_logging(argv[argc-1]);

    // Initialize global options
    main_init();
    // Initialize options for the input modules
    tInputModule::init();
    
#ifndef __BORLANDC__
    if(!parse_options(argc, argv)) {
      usage(ferr);
      exit(1);
    }
#else
    grammar_file_name = "english"
    if(argc > 1)
      grammar_file_name = argv[1];
#endif
    
#if defined(YY) && defined(SOCKET_INTERFACE)
    if(get_opt_int("opt_server") != 0) {
      if(cheap_server_initialize(get_opt_int("opt_server")))
        exit(1);
    }
#endif
    
    string grammar_name = find_file(grammar_file_name, GRAMMAR_EXT);
    if(grammar_name.empty()) {
      throw tError("Grammar not found");
    }
    
    process(grammar_name.c_str()); 
  }
  catch(tError &e) {
    LOG(logAppl, FATAL, e.getMessage() << endl);
    exit(1);
  }

  catch(bad_alloc) {
    LOG(logAppl, FATAL, "out of memory");
    exit(1);
  }

  if(flog != NULL) fclose(flog);
  return 0;
}

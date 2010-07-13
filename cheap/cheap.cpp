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
#include "fsc-tokenizer.h"
#endif
#include "item-printer.h"
#include "version.h"
#include "mrs.h"
#include "mrs-printer.h"
#include "vpm.h"
#include "qc.h"
#include "pcfg.h"
#include "configs.h"
#include "options.h"
#include "settings.h"

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
#ifdef HAVE_XMLRPC_C
#include "server-xmlrpc.h"
#endif

#include <fstream>
#include <iostream>

#include "logging.h"

using namespace std;

const char * version_string = VERSION ;
const char * version_change_string = VERSION_CHANGE " " VERSION_DATETIME ;

FILE *ferr, *fstatus, *flog;

int verbosity = 0;

// global variables for parsing

tGrammar *Grammar = 0;
ParseNodes pn;
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

/** Dump \a dag onto \a f (external binary representation) */
extern bool dag_dump(dumper *f, dag_node *dag);

class ChartDumper : public tAbstractItemPrinter {
private:
  dumper *_dmp;

  virtual void print_common(const tItem * item, const std::string &name, dag_node *dag) {
    _dmp->dump_int(item->id());
    _dmp->dump_int(item->start());
    _dmp->dump_int(item->end());
    _dmp->dump_string(name);
    item_list dtrs = item->daughters();
    _dmp->dump_short(dtrs.size());
    for(item_citer it = dtrs.begin(); it != dtrs.end(); ++it) {
      _dmp->dump_int((*it)->id());
    }
    dag_dump(_dmp, dag);
  }

public:
  ChartDumper(dumper *dmp) : _dmp(dmp) {}

  virtual ~ChartDumper() {}

  /** The top level function called by the user */
  virtual void print(const tItem *arg) { arg->print_gen(this); }

  /** Base printer function for a tInputItem */
  virtual void real_print(const tInputItem *item) {
    print_common((const tItem *)item, (string) "\"" + item->form() + "\"",
      type_dag(BI_TOP));
  }
  /** Base printer function for a tLexItem */
  virtual void real_print(const tLexItem *item) {
    print_common((const tItem *)item,
      (item->rule() != NULL
      ? item->rule()->printname()
      : ((string) "\"" + item->printname() + "\"")),
      get_fs(item).dag());
  }

  /** Base printer function for a tPhrasalItem */
  virtual void real_print(const tPhrasalItem *item) {
    print_common((const tItem *)item, item->rule()->printname(),
      get_fs(item).dag());
  }
};

/** Dump the chart to a binary data file.
* \param surface the supposed surface string that was parsed
* \param ch the current chart
*/
void dump_jxchg_binary(string &surface, chart *ch) {
  if (! get_opt_string("opt_jxchg_dir").empty()) {
    string yieldname = surface;
    replace(yieldname.begin(), yieldname.end(), ' ', '_');
    yieldname = get_opt_string("opt_jxchg_dir") + yieldname;
    try {
      dumper dmp(yieldname.c_str(), true);

      dump_header(&dmp, surface.c_str());

      dump_toc_maker toc(&dmp);
      toc.add_section(SEC_CHART);
      toc.close();

      toc.start_section(SEC_CHART);
      dmp.dump_int(0);
      dmp.dump_int(ch->rightmost());
      ChartDumper chdmp(&dmp);
      ch->print(&chdmp, onlypassives);
      // logkb("edges", f);
      dmp.dump_int(0);
      dmp.dump_int(0);
      dmp.dump_int(0);

      toc.dump();
    }
    catch (tError err) {
      LOG(logAppl, WARN, "Can not open file " << yieldname);
    }
  }
}

void dump_jxchg_string(string surface, chart *current) {
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
      current->print(out, &chp, passive_no_inflrs); // print only passive edges
    }
  }
}

void dump_jxchg(string surface, chart *ch) {
  dump_jxchg_string(surface, ch);
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

  string infile = get_opt_string("opt_infile");
  ifstream ifs;
  ifs.open(infile.c_str());
  istream& lexinput = ifs ? ifs : std::cin;
  while(Lexparser.next_input(lexinput, input)) {
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

      //ofstream out("/tmp/final-chart-bernie");
      //tTclChartPrinter chp(out, 0);
      //tFegramedPrinter chp("/tmp/fed-");
      //Chart->print(out, &chp, true, true);

      const char * opt_mrs = get_opt_string("opt_mrs").c_str();
      if (strlen(opt_mrs) == 0) opt_mrs = NULL;
      if(verbosity > 1 || opt_mrs) {
        int nres = 0;

        item_list results(Chart->readings().begin()
          , Chart->readings().end());
        // sorting was done already in parse_finish
        // results.sort(item_greater_than_score());
        int opt_nresults;
        get_opt("opt_nresults", opt_nresults);
        for(item_iter iter = results.begin()
          ; (iter != results.end()
          && ((opt_nresults == 0) || (opt_nresults > nres)))
          ; ++iter) {
            //tFegramedPrinter baseprint("/tmp/fed-");
            //tLabelPrinter baseprint(pn) ;
            //tDelegateDerivationPrinter deriv(std::cerr, baseprint, 2);
            //tTSDBDerivationPrinter deriv(std::cerr, 1);
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
          if (opt_mrs && 
             ((strcmp(opt_mrs, "new") != 0) &&
              (strcmp(opt_mrs, "simple") != 0))) {
              string mrs;
            if(it->trait() != PCFG_TRAIT)
              mrs = ecl_cpp_extract_mrs(it->get_fs().dag(), opt_mrs);
              if (mrs.empty()) {
                fprintf(fstatus, "\n%s\n",
                  ((strcmp(opt_mrs, "rmrx") == 0)
                  ? "<rmrs cfrom='-2' cto='-2'>\n</rmrs>"
                  : "No MRS"));
              } else {
                fprintf(fstatus, "%s\n", mrs.c_str());
              }
            }
#endif
          if (opt_mrs && 
              ((strcmp(opt_mrs, "new") == 0) ||
               (strcmp(opt_mrs, "simple") == 0))) {
            fs f = it->get_fs();
            // if(it->trait() == PCFG_TRAIT) f = instantiate_robust(it);
            mrs::tPSOA* mrs = new mrs::tPSOA(f.dag());
              if (mrs->valid()) {
                mrs::tPSOA* mapped_mrs = vpm->map_mrs(mrs, true);
                if (mapped_mrs->valid()) {
                  std::cerr << std::endl;
                if (strcmp(opt_mrs, "new") == 0) {
                  MrxMRSPrinter ptr(cerr);
                  ptr.print(mapped_mrs);
                } else if (strcmp(opt_mrs, "simple") == 0) {
                  SimpleMRSPrinter ptr(cerr);
                  ptr.print(mapped_mrs);
                }
                //                mapped_mrs->print_simple(cerr);
                  std::cerr << std::endl;
                }
                delete mapped_mrs;
              }
              delete mrs;
            }
        }

        if(get_opt_bool("opt_partial") && (Chart->readings().empty())) {
          list< tItem * > partials;
          passive_weights pass;
          Chart->shortest_path<unsigned int>(partials, pass, true);
          bool rmrs_xml = (opt_mrs != NULL && strcmp(opt_mrs, "rmrx") == 0);
#ifdef HAVE_MRS
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
#endif
        }
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
    compute_qc_paths(qc, get_opt_int("opt_packing"));
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
  if(what == 's' || what == 'a') {
    out << ";; TYPE NAMES (PRINT NAMES) ==========================" << endl;
    for(int i = 0; i < nstatictypes; i++) {
      out << i << "\t" << type_name(i) << " (" << print_name(i) << ")" << endl;
    }

    out << ";; ATTRIBUTE NAMES ===================================" << endl;
    size_t nattrs = attrname.size();
    for(int i = 0; i < nattrs; i++) {
      out << i << "\t" << attrname[i] << endl;
    }
  }

  out << ";; GLBs ================================================" << endl;
  if(what == 'g' || what == 'a') {
    int i, j;
    for(i = 0; i < nstatictypes; i++) {
      prune_glbcache();
      for(j = 0; j < i; j++)
        if(glb(i,j) != -1) out << i << ' ' << j << ' ' << glb(i,j) << endl;
    }
  }

  if(what == 't' || what == 'a') {
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

  // initialize the server mode if requested:
#if defined(YY) && defined(SOCKET_INTERFACE)
  if(get_opt_int("opt_server") != 0) {
    if(cheap_server_initialize(get_opt_int("opt_server")))
      exit(1);
  }
#endif
#if defined(HAVE_XMLRPC_C) && !defined(SOCKET_INTERFACE)
  std::auto_ptr<tXMLRPCServer> server;
  if(get_opt_int("opt_server") != 0) {
    server = std::auto_ptr<tXMLRPCServer>(new tXMLRPCServer(get_opt_int("opt_server")));
  }
#endif

  try {
    cheap_settings = new settings(raw_name(s), s, "reading");
    LOG(logAppl, INFO, "loading `" << s << "' ");
    Grammar = new tGrammar(s);

#ifdef HAVE_ECL
    const char *cl_argv[] = {"cheap", 0};
    ecl_initialize(1, const_cast<char**>(cl_argv));
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
    tTokenizer *tok = 0;
    switch (get_opt<tokenizer_id>("opt_tok")) {
    case TOKENIZER_YY:
    case TOKENIZER_YY_COUNTS:
      {
        const char *classchar = cheap_settings->value("class-name-char");
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

    case TOKENIZER_FSC:
#ifdef HAVE_XML
      xml_initialize();
      XMLServices = true;
      tok = new tFSCTokenizer();
      break;
#else
      LOG(logAppl, FATAL, "No XML support compiled into this cheap.");
      exit(1);
#endif

    case TOKENIZER_INVALID:
      //LOG(logAppl, WARN, "unknown tokenizer mode \"" << optarg
      //    <<"\": using 'tok=string'");
      LOG(logAppl, WARN, "unknown tokenizer mode \"" <<"\": using 'tok=string'"); // FIXME
    case TOKENIZER_STRING:
    default:
      tok = new tLingoTokenizer(); break;
    }
    tok->set_comment_passthrough(get_opt_bool("opt_comment_passthrough"));
    Lexparser.register_tokenizer(tok);
  } catch(tError &e) {
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
  const char *name2 = cheap_settings->value("vpm");
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

    pn.initialize();

#if defined(YY) && defined(SOCKET_INTERFACE)
    if(get_opt_int("opt_server") != 0)
      cheap_server(get_opt_int("opt_server"));
    else
#endif
#if defined(HAVE_XMLRPC_C) && !defined(SOCKET_INTERFACE)
    if (get_opt_int("opt_server") != 0) {
      server->run();
    } else
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
  //2004/03/12 Eric Nichols <eric-n@is.naist.jp>: new option to allow for
  // input comments
  managed_opt("opt_comment_passthrough",
    "Ignore/repeat input classified as comment: starts with '#' or '//'",
    false);

  /** @name Operating modes
  * If none of the following options is provided, cheap will go into the usual
  * parsing mode.
  */
  //@{
  managed_opt("opt_tsdb",
    "enable [incr tsdb()] slave mode (protocol version = n)",
    0);
  managed_opt("opt_server",
    "go into server mode, bind to port `n' (default: 4711)",
    0);
  managed_opt("opt_pg",
    "print grammar in ASCII form, one of (s)ymbols (the default), (g)lbs "
    "(t)ype fs's or (a)ll", '\0');
  //@}

  managed_opt("opt_interactive_morph",
    "make cheap only run interactive morphology (only morphological rules, "
    "without lexicon)", false);

  managed_opt("opt_online_morph",
    "use the internal morphology (the regular expression style one)", true);

  managed_opt("opt_tsdb_dir",
    "write [incr tsdb()] item, result and parse files to this directory",
    ((std::string) ""));

  managed_opt("opt_infile",
    "read text input from this file",
    ((std::string) ""));

  managed_opt("opt_jxchg_dir",
    "write parse charts in jxchg format to the given directory",
    std::string());

  /** @name Output control */
  //@{
  managed_opt("opt_mrs",
    "determines if and which kind of MRS output is generated. "
    "(modes: C implementation, LKB bindings via ECL; default: no)",
    std::string());
  managed_opt("opt_nresults",
    "print at most n (full) results "
    "(should be the argument of an API function)", 0);
  /** print partial results in case of parse failure */
  managed_opt("opt_partial",
    "in case of parse failure, find a set of chart edges (partial results)"
    "that covers the chart in a good manner", false);
  //@}

  // \todo should go to yy.cpp, but this produces no code and the call is
  // optimized away
  managed_opt("opt_yy",
    "old shit that should be thrown out or properly reengineered and renamed.",
    false);
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

    string grammar_file_name;
    try {
      grammar_file_name = parse_options(argc, argv);
    }
    catch(logic_error)
    {
      // wrong program options
      exit(1); 
    }
    catch(...)
    {
      LOG(logAppl, FATAL, "unknown exception");
      exit(1);
    }

#if defined(YY) && defined(SOCKET_INTERFACE)
    if(get_opt_int("opt_server") != 0) {
      if(cheap_server_initialize(get_opt_int("opt_server")))
        exit(1);
    }
#endif

    if (grammar_file_name.empty()) {
      exit(1);
    }
    string grammar_name = find_file(grammar_file_name, GRAMMAR_EXT);
    if(grammar_name.empty()) {
      throw tError("Grammar not found");
    }

    process(grammar_name.c_str());
  } catch(tError &e) {
    LOG(logAppl, FATAL, e.getMessage() << endl);
    exit(1);
  } catch(bad_alloc) {
    LOG(logAppl, FATAL, "out of memory");
    exit(1);
  }

  if(flog != NULL) fclose(flog);
  return 0;
}


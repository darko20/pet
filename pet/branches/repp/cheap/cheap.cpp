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
#ifdef HAVE_BOOST_REGEX_ICU_HPP
#include "tRepp.h"
#endif
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
#include "qc.h"
#include "pcfg.h"
#include "configs.h"
#include "options.h"
#include "settings.h"

#include "api.h"

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
static bool XMLServices = false;

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

  virtual void print_common(const tItem * item, const string &name, dag_node *dag) {
    _dmp->dump_int(item->id());
    _dmp->dump_int(item->start());
    _dmp->dump_int(item->end());
    _dmp->dump_string(name.c_str());
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
  virtual void print(const tItem *arg) {
    arg->print_gen(this);
  }

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
      // print only passive unblocked edges
      current->print(out, &chp, passive_unblocked_non_input);
    }
  }
}

void dump_jxchg(string surface, chart *ch) {
  dump_jxchg_string(surface, ch);
}


void print_fs_as(char format, const fs &f, ostream &out) {
  dag_node *dag = f.dag();
  switch (format) {
  case 'r': { ReadableDagPrinter rfpr; rfpr.print(out, dag); break; }
  case 't': { ItsdbDagPrinter ifpr; ifpr.print(out, dag); break; }
  case 'j': { JxchgDagPrinter jfpr; jfpr.print(out, dag); break; }
  }
}

void print_derivation_as(char format, tItem *item, ostream &out) {
  switch (format) {
  case 'd': { tCompactDerivationPrinter cdpr(out); cdpr.print(item); break; }
  case 't': {
    int protocol_version = get_opt_int("opt_tsdb");
    if (protocol_version == 0)
      protocol_version = 1;
    tTSDBDerivationPrinter tdpr(out, protocol_version);
    tdpr.print(item);
    break;
  }
    // case 'x': { tXmlLabelPrinter xdpr(out); xdpr.print(item); break; }
  }
}

void print_result_as(string format, tItem *reading, ostream &out) {
  string::size_type dashloc = format.find('-');
  if (dashloc != string::npos) {
    char subformat = format.at(dashloc + 1);
    switch (format.at(0)) {
    case 'f': // print fs
      print_fs_as(subformat, reading->get_fs(), out);
      break;
    case 'd': // print derivation
      print_derivation_as(subformat, reading, out);
      break;
    case 'm': // print mrs
      print_mrs_as(subformat, reading->get_fs().dag(), out);
      break;
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

  string infile = get_opt_string("opt_infile");
  ifstream ifs;
  ifs.open(infile.c_str());
  istream& lexinput = ifs ? ifs : cin;
  while(Lexparser.next_input(lexinput, input)) {
    chart *Chart = 0;

    tsdb_dump.start();

    try {
      fs_alloc_state FSAS;

      list<tError> errors;
      analyze(input, Chart, FSAS, errors, id);
      if(!errors.empty())
        throw errors.front();

      /// \todo Who needs this? Can we remove it? (pead 01.04.2008)
      if(verbosity == -1)
        fprintf(stdout, "%d\t%d\t%d\n", stats.id, stats.readings, stats.pedges);

      string surface = Chart->get_surface_string();

      fprintf(fstatus,
              "(%d) `%s' [%d] --- %s%d (%.2f|%.2fs) <%d:%d> (%.1fK) [%.1fs]\n",
              stats.id, surface.c_str(),
              get_opt_int("opt_pedgelimit"),
              (stats.readings && stats.rreadings ? "*" : ""), stats.readings,
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
        int nres = 1;

        item_list results(Chart->readings().begin()
                                , Chart->readings().end());
        // sorting was done already in parse_finish
        // results.sort(item_greater_than_score());
        int opt_nresults;
        get_opt("opt_nresults", opt_nresults);
        for(item_iter iter = results.begin()
              ; (iter != results.end()
                 && ((opt_nresults == 0) || (opt_nresults >= nres)))
              ; ++iter, ++nres) {
          //tFegramedPrinter baseprint("/tmp/fed-");
          //tLabelPrinter baseprint(pn) ;
          //tDelegateDerivationPrinter deriv(cerr, baseprint, 2);
          //tTSDBDerivationPrinter deriv(cerr, 1);
          tCompactDerivationPrinter deriv(cerr);
          tItem *it = *iter;

          fprintf(fstatus, "derivation[%d]", nres);
          fprintf(fstatus, " (%.4g)", it->score());
          fprintf(fstatus, ":%s\n", it->get_yield().c_str());
          if(verbosity > 2) {
            deriv.print(it);
            fprintf(fstatus, "\n");
          }
          if (opt_mrs != NULL) {
            if ((strcmp(opt_mrs, "new") != 0)
                && (strcmp(opt_mrs, "simple") != 0)) {
#ifdef HAVE_MRS
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
#endif
            }
            else {
              print_mrs_as(opt_mrs[0], it->get_fs().dag(), cerr);
            }
          }
        }

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
#ifdef HAVE_MRS
                string mrs = ecl_cpp_extract_mrs(item->get_fs().dag(), opt_mrs);
                if (! mrs.empty()) {
                  fprintf(fstatus, "%s\n", mrs.c_str());
                }
#else
                if ((strcmp(opt_mrs, "new") == 0)
                    || (strcmp(opt_mrs, "simple") == 0)) {
                  print_mrs_as(opt_mrs[0], item->get_fs().dag(), cerr);
                }
#endif
              }
            }
          }
          if (rmrs_xml) fprintf(fstatus, "</rmrs-list>\n");
          else fprintf(fstatus, "EOM\n");
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

static void interactive_morphology() {
  string input;

  while(Lexparser.next_input(cin, input)) {
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
        << setprecision(2) << clock.convert2ms(clock.elapsed()) / 1000.
        << " s\n");
  } // while

} // interactive_morphology()

void print_grammar(int what, ostream &out) {
  if(what == 's' || what == 'a') {
    out << ";; TYPE NAMES (PRINT NAMES) ==========================" << endl;
    for(int i = 0; i < nstatictypes; ++i) {
      out << i << "\t" << type_name(i) << " (" << print_name(i) << ")" << endl;
    }

    out << ";; ATTRIBUTE NAMES ===================================" << endl;
    for(int i = 0; i < nattrs; ++i) {
      out << i << "\t" << attrname[i] << endl;
    }
  }

  out << ";; GLBs ================================================" << endl;
  if(what == 'g' || what == 'a') {
    int i, j;
    for(i = 0; i < nstatictypes; ++i) {
      prune_glbcache();
      for(j = 0; j < i; ++j)
        if(glb(i,j) != -1) out << i << ' ' << j << ' ' << glb(i,j) << endl;
    }
  }

  if(what == 't' || what == 'a') {
    out << endl << " ;; TYPE DAGS ================================" << endl;
    ReadableDagPrinter dp;
    for(int i = 0; i < nstatictypes; ++i) {
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

static void cleanup() {
#ifdef HAVE_XML
  if (XMLServices) xml_finalize();
#endif
  delete Grammar;
  delete cheap_settings;
  mrs_finalize();
}


/** Try to load the grammar specified by the given file name, and initialize
 *  all specified components that depend on it.
 *  \return \c false if everything went smoothly, \true in case of error
 */
bool load_grammar(string initial_name) {
  timer t_start;

  try {
    // the name of the .grm file (binary file generated by flop
    string grammar_file_name = find_file(initial_name, GRAMMAR_EXT);
    if(grammar_file_name.empty()) {
      LOG(logAppl, FATAL, "Grammar not found");
      return true;
    }

    cheap_settings = new settings(raw_name(grammar_file_name),
                                  grammar_file_name, "reading");
    LOG(logAppl, INFO, "loading `" << grammar_file_name << "' ");
    Grammar = new tGrammar(grammar_file_name.c_str());

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

    dumper dmp(grammar_file_name.c_str());
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
    case TOKENIZER_YY_COUNTS:
    case TOKENIZER_YY:
      {
        const char *classchar = cheap_settings->value("class-name-char");
        position_map what =
          get_opt<tokenizer_id>("opt_tok") == TOKENIZER_YY_COUNTS
          ? STANDOFF_COUNTS
          : STANDOFF_POINTS;
        if (classchar != NULL)
          tok = new tYYTokenizer(what, classchar[0]);
        else
          tok = new tYYTokenizer(what);
      }
      break;

	 case TOKENIZER_REPP:
#ifdef HAVE_BOOST_REGEX_ICU_HPP
    {
      string path;
      //have conf file, use to overlay options (not implemented yet) TODO
      string repp_opt = get_opt_string("opt_repp");
      if (!repp_opt.empty()) {  
        string reppfilename(find_file(repp_opt, SET_EXT, grammar_file_name));
        if (reppfilename.empty()) {
          cerr << "Couldn't find REPP conf file \"" 
            << dir_name(grammar_file_name) << repp_opt << "{" << SET_EXT
            << "}\"." << endl;
          exit(1);
        }
        path = dir_name(reppfilename);
      }
      else
        path = dir_name(grammar_file_name)+"rpp/";
      //path should be a setting, but for now, set here TODO
      tok = new tReppTokenizer(path);	
    }
    break;
#else
      LOG(logAppl, FATAL, 
			"No Unicode-aware regexp support compiled into this cheap.");
      return true;
#endif

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
      return true;
#endif

    case TOKENIZER_FSR:
#if defined(HAVE_PREPROC) && defined(HAVE_ICU)
      tok = new tFSRTokenizer(grammar_file_name.c_str()); break;
#else
      LOG(logAppl, FATAL,
          "No ecl/Lisp based FSR preprocessor compiled into this cheap");
      return true;
#endif

    case TOKENIZER_SMAF:
#if defined(HAVE_XML) && defined(HAVE_ICU)
      xml_initialize();
      XMLServices = true;
      tok = new tSMAFTokenizer(); break;
#else
      LOG(logAppl, FATAL,
          "No XML or ICU (Unicode) support compiled into this cheap.");
      return true;
#endif

    case TOKENIZER_FSC:
#ifdef HAVE_XML
      xml_initialize();
      XMLServices = true;
      tok = new tFSCTokenizer();
      break;
#else
      LOG(logAppl, FATAL, "No XML support compiled into this cheap.");
      return true;
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

#ifdef HAVE_MRS
    //
    // when requested, initialize the MRS variable property mapping from a file
    // specified in the grammar configuration.                   (24-aug-06; oe)
    //
    const char *name = cheap_settings->value("vpm");
    string file = (name != NULL
                   ? find_file(name, ".vpm", grammar_file_name)
                   : string());
    mrs_initialize(grammar_file_name.c_str(),
                   (file.empty() ? NULL : file.c_str()));
#endif

    mrs_init(grammar_file_name);

#ifdef HAVE_ECL
    // reset the error stream so warnings show up again
    ecl_eval_sexpr("(setq cl-user::*error-output* cl-user::erroutsave)");
#endif // HAVE_ECL

    initialize_version();

    pn.initialize();
  }
  catch(tError &e) {
    LOG(logAppl, FATAL, "aborted" << endl << e.getMessage());
    cleanup();
    return true;
  }
  catch(bad_alloc) {
    LOG(logAppl, FATAL, "out of memory");
    return true;
  }
  LOG(logAppl, INFO, nstatictypes << " types in " << setprecision(2)
      << t_start.convert2ms(t_start.elapsed()) / 1000. << " s" << endl);
  fflush(fstatus);

  return false;
}



/** If there's a server to start, this method will do it. All server modes have
 *  in common that the method will never return if a server has been started
 *  successfully.
 */
static void eventually_start_server(const char *s) {

  // initialize the server mode if requested:
#if defined(YY) && defined(SOCKET_INTERFACE)
  int opt_server = get_opt_int("opt_server");
  if(opt_server != 0) {
    if(cheap_server_initialize(opt_server))
      exit(1);
    load_grammar(s);
    cheap_server(opt_server);
  }
#endif

#if defined(HAVE_XMLRPC_C) && !defined(SOCKET_INTERFACE)
  int opt_server = get_opt_int("opt_server");
  auto_ptr<tXMLRPCServer> server;
  if(opt_server != 0) {
    server = auto_ptr<tXMLRPCServer>(new tXMLRPCServer(opt_server));
    load_grammar(s);
    server->run();
  }
#endif

#ifdef TSDBAPI
  if(get_opt_int("opt_tsdb")) {
    load_grammar(s);
    tsdb_mode();
  }
#endif
}


static void init_main_options() {
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
    string());

  managed_opt("opt_infile",
    "read text input from this file",
    string());

  managed_opt("opt_take", "use take processing mode: mrs|trees|both", string());

  managed_opt("opt_jxchg_dir",
              "write parse charts in jxchg format to the given directory",
              string());

  /** @name Output control */
  //@{
  managed_opt("opt_mrs",
              "determines if and which kind of MRS output is generated. "
              "(modes: C implementation, LKB bindings via ECL; default: no)",
              string());
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
	managed_opt("opt_repp",
		"Tokenize using REPP, with repp settings in the file argument", string());
}

/** general setup of globals, etc. */
const char *init(int argc, char* argv[], char *logging_dir) {
  timer t_start;

  ferr = stderr;
  fstatus = stderr;
  flog = (FILE *)NULL;

  setlocale(LC_ALL, "C" );

  // Initialize global options
  init_main_options();

  const char *grammar_file_name = parse_options(argc, argv);

  // initialization of logging
  init_logging(logging_dir == NULL ? grammar_file_name : logging_dir);

  return grammar_file_name;
}

/** clean up internal data structures, return zero if everything went smoothly
 */
int shutdown() {
  cleanup();

  if(flog != NULL) fclose(flog);
  return 0;
}

string massage_infilename(string inputfile, string suffix) {
  string::size_type lastdot = inputfile.rfind('.');
  if (lastdot != string::npos) {
    return inputfile.substr(0, lastdot) + "-" + suffix
      + inputfile.substr(lastdot);
  }
  return inputfile + "-" + suffix;
}

void take_process(const char *grammar_file_name) {
  // initialization
  load_grammar(grammar_file_name);

  char mode = get_opt_string("opt_take").at(0);

  // batch_process input
  string input;

  string infile = get_opt_string("opt_infile");
  ifstream ifs;
  ifs.open(infile.c_str());
  istream& lexinput = ifs ? ifs : cin;
  XmlLabelPrinter xlpr(pn);
  while(Lexparser.next_input(lexinput, input)) {
    int session_id = start_parse(input);
    int error_present = run_parser(session_id);
    if (error_present != RUNTIME_ERROR) {
      int result_no = results(session_id);
      int error_no = errors(session_id);

      fprintf(fstatus,
              "(%d) `%s' [%d] --- %s%d (%.2f|%.2fs) <%d:%d> (%.1fK) [%.1fs] %s\n",
              stats.id, input.c_str(),
              get_opt_int("opt_pedgelimit"),
              (stats.readings && stats.rreadings ? "*" : ""), stats.readings,
              stats.first/1000., stats.tcpu / 1000.,
              stats.words, stats.pedges, stats.dyn_bytes / 1024.0,
              TotalParseTime.elapsed_ts() / 10.,
              ((error_no > 0) ? get_error(session_id, 0).c_str() : ""));

      if (mode == 'm' || mode == 'b') {
        ofstream mrs_out(massage_infilename(input, "mrs").c_str());
        mrs_out << "<results tcpu=\"" << stats.tcpu / 1000.0 << "\">" << endl;
        for (int i = 1; i < result_no; ++i) {
          tItem *res = get_result_item(session_id, i);
          mrs_out << "<result nr=\"" << i << "\" score=\"" << res->score()
                  << "\">" << endl;
          print_mrs_as('n', res->get_fs().dag(), mrs_out);
          mrs_out << "</result>" << endl;
        }
        for (int i = 1; i < error_no; ++i) {
          string err = xml_escape(get_error(session_id, i));
          mrs_out << "<error>" << err << "</error>" << endl;
        }
        mrs_out << "</results>" << endl;
        mrs_out.close();
      }

      if (mode == 't' || mode == 'b') {
        ofstream tree_out(massage_infilename(input, "tree").c_str());
        tree_out << "<results tcpu=\"" << stats.tcpu / 1000.0 << "\">" << endl;
        for (int i = 1; i < result_no; ++i) {
          tItem *res = get_result_item(session_id, i);
          tree_out << "<result nr=\"" << i << "\" score=\"" << res->score()
                   << "\">" << endl;
          xlpr.print_to(tree_out, res);
          tree_out << "</result>" << endl;
        }
        for (int i = 1; i < error_no; ++i) {
          string err = xml_escape(get_error(session_id, i));
          tree_out << "<error>" << err << "</error>" << endl;
        }
        tree_out << "</results>" << endl;
        tree_out.close();
      }
    }
    else {
      fprintf(fstatus,
              "!%d! `%s' --- %s\n",
              stats.id, input.c_str(), get_error(session_id, 0).c_str());
    }
    end_parse(session_id);
  }
}

void process(const char *grammar_file_name) {
  if(get_opt_char("opt_pg") != '\0') {
    if (! load_grammar(grammar_file_name)) {
      print_grammar(get_opt_char("opt_pg"), cout);
    }
  } else {
    // will not return if a server was started
    eventually_start_server(grammar_file_name);

    load_grammar(grammar_file_name);
    if(get_opt_bool("opt_interactive_morph"))
      interactive_morphology();
    else
      interactive();
  }
}

int main(int argc, char* argv[]) {
  // initialization
  try {
     const char *grammar_file_name = init(argc, argv, argv[argc-1]);
     if (grammar_file_name == NULL) {
       LOG(logAppl, FATAL, "could not parse options: "
           "expecting grammar-file as last parameter");
       usage(stderr);
       exit(1);
     }
     if (! get_opt_string("opt_take").empty())
       take_process(grammar_file_name);
     else
       process(grammar_file_name);
  }
  catch (tError err) {
    cerr << err.getMessage();
    exit(1);
  }

  return shutdown();
}

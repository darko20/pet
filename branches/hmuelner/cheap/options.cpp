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

/* command line options */

#include "pet-config.h"
#include "options.h"
#include "settings.h"
#include "parse.h"
#include "yy.h"
#include "fs.h"
#include "cheap.h"
#include "utility.h"
#include "version.h"
#include "logging.h"
#include "configs.h"
#include "lexparser.h"

#include <string>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace po = boost::program_options;
using namespace std;

std::string parse_options(int argc, char* argv[])
{
  string filename;
  string inputfilename;
  int opt_tsdb = 1;
  int nsolutions = 1;
  string default_les("tradtional");
  string defaultOutputFile("/tmp/qc.tdl");
  int server = CHEAP_SERVER_PORT;
  int key = 0;
  int subs = 0;
  int pedgelimit = 0;
  int memlimit = 0;
  int timeout = 0;
  int packing = 15;
  string logfile;
  string pg;
  string mrs("simple");
  string tsdbdump;
  int results = 0;
  string tok;
  string jxchgdump;
  int comment_passthrough = 0;
  int predict_les = 1;
  string sm;
  int one_meaning = 1;
  po::options_description generic("generic options");
  generic.add_options()
    ("help", "produce help message")
#ifdef TSDBAPI
    ("tsdb", po::value<int>(&opt_tsdb)->zero_tokens(), "enable [incr tsdb()] slave mode (protocol version = n)")
#endif
    ("nsolutions", po::value<int>(&nsolutions)->implicit_value(1), "find best n only, 1 if n is not given")
    ("input-file", po::value<string>(&inputfilename), "name of text input file")
    ("verbose", po::value<int>(&verbosity)->default_value(0), "set verbosity level to n")
    ("limit", po::value<int>(&pedgelimit), "maximum number of passive edges")
    ("memlimit", po::value<int>(&memlimit), "maximum amount of fs memory (in MB)")
    ("timeout", po::value<int>(&timeout), "maximum time (in s) spent on analyzing a sentence")
    ("no-shrink-mem", "don't shrink process size after huge items")
    ("no-filter", "disable rule filter")
    ("qc-unif", po::value<int>(), "use only top n quickcheck paths (unification)")
    ("qc-subs", po::value<int>(&subs), "use only top n quickcheck paths (subsumption)")
    ("compute-qc", po::value<string>()->implicit_value(defaultOutputFile), "compute quickcheck paths (output to file)")
    ("compute-qc-unif", po::value<string>()->implicit_value(defaultOutputFile), "compute quickcheck paths only for unificaton (output to file)")
    ("compute-qc-subs", po::value<string>()->implicit_value(defaultOutputFile), "compute quickcheck paths only for subsumption (output to file)")
    ("mrs", po::value<string>(&mrs)->implicit_value("mrs"), "compute MRS semantics [mrs|mrx|rmrs|rmrx]") // TODO use a validate function
    ("key", po::value<int>(&key), "select key mode (0=key-driven, 1=l-r, 2=r-l, 3=head-driven)")
    ("no-hyper", "disable hyper-active parsing")
    ("no-derivation", "disable output of derivations")
    ("rulestats", "enable tsdb output of rule statistics")
    ("no-chart-man", "disable chart manipulation")
    ("default-les", po::value<string>(&default_les)->implicit_value("traditional"), "enable use of default lexical entries [=all|traditional]\n"
    "         * all: try to instantiate all default les for all input fs\n"
    "         * traditional (default): determine default les by posmapping for all lexical gaps")
    ("predict-les", po::value<int>(&predict_les)->implicit_value(1), "enable use of type predictor for lexical gaps")
    ("lattice", "word lattice parsing")
    ("no-online-morph", "no online morphology")
#ifdef YY
    ("server", po::value<int>(&server)->implicit_value(CHEAP_SERVER_PORT), "go into server mode, bind to port 'n'")
    ("one-meaning", po::value<int>(&one_meaning)->implicit_value(1), "non exhaustive search for first [nth] valid semantic formula")
    ("yy", "YY input mode (highly experimental)")
#endif
    ("failure-print", "print failure paths")
    ("interactive-online-morph", "morphology only")
    ("pg", po::value<string>(&pg), "print grammar in ASCII form ('s'ymbols, 'g'lbs, 't'ypes(fs), 'a'll)")
    ("packing", po::value<int>(&packing)->implicit_value(15), "set packing to n (bit coded; default: 15)")
    ("log", po::value<string>(&logfile), "log server mode activity to `file' (`+' appends)")
    ("tsdbdump", po::value<string>(&tsdbdump), "write [incr tsdb()] item, result and parse files to `directory'")
    ("jxchgdump", po::value<string>(&jxchgdump), "write jxchg/approximation chart files to `directory'")
    ("partial", "print partial results in case of parse failure")
    ("results", po::value<int>(&results), "print at most n (full) results")
    ("tok", po::value<string>(&tok), "select input method (string|fsr|yy|yy_counts|pic|pic_counts|smaf, default `string')")
    ("comment-passthrough", po::value<int>(&comment_passthrough), "allow input comments (-1 to suppress output)")
    ("sm", po::value<string>(&sm)->implicit_value("null"), "parse selection model ('null' for none)")
    ;

  po::options_description hidden("hidden options");
  hidden.add_options()
    ("grammar-file", po::value<std::string>(&filename), "grammar file")
    ;        

  po::positional_options_description p;
  p.add("grammar-file", -1);

  po::options_description cmdline_options;
  cmdline_options.add(generic).add(hidden);

  po::options_description visible("valid options are");
  visible.add(generic);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).
      options(cmdline_options).positional(p).run(), vm);
    po::notify(vm);
    if (vm.count("help")) {
      std::cout << visible << "\n";
      return "";
    }

    set_opt("opt_tsdb", opt_tsdb);
    set_opt("opt_nsolutions", nsolutions);
    int dles = 0;
    if (vm.count("default-les")>0) {
      string dl = vm["default-les"].as<string>();
      if (dl == "traditional") {
        dles = 1;
      } else if (dl == "all") {
        dles = 2;
      }
    }
    set_opt("opt_default_les", static_cast<default_les_strategy>(dles));
    set_opt("opt_chart_man", vm.count("no-chart-man")==0); 
    set_opt("opt_server", server);
    set_opt("opt_shrink_mem", vm.count("no-shrink-mem")==0);
    set_opt("opt_filter", vm.count("no-filter")==0);
    set_opt("opt_hyper", vm.count("no-hyper")==0);
    set_opt("opt_derivation", vm.count("no-derivation")==0);
    set_opt("opt_rulestatistics", vm.count("rulestats")>0);
    if (vm.count("compute-qc")>0) {
      set_opt("opt_compute_qc", vm["compute-qc"]);
      // TODO: this is maybe the first application for a callback option to
      // handle the three cases
      set_opt("opt_compute_qc_unif", true);
      set_opt("opt_compute_qc_subs", true);
    }
    if (vm.count("compute-qc-unif")>0) {
      set_opt("opt_compute_qc", vm["compute-qc-nif"]);
      set_opt("opt_compute_qc_unif", true);
    }
    if (vm.count("compute-qc-subs")>0) {
      set_opt("opt_compute_qc", vm["compute-qc-subs"]);
      set_opt("opt_compute_qc_subs", true);
    }
    set_opt("opt_print_failure", vm.count("failure-print")>0);
    if (vm.count("pg")>0) {
      set_opt("opt_pg", 's');
      if (pg.length()>0 && strchr("sgta", pg[0])) {
        set_opt("opt_pg", pg[0]);
      } else {
        LOG(logAppl, WARN,
          "Invalid argument to -pg, printing only symbols\n");
      }
    }
    set_opt("opt_interactive_morph", vm.count("interactive-online-morph")>0);
    set_opt("opt_lattice", vm.count("lattice")>0);
    if (vm.count("qc-unif")>0) {
      set_opt("opt_nqc_unif", vm["qc-unif"]);
    }
    if (vm.count("qc-subs")>0) {
      set_opt("opt_nqc_subs", subs);
    }
    if (vm.count("key")>0) {
      set_opt("opt_key", key);
    }
    if (vm.count("limit")>0) {
      set_opt("opt_pedgelimit", pedgelimit);
    }
    if (vm.count("memlimit")>0) {
      set_opt("opt_memlimit", memlimit);
    }
    if (vm.count("timeout")>0) {
      set_opt("opt_timeout", timeout);
    }
    if (vm.count("log")>0) {
      if(logfile[0] == '+') flog = fopen(&logfile[1], "a");
      else flog = fopen(logfile.c_str(), "w");
    }
    set_opt("opt_online_morph", vm.count("no-online-morph")==0);
    set_opt("opt_packing", packing);
    set_opt("opt_mrs", mrs);
    if (vm.count("tsdbdump") > 0) {
      set_opt("opt_tsdb_dir", tsdbdump);
    }
    if (vm.count("input-file") > 0) {
      set_opt("opt_infile", inputfilename);
    }
    set_opt("opt_partial", vm.count("partial")>0);
    if (results>0) {
      set_opt("opt_nresults", results);
    }
    if (vm.count("tok")>0) {
      set_opt_from_string("opt_tok", tok);
    }
    if (vm.count("jxchgdump")>0) {
      if (jxchgdump[jxchgdump.length() - 1] != '/')
        set_opt("opt_jxchg_dir", jxchgdump + "/");
      else
        set_opt("opt_jxchg_dir", jxchgdump);
    }
    if (vm.count("comment-passthrough")>0) {
      set_opt("opt_comment_passthrough", comment_passthrough);
    }
    if (vm.count("predict-les")>0) {
      set_opt("opt_predict_les", predict_les);
    }
    if (vm.count("sm")>0) {
      set_opt("opt_sm", sm);
    }
#ifdef YY
    if (vm.count("one-meaning")>0) {
      set_opt("opt_nth_meaning", one_meaning);
    }
    if (vm.count("yy")>0) {
      set_opt("opt_yy", true);
      set_opt_from_string("opt_tok", "yy");
    }
#endif

    if( get_opt_bool("opt_hyper") &&
      get_opt_charp("opt_compute_qc") != NULL)
    {
      LOG(logAppl, WARN,"quickcheck computation doesn't work "
        "in hyperactive mode, disabling.");
      set_opt("opt_hyper", false);
    }

  }
  catch (po::required_option) {
    throw std::logic_error("option error");
  }
  catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    throw std::logic_error("option error");
  }
  if (filename.empty()) {
    LOG(logAppl, FATAL, "parse_options(): expecting name of TDL grammar to process");
  }
  return filename;
}

bool bool_setting(settings *set, const std::string& s)
{
  string v = set->value(s.c_str());
  if(v.empty() || v == "0" || boost::algorithm::iequals(v, "false") ||
    boost::algorithm::iequals(v, "nil"))
    return false;

  return true;
}

int int_setting(settings *set, const char *s)
{
  string v = set->value(s);
  if(v.empty())
    return 0;
  int i = boost::lexical_cast<int>(v);
  return i;
}

void options_from_settings(settings *set) // TODO: unused
{
  if(bool_setting(set, "one-solution"))
    set_opt("opt_nsolutions", 1);
  else
    set_opt("opt_nsolutions", 0);
  verbosity = int_setting(set, "verbose");
  set_opt("pedgelimit", int_setting(set, "limit"));
  set_opt("memlimit", int_setting(set, "memlimit"));
  set_opt("opt_hyper", bool_setting(set, "hyper"));
  set_opt("opt_default_les", bool_setting(set, "default-les"));
  set_opt("opt_predict_les", int_setting(set, "predict-les"));
#ifdef YY
  set_opt("opt_nth_meaning", (bool_setting(set, "one-meaning")) ? 1 : 0);
#endif
}

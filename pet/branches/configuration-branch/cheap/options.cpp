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

#include "options.h"

#include "getopt.h"
#include "fs.h"
#include "cheap.h"
#include "utility.h"
#include "version.h"
#include "logging.h"
#include <string>


// opt_fullform_morph is obsolete
// bool opt_fullform_morph;

//defined in fs.cpp
extern bool opt_compute_qc_unif, opt_compute_qc_subs, opt_print_failure;

int opt_nqc_unif, opt_nqc_subs, verbosity;

char *grammar_file_name = 0;

void usage(FILE *f)
{
  fprintf(f, "cheap version %s\n", version_string);
  fprintf(f, "usage: `cheap [options] grammar-file'; valid options are:\n");
#ifdef TSDBAPI
  fprintf(f, "  `-tsdb[=n]' --- "
          "enable [incr tsdb()] slave mode (protocol version = n)\n");
#endif
  fprintf(f, "  `-nsolutions[=n]' --- find best n only, 1 if n is not given\n");
  fprintf(f, "  `-verbose[=n]' --- set verbosity level to n\n");
  fprintf(f, "  `-limit=n' --- maximum number of passive edges\n");
  fprintf(f, "  `-memlimit=n' --- maximum amount of fs memory (in MB)\n");
  fprintf(f, "  `-no-shrink-mem' --- don't shrink process size after huge items\n"); 
  fprintf(f, "  `-no-filter' --- disable rule filter\n"); 
  fprintf(f, "  `-qc-unif=n' --- use only top n quickcheck paths (unification)\n");
  fprintf(f, "  `-qc-subs=n' --- use only top n quickcheck paths (subsumption)\n");
  fprintf(f, "  `-compute-qc[=file]' --- compute quickcheck paths (output to file,\n"
             "                           default /tmp/qc.tdl)\n");
  fprintf(f, "  `-compute-qc-unif[=file]' --- compute quickcheck paths only for unificaton (output to file,\n"
             "                           default /tmp/qc.tdl)\n");
  fprintf(f, "  `-compute-qc-subs[=file]' --- compute quickcheck paths only for subsumption (output to file,\n"
             "                           default /tmp/qc.tdl)\n");
  fprintf(f, "  `-mrs[=mrs|mrx|rmrs|rmrx]' --- compute MRS semantics\n");
  fprintf(f, "  `-key=n' --- select key mode (0=key-driven, 1=l-r, 2=r-l, 3=head-driven)\n");
  fprintf(f, "  `-no-hyper' --- disable hyper-active parsing\n");
  fprintf(f, "  `-no-derivation' --- disable output of derivations\n");
  fprintf(f, "  `-rulestats' --- enable tsdb output of rule statistics\n");
  fprintf(f, "  `-no-chart-man' --- disable chart manipulation\n");
  fprintf(f, "  `-default-les' --- enable use of default lexical entries\n");
  fprintf(f, "  `-predict-les' --- enable use of type predictor for lexical gaps\n");
  fprintf(f, "  `-lattice' --- word lattice parsing\n");
#ifdef YY
  fprintf(f, "  `-server[=n]' --- go into server mode, bind to port `n' (default: 4711)\n");
  fprintf(f, "  `-k2y[=n]' --- "
          "output K2Y, filter at `n' %% of raw atoms (default: 50)\n");
  fprintf(f, "  `-k2y-segregation' --- "
          "pre-nominal modifiers in analogy to reduced relatives\n");
  fprintf(f, "  `-one-meaning[=n]' --- non exhaustive search for first [nth]\n"
             "                         valid semantic formula\n");
  fprintf(f, "  `-yy' --- YY input mode (highly experimental)\n");
#endif
  fprintf(f, "  `-failure-print' --- print failure paths\n");
  fprintf(f, "  `-interactive-online-morph' --- morphology only\n");
#ifdef ONLINEMORPH
  fprintf(f, "  `-no-online-morph' --- disable online morphology\n");
#endif
  fprintf(f, "  `-no-fullform-morph' --- disable full form morphology\n");
  fprintf(f, "  `-pg' --- print grammar in ASCII form\n");
  fprintf(f, "  `-nbest' --- n-best parsing mode\n");
  fprintf(f, "  `-packing[=n]' --- "
          "set packing to n (bit coded; default: 15)\n");
  fprintf(f, "  `-log=[+]file' --- "
             "log server mode activity to `file' (`+' appends)\n");
  fprintf(f, "  `-tsdbdump directory' --- "
             "write [incr tsdb()] item, result and parse files to `directory'\n");
  fprintf(f, "  `-jxchgdump directory' --- "
             "write jxchg/approximation chart files to `directory'\n");
  fprintf(f, "  `-partial' --- "
             "print partial results in case of parse failure\n");  
  fprintf(f, "  `-results=n' --- print at most n (full) results\n");  
  fprintf(f, "  `-tok=string|fsr|yy|yy_counts|pic|pic_counts|smaf' --- "
             "select input method (default `string')\n");  

  fprintf(f, "  `-comment-passthrough[=1]' --- "
          "allow input comments (-1 to suppress output)\n");
}

#define OPTION_TSDB 0
#define OPTION_NSOLUTIONS 1
#define OPTION_VERBOSE 2
#define OPTION_LIMIT 3
#define OPTION_NO_SHRINK_MEM 4
#define OPTION_NO_FILTER 5
#define OPTION_NQC_UNIF 6
#define OPTION_COMPUTE_QC 7
#define OPTION_PRINT_FAILURE 8
#define OPTION_KEY 9
#define OPTION_NO_HYPER 10
#define OPTION_NO_DERIVATION 11
#define OPTION_SERVER 12
#define OPTION_DEFAULT_LES 13
#define OPTION_RULE_STATISTICS 14
#define OPTION_PG 15
#define OPTION_NO_CHART_MAN 16
#define OPTION_LOG 17
#define OPTION_MEMLIMIT 18
#define OPTION_LATTICE 22
#define OPTION_NBEST 23
#define OPTION_NO_ONLINE_MORPH 24
#define OPTION_NO_FULLFORM_MORPH 25
#define OPTION_PACKING 26
#define OPTION_NQC_SUBS 27
#define OPTION_MRS 28
#define OPTION_TSDB_DUMP 29
#define OPTION_PARTIAL 30
#define OPTION_NRESULTS 31
#define OPTION_TOK 32
#define OPTION_COMPUTE_QC_UNIF 33
#define OPTION_COMPUTE_QC_SUBS 34
#define OPTION_JXCHG_DUMP 35
#define OPTION_COMMENT_PASSTHROUGH 36
#define OPTION_PREDICT_LES 37

#ifdef YY
#define OPTION_ONE_MEANING 100
#define OPTION_YY 101
#define OPTION_K2Y 102
#define OPTION_K2Y_SEGREGATION 103
#endif


class FooConverter : public AbstractConverter<tokenizer_id> {
  virtual ~FooConverter() {}
  virtual std::string toString(const tokenizer_id& t) {
    std::ostringstream out;
    out << (int) t;
    return out.str();
  }
  virtual tokenizer_id fromString(const std::string& s) {
    if (s == "pic_counts") return TOKENIZER_PIC_COUNTS;
    return TOKENIZER_INVALID;
  }
};

void init_options()
{  
  verbosity = 0;
  
  opt_nqc_unif = -1;
  opt_nqc_subs = -1;
  
  Config::addOption<char*> ("opt_compute_qc",
    "Activate code that collects unification/subsumption failures "
    "for quick check computation, contains filename to write results to", 0);
  
  Config::addReference<bool>("opt_compute_qc_unif",
     "Activate failure registration for unification",
     opt_compute_qc_unif, false);
  
  Config::addReference<bool>
    ("opt_compute_qc_subs", "Activate failure registration for subsumption",
     opt_compute_qc_subs, false);
  
  Config::addReference<bool>
    ("opt_print_failure", 
     "Log unification/subsumption failures "
     "(should be replaced by logging or new/different API functionality)",
     opt_print_failure, false);
  
  Config::addOption<bool>("opt_derivation",
    "Store derivations in tsdb profile", true);
  
  Config::addOption<bool>("opt_rulestatistics",
    "dump the per-rule statistics to the tsdb database", false);
  
  Config::addOption<bool>("opt_default_les",
    "Try to use default lexical entries if no regular entries could be found. "
    "Uses POS annotation, if available.", false);
  
  Config::addOption<bool>("opt_pg", "", false);
  
  Config::addOption<bool>("opt_chart_man",
    "Allow lexical dependency filtering", true);
  
  Config::addOption<bool>("opt_nbest", "", false);
  
  Config::addOption<bool>("opt_online_morph", 
    "use the internal morphology (the regular expression style one)", true);
  
  // opt_fullform_morph is obsolete
  // opt_fullform_morph = true;
    
  Config::addOption("opt_predict_les", 
                    "if not zero predict lexical entries for uncovered input",
                    (int) 0);

  Config::addOption<char*>("opt_mrs",
    "determines if and which kind of MRS output is generated", 0 );
  
  Config::addOption<bool>("opt_partial",
    "in case of parse failure, find a set of chart edges "
    "that covers the chart in a good manner", false);
  
  Config::addOption<int>("opt_nresults",
                         "The number of results to print "
                         "(should be an argument of an API function)", 0);
  
  Config::addOption<tokenizer_id>("opt_tok", "", TOKENIZER_STRING,
                                  new FooConverter());

  Config::addOption("opt_jxchg_dir",
                    "the directory to write parse charts in jxchg format to",
                    ((std::string) ""));

}

#ifndef __BORLANDC__
bool parse_options(int argc, char* argv[])
{
  int c,  res;

  struct option options[] = {
#ifdef TSDBAPI
    {"tsdb", optional_argument, 0, OPTION_TSDB},
#endif
    {"nsolutions", optional_argument, 0, OPTION_NSOLUTIONS},
    {"verbose", optional_argument, 0, OPTION_VERBOSE},
    {"limit", required_argument, 0, OPTION_LIMIT},
    {"memlimit", required_argument, 0, OPTION_MEMLIMIT},
    {"no-shrink-mem", no_argument, 0, OPTION_NO_SHRINK_MEM},
    {"no-filter", no_argument, 0, OPTION_NO_FILTER},
    {"qc-unif", required_argument, 0, OPTION_NQC_UNIF},
    {"qc-subs", required_argument, 0, OPTION_NQC_SUBS},
    {"compute-qc", optional_argument, 0, OPTION_COMPUTE_QC},
    {"failure-print", no_argument, 0, OPTION_PRINT_FAILURE},
    {"key", required_argument, 0, OPTION_KEY},
    {"no-hyper", no_argument, 0, OPTION_NO_HYPER},
    {"no-derivation", no_argument, 0, OPTION_NO_DERIVATION},
    {"rulestats", no_argument, 0, OPTION_RULE_STATISTICS},
    {"no-chart-man", no_argument, 0, OPTION_NO_CHART_MAN},
    {"default-les", no_argument, 0, OPTION_DEFAULT_LES},
    {"predict-les", optional_argument, 0, OPTION_PREDICT_LES},
#ifdef YY
    {"yy", no_argument, 0, OPTION_YY},
    {"one-meaning", optional_argument, 0, OPTION_ONE_MEANING},
#endif
    {"server", optional_argument, 0, OPTION_SERVER},
    {"log", required_argument, 0, OPTION_LOG},
    {"pg", no_argument, 0, OPTION_PG},
    {"lattice", no_argument, 0, OPTION_LATTICE},
    {"nbest", no_argument, 0, OPTION_NBEST},
    {"no-online-morph", no_argument, 0, OPTION_NO_ONLINE_MORPH},
    {"no-fullform-morph", no_argument, 0, OPTION_NO_FULLFORM_MORPH},
    {"packing", optional_argument, 0, OPTION_PACKING},
    {"mrs", optional_argument, 0, OPTION_MRS},
    {"tsdbdump", required_argument, 0, OPTION_TSDB_DUMP},
    {"partial", no_argument, 0, OPTION_PARTIAL},
    {"results", required_argument, 0, OPTION_NRESULTS},
    {"tok", optional_argument, 0, OPTION_TOK},
    {"compute-qc-unif", optional_argument, 0, OPTION_COMPUTE_QC_UNIF},
    {"compute-qc-subs", optional_argument, 0, OPTION_COMPUTE_QC_SUBS},
    {"jxchgdump", required_argument, 0, OPTION_JXCHG_DUMP},
    {"comment-passthrough", optional_argument, 0, OPTION_COMMENT_PASSTHROUGH},

    {0, 0, 0, 0}
  }; /* struct option */

  init_options();

  if(!argc) return true;

  while((c = getopt_long_only(argc, argv, "", options, &res)) != EOF)
  {
      switch(c)
      {
      case '?':
          return false;
      case OPTION_TSDB: {
          int opt_tsdb;
          if(optarg != NULL)
          {
              opt_tsdb = strtoint(optarg, "as argument to -tsdb");
              if(opt_tsdb < 0 || opt_tsdb > 2)
              {
                LOG_ERROR(loggerUncategorized, 
                          "parse_options(): invalid tsdb++ protocol"
                          " version");
                  return false;
              }
          }
          else
          {
              opt_tsdb = 1;
          }
          Config::set("opt_tsdb", opt_tsdb);
          }
          break;
      case OPTION_NSOLUTIONS:
          if(optarg != NULL)
              Config::set("opt_nsolutions",
                strtoint(optarg, "as argument to -nsolutions"));
          else
              Config::set("opt_nsolutions", 1);
          break;
      case OPTION_DEFAULT_LES:
          Config::set("opt_default_les", true);
          break;
      case OPTION_NO_CHART_MAN:
          Config::set("opt_chart_man", false); 
          break;
      case OPTION_SERVER:
          if(optarg != NULL)
            Config::setString("opt_server", optarg);
          else
            Config::set("opt_server", CHEAP_SERVER_PORT);
          break;
      case OPTION_NO_SHRINK_MEM:
          Config::set("opt_shrink_mem", false);
          break;
      case OPTION_NO_FILTER:
          Config::set("opt_filter", false);
          break;
      case OPTION_NO_HYPER:
          Config::set("opt_hyper", false);
          break;
      case OPTION_NO_DERIVATION:
          Config::set("opt_derivation", false);
          break;
      case OPTION_RULE_STATISTICS:
          Config::set("opt_rulestatistics", true);
          break;
      case OPTION_COMPUTE_QC:
        // TODO: this is maybe the first application for a callback option to
        // handle the three cases
          if(optarg != NULL)
              Config::set("opt_compute_qc", strdup(optarg));
          else
              Config::set("opt_compute_qc", "/tmp/qc.tdl");
          Config::set("opt_compute_qc_unif", true);
          Config::set("opt_compute_qc_subs", true);
          break;
      case OPTION_COMPUTE_QC_UNIF:
          if(optarg != NULL)
              Config::set("opt_compute_qc", strdup(optarg));
          else
              Config::set("opt_compute_qc", "/tmp/qc.tdl");
          Config::set("opt_compute_qc_unif", true);
          break;
      case OPTION_COMPUTE_QC_SUBS:
          if(optarg != NULL)
              Config::set("opt_compute_qc", strdup(optarg));
          else
              Config::set("opt_compute_qc", "/tmp/qc.tdl");
          Config::set("opt_compute_qc_subs", true);
          break;
      case OPTION_PRINT_FAILURE:
          Config::set("opt_print_failure", true);
          break;
      case OPTION_PG:
          Config::set("opt_pg", true);
          break;
      case OPTION_LATTICE:
          Config::set("opt_lattice", true);
          break;
      case OPTION_VERBOSE:
          if(optarg != NULL)
              verbosity = strtoint(optarg, "as argument to `-verbose'");
          else
              verbosity++;
          break;
      case OPTION_NQC_UNIF:
          if(optarg != NULL)
              opt_nqc_unif = strtoint(optarg, "as argument to `-qc-unif'");
          break;
      case OPTION_NQC_SUBS:
          if(optarg != NULL)
              opt_nqc_subs = strtoint(optarg, "as argument to `-qc-subs'");
          break;
      case OPTION_KEY:
          if(optarg != NULL)
              Config::set("opt_key",
                strtoint(optarg, "as argument to `-key'"));
          break;
      case OPTION_LIMIT:
          if(optarg != NULL)
            Config::setString("pedgelimit", optarg);
          break;
      case OPTION_MEMLIMIT:
          if(optarg != NULL)
            Config::setString("memlimit", optarg);
          break;
      case OPTION_LOG:
          if(optarg != NULL)
              if(optarg[0] == '+') flog = fopen(&optarg[1], "a");
              else flog = fopen(optarg, "w");
          break;
      case OPTION_NBEST:
          Config::set("opt_nbest", true);
          break;
      case OPTION_NO_ONLINE_MORPH:
          Config::set("opt_online_morph", false);
          break;
      case OPTION_NO_FULLFORM_MORPH:
          // opt_fullform_morph is obsolete
          // opt_fullform_morph = false;
          break;
      case OPTION_PACKING:
        if(optarg != NULL)
          Config::setString("opt_packing", optarg);
        else
          Config::set("opt_packing", 
                      (int)(PACKING_EQUI | PACKING_PRO |
                            PACKING_RETRO | PACKING_SELUNPACK));
        break;
      case OPTION_MRS:
          if(optarg != NULL)
            Config::set("opt_mrs", strdup(optarg));
          else
            Config::set("opt_mrs", "simple");
          break;
      case OPTION_TSDB_DUMP:
        Config::set<std::string>("opt_tsdb_dir", ((std::string) optarg));
          break;
      case OPTION_PARTIAL:
          Config::set("opt_partial", true);
          break;
      case OPTION_NRESULTS:
          if(optarg != NULL)
              Config::set("opt_nresults",
                strtoint(optarg, "as argument to -results"));
          break;
      case OPTION_TOK: 
        {
          tokenizer_id opt_tok = TOKENIZER_STRING; //todo: make FSR the default
          if (optarg != NULL) {
            if (strcasecmp(optarg, "string") == 0)
              opt_tok = TOKENIZER_STRING;
            else if (strcasecmp(optarg, "yy") == 0)
              opt_tok = TOKENIZER_YY;
            else if (strcasecmp(optarg, "yy_counts") == 0)
              opt_tok = TOKENIZER_YY_COUNTS;
            else if (strcasecmp(optarg, "xml") == 0) {
              LOG(loggerUncategorized, WARN, "WARNING: deprecated command-line option "
                       " -tok=xml, use -tok=pic instead\n");
              opt_tok = TOKENIZER_PIC; // deprecated command-line option
            }
            else if (strcasecmp(optarg, "xml_counts") == 0) {
              LOG(loggerUncategorized, WARN, "WARNING: deprecated command-line option "
                       " -tok=xml_counts, use -tok=pic_counts instead\n");
              opt_tok = TOKENIZER_PIC_COUNTS; // deprecated command-line option
            }
            else if (strcasecmp(optarg, "pic") == 0)
              opt_tok = TOKENIZER_PIC;
            else if (strcasecmp(optarg, "pic_counts") == 0)
              opt_tok = TOKENIZER_PIC_COUNTS;
            else if (strcasecmp(optarg, "smaf") == 0)
              opt_tok = TOKENIZER_SMAF;
            else if (strcasecmp(optarg, "fsr") == 0)
              opt_tok = TOKENIZER_FSR;
            else
              LOG_ERROR(loggerUncategorized,
                        "WARNING: unknown tokenizer mode "
                        "\"%s\": using 'tok=string'", optarg);
          }
          Config::set("opt_tok", opt_tok);
        }
        break;
      case OPTION_JXCHG_DUMP:
          Config::set<std::string>("opt_jxchg_dir", optarg);
          if (*(Config::get<std::string>("opt_jxchg_dir").end()--)
              != '/') 
            Config::get<std::string>("opt_jxchg_dir") += '/';
          break;

      case OPTION_COMMENT_PASSTHROUGH:
          if(optarg != NULL)
            Config::setString("opt_comment_passthrough", optarg);
          else
            Config::set("opt_comment_passthrough", true);
          break;

      case OPTION_PREDICT_LES:
        if (optarg != NULL)
          Config::setString("opt_predict_les", optarg);
        else 
          Config::set("opt_predict_les", (int) 1);
        break;

#ifdef YY
      case OPTION_ONE_MEANING:
          if(optarg != NULL)
            Config::setString("opt_nth_meaning", optarg);
          else
            Config::set("opt_nth_meaning", 1);
          break;
      case OPTION_YY:
          Config::set("opt_yy", true);
          Config::set("opt_tok", TOKENIZER_YY);
          break;
#endif
        }
    }

  if(optind != argc - 1)
    {
      LOG_ERROR(loggerUncategorized, 
                "parse_options(): expecting name of grammar to load");
      return false;
    }
  grammar_file_name = argv[optind];

  if( Config::get<bool>("opt_hyper") &&
      Config::get<char*>("opt_compute_qc") != NULL)
  {
    LOG_ERROR(loggerUncategorized, 
              "quickcheck computation doesn't work "
              "in hyperactive mode, disabling hyperactive mode.");
      Config::set("opt_hyper", false);
  }

  return true;
}
#endif

bool bool_setting(settings *set, const char *s)
{
  char *v = set->value(s);
  if(v == 0 || strcmp(v, "0") == 0 || strcasecmp(v, "false") == 0 ||
     strcasecmp(v, "nil") == 0)
    return false;

  return true;
}

int int_setting(settings *set, const char *s)
{
  char *v = set->value(s);
  if(v == 0)
    return 0;
  int i = strtoint(v, "in settings file");
  return i;
}

void options_from_settings(settings *set)
{
  init_options();
  if(bool_setting(set, "one-solution"))
      Config::set("opt_nsolutions", 1);
  else
      Config::set("opt_nsolutions", 0);
  verbosity = int_setting(set, "verbose");
  Config::set("pedgelimit", int_setting(set, "limit"));
  Config::set("memlimit", int_setting(set, "memlimit"));
  Config::set("opt_hyper", bool_setting(set, "hyper"));
  Config::set("opt_default_les", bool_setting(set, "default-les"));
  Config::set("opt_predict_les", int_setting(set, "predict-les"));
#ifdef YY
  Config::set("opt_nth_meaning", (bool_setting(set, "one-meaning")) ? 1 : 0);
#endif
}
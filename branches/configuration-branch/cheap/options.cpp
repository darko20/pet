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
#include <string>

bool opt_shrink_mem, opt_shaping;

// opt_fullform_morph is obsolete
// bool opt_fullform_morph;

//defined in fs.cpp
extern bool opt_compute_qc_unif, opt_compute_qc_subs, opt_print_failure;

//defined in parse.cpp
extern bool opt_filter, opt_hyper;
extern int  opt_nsolutions;

// defined in item.cpp
extern bool opt_lattice;

#ifdef YY
bool opt_yy, opt_nth_meaning;
#endif

int opt_nqc_unif, opt_nqc_subs, verbosity, pedgelimit, opt_server;
int opt_tsdb;
long int memlimit;
char *grammar_file_name = 0;

// 2006/10/01 Yi Zhang <yzhang@coli.uni-sb.de>: new option for grand-parenting
// level in MEM-based parse selection
// defined in item.cpp
extern unsigned int opt_gplevel;

// defined in parse.cpp
extern int opt_packing;

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
  fprintf(f, "  `-lattice' --- word lattice parsing\n");
  fprintf(f, "  `-server[=n]' --- go into server mode, bind to port `n' (default: 4711)\n");
#ifdef YY
  fprintf(f, "  `-k2y[=n]' --- "
          "output K2Y, filter at `n' %% of raw atoms (default: 50)\n");
  fprintf(f, "  `-k2y-segregation' --- "
          "pre-nominal modifiers in analogy to reduced relatives\n");
  fprintf(f, "  `-one-meaning[=n]' --- non exhaustive search for first [nth]\n"
             "                         valid semantic formula\n");
  fprintf(f, "  `-yy' --- YY input mode (highly experimental)\n");
#endif
  fprintf(f, "  `-failure-print' --- print failure paths\n");
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
  fprintf(f, "  `-tok=string|fsr|yy|yy_counts|xml|xml_counts' --- "
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

#ifdef YY
#define OPTION_ONE_MEANING 100
#define OPTION_YY 101
#define OPTION_K2Y 102
#define OPTION_K2Y_SEGREGATION 103
#endif


void init_options()
{
  opt_tsdb = 0;
  
  Configuration::addRefOption<int>("opt_nsolutions", &opt_nsolutions,
    "The number of solutions until the parser is stopped, "
    "if not in packing mode", 0);
    
  verbosity = 0;
  pedgelimit = 0;
  memlimit = 0;
  opt_shrink_mem = true;
  opt_shaping = true;
  
  Configuration::addRefOption<bool>("opt_filter", &opt_filter,
    "Use the static rule filter", true);
  
  opt_nqc_unif = -1;
  opt_nqc_subs = -1;
  
  Configuration::addOption<char*>("opt_compute_qc",
    "Activate code that collects unification/subsumption failures "
    "for quick check computation, contains filename to write results to", 0);
  
  Configuration::addRefOption<bool>("opt_compute_qc_unif", &opt_compute_qc_unif,
    "Activate failure registration for unification", false);
  
  Configuration::addRefOption<bool>("opt_compute_qc_subs", &opt_compute_qc_subs,
    "Activate failure registration for subsumption", false);
  
  Configuration::addRefOption<bool>("opt_print_failure", &opt_print_failure,
    "Log unification/subsumption failures "
    "(should be replaced by logging or new/different API functionality)",
    false);
  
  Configuration::addOption<int>("opt_key",
    "What is the key daughter used in parsing?"
    "0: key-driven, 1: l-r, 2: r-l, 3: head-driven", 0);
  
  Configuration::addRefOption<bool>("opt_hyper", &opt_hyper,
    "use hyperactive parsing", true);
  
  Configuration::addOption<bool>("opt_derivation",
    "Store derivations in tsdb profile", true);
  
  Configuration::addOption<bool>("opt_rulestatistics",
    "dump the per-rule statistics to the tsdb database", false);
  
  Configuration::addOption<bool>("opt_default_les",
    "Try to use default lexical entries if no regular entries could be found. "
    "Uses POS annotation, if available.", false);
  
  opt_server = 0;
  
  Configuration::addOption<bool>("opt_pg", "", false);
  
  Configuration::addOption<bool>("opt_chart_man",
    "Allow lexical dependency filtering", true);
  
  Configuration::addRefOption<bool>("opt_lattice", &opt_lattice, 
    "is the lattice structure specified in the input "
    "used to restrict the search space in parsing", false);
  
  Configuration::addOption<bool>("opt_nbest", "", false);
  
  Configuration::addOption<bool>("opt_online_morph", 
    "use the internal morphology (the regular expression style one)", true);
  
  // opt_fullform_morph is obsolete
  // opt_fullform_morph = true;
    
  Configuration::addRefOption<int>("opt_packing", &opt_packing,
    "a bit vector of flags: 1:equivalence 2:proactive 4:retroactive packing "
    "8:selective 128:no unpacking", 0);

  Configuration::addOption<char*>("opt_mrs",
    "determines if and which kind of MRS output is generated", 0 );
  
  Configuration::addOption<std::string>("opt_tsdb_dir", "", "");

  Configuration::addOption<bool>("opt_partial",
    "in case of parse failure, find a set of chart edges "
    "that covers the chart in a good manner", false);
  
  Configuration::addOption<int>("opt_nresults",
    "The number of results to print "
    "(should be an argument of an API function)", 0);
  
  Configuration::addOption<tokenizer_id>("opt_tok", "", TOKENIZER_STRING);

  Configuration::addOption<std::string>("opt_jxchg_dir",
    "the directory to write parse charts in jxchg format to",
    "");
  
  // 2004/03/12 Eric Nichols <eric-n@is.naist.jp>: new option for input comments
  Configuration::addOption<bool>("opt_comment_passthrough",
    "Ignore/repeat input classified as comment: starts with '#' or '//'",
    false);
  
  Configuration::addOption<bool>("opt_linebreaks", "", false);
  
  Configuration::addRefOption<unsigned int>("opt_gplevel", &opt_gplevel,
    "determine the level of grandparenting "
    "used in the models for selective unpacking", 0);

#ifdef YY
  opt_yy = false;
  opt_nth_meaning = 0;
#endif
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
      case OPTION_TSDB:
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
          break;
      case OPTION_NSOLUTIONS:
          if(optarg != NULL)
              Configuration::set("opt_nsolutions",
                strtoint(optarg, "as argument to -nsolutions"));
          else
              Configuration::set("opt_nsolutions", 1);
          break;
      case OPTION_DEFAULT_LES:
          Configuration::set("opt_default_les", true);
          break;
      case OPTION_NO_CHART_MAN:
          Configuration::set("opt_chart_man", false); 
          break;
      case OPTION_SERVER:
          if(optarg != NULL)
              opt_server = strtoint(optarg, "as argument to `-server'");
          else
              opt_server = CHEAP_SERVER_PORT;
          break;
      case OPTION_NO_SHRINK_MEM:
          opt_shrink_mem = false;
          break;
      case OPTION_NO_FILTER:
          Configuration::set("opt_filter", false);
          break;
      case OPTION_NO_HYPER:
          Configuration::set("opt_hyper", false);
          break;
      case OPTION_NO_DERIVATION:
          Configuration::set("opt_derivation", false);
          break;
      case OPTION_RULE_STATISTICS:
          Configuration::set("opt_rulestatistics", true);
          break;
      case OPTION_COMPUTE_QC:
          if(optarg != NULL)
              Configuration::set("opt_compute_qc", strdup(optarg));
          else
              Configuration::set("opt_compute_qc", "/tmp/qc.tdl");
          Configuration::set("opt_compute_qc_unif", true);
          Configuration::set("opt_compute_qc_subs", true);
          break;
      case OPTION_COMPUTE_QC_UNIF:
          if(optarg != NULL)
              Configuration::set("opt_compute_qc", strdup(optarg));
          else
              Configuration::set("opt_compute_qc", "/tmp/qc.tdl");
          Configuration::set("opt_compute_qc_unif", true);
          break;
      case OPTION_COMPUTE_QC_SUBS:
          if(optarg != NULL)
              Configuration::set("opt_compute_qc", strdup(optarg));
          else
              Configuration::set("opt_compute_qc", "/tmp/qc.tdl");
          Configuration::set("opt_compute_qc_subs", true);
          break;
      case OPTION_PRINT_FAILURE:
          Configuration::set("opt_print_failure", true);
          break;
      case OPTION_PG:
          Configuration::set("opt_pg", true);
          break;
      case OPTION_LATTICE:
          Configuration::set("opt_lattice", true);
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
              Configuration::set("opt_key",
                strtoint(optarg, "as argument to `-key'"));
          break;
      case OPTION_LIMIT:
          if(optarg != NULL)
              pedgelimit = strtoint(optarg, "as argument to -limit");
          break;
      case OPTION_MEMLIMIT:
          if(optarg != NULL)
              memlimit = 1024 * 1024 * strtoint(optarg, "as argument to -memlimit");
          break;
      case OPTION_LOG:
          if(optarg != NULL)
              if(optarg[0] == '+') flog = fopen(&optarg[1], "a");
              else flog = fopen(optarg, "w");
          break;
      case OPTION_NBEST:
          Configuration::set("opt_nbest", true);
          break;
      case OPTION_NO_ONLINE_MORPH:
          Configuration::set("opt_online_morph", false);
          break;
      case OPTION_NO_FULLFORM_MORPH:
          // opt_fullform_morph is obsolete
          // opt_fullform_morph = false;
          break;
      case OPTION_PACKING:
          if(optarg != NULL)
            Configuration::set("opt_packing",
              strtoint(optarg, "as argument to `-packing'"));
          else
            Configuration::set("opt_packing",
              PACKING_EQUI|PACKING_PRO|PACKING_RETRO|PACKING_SELUNPACK);
          break;
      case OPTION_MRS:
          if(optarg != NULL)
            Configuration::set("opt_mrs", strdup(optarg));
          else
            Configuration::set("opt_mrs", "simple");
          break;
      case OPTION_TSDB_DUMP:
          Configuration::set<std::string>("opt_tsdb_dir", optarg);
          break;
      case OPTION_PARTIAL:
          Configuration::set("opt_partial", true);
          break;
      case OPTION_NRESULTS:
          if(optarg != NULL)
              Configuration::set("opt_nresults",
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
            else if (strcasecmp(optarg, "xml") == 0)
              opt_tok = TOKENIZER_XML; //obsolete
            else if (strcasecmp(optarg, "xml_counts") == 0)
              opt_tok = TOKENIZER_XML_COUNTS; //obsolete
            else if (strcasecmp(optarg, "pic") == 0)
              opt_tok = TOKENIZER_XML;
            else if (strcasecmp(optarg, "pic_counts") == 0)
              opt_tok = TOKENIZER_XML_COUNTS;
            else if (strcasecmp(optarg, "smaf") == 0)
              opt_tok = TOKENIZER_SMAF;
            else if (strcasecmp(optarg, "fsr") == 0)
              opt_tok = TOKENIZER_FSR;
            else
              LOG_ERROR(loggerUncategorized,
                        "WARNING: unknown tokenizer mode "
                        "\"%s\": using 'tok=string'", optarg);
          }
          Configuration::set("opt_tok", opt_tok);
        }
        break;
      case OPTION_JXCHG_DUMP:
          Configuration::set<std::string>("opt_jxchg_dir", optarg);
          if (*(Configuration::get<std::string>("opt_jxchg_dir").end()--)
              != '/') 
            Configuration::get<std::string>("opt_jxchg_dir") += '/';
          break;

      case OPTION_COMMENT_PASSTHROUGH:
          if(optarg != NULL)
            Configuration::set("opt_comment_passthrough", 
              strtoint(optarg, "as argument to -comment-passthrough") == 1);
          else
              Configuration::set("opt_comment_passthrough", true);
	  break;

#ifdef YY
      case OPTION_ONE_MEANING:
          if(optarg != NULL)
              opt_nth_meaning = strtoint(optarg, "as argument to -one-meaning");
          else
              opt_nth_meaning = 1;
          break;
      case OPTION_YY:
          opt_yy = true;
          Configuration::set("opt_tok", TOKENIZER_YY);
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

  if(opt_hyper && Configuration::get<char*>("opt_compute_qc") != 0)
  {
    LOG_ERROR(loggerUncategorized, 
              "quickcheck computation doesn't work "
              "in hyperactive mode, disabling hyperactive mode.");
      Configuration::set("opt_hyper", false);
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
      Configuration::set("opt_nsolutions", 1);
  else
      Configuration::set("opt_nsolutions", 0);
  verbosity = int_setting(set, "verbose");
  pedgelimit = int_setting(set, "limit");
  memlimit = 1024 * 1024 * int_setting(set, "memlimit");
  Configuration::set("opt_hyper", bool_setting(set, "hyper"));
  Configuration::set("opt_default_les", bool_setting(set, "default-les"));
#ifdef YY
  if(bool_setting(set, "one-meaning"))
    opt_nth_meaning = 1;
  else
    opt_nth_meaning = 0;
#endif
}

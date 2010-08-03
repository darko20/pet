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
#include "getopt.h"
#include "fs.h"
#include "cheap.h"
#include "utility.h"
#include "version.h"
#include "logging.h"
#include <string>
#include <unistd.h>

extern int verbosity;

void usage(FILE *f)
{
  fprintf(f, "cheap version %s\n", version_string);
  fprintf(f, "usage: `cheap [options] grammar-file'; valid options are:\n");
#ifdef TSDBAPI
  fprintf(f, "  `-tsdb[=n]' --- "
          "enable [incr tsdb()] slave mode (protocol version = n)\n");
#endif
  fprintf(f, "  `-nsolutions[=n]' --- find best n only, 1 if n is not given\n");
  fprintf(f, "  `-sm[=string]' --- parse selection model (`null' for none)\n");
  fprintf(f, "  `-verbose[=n]' --- set verbosity level to n\n");
  fprintf(f, "  `-limit=n' --- maximum number of passive edges\n");
  fprintf(f, "  `-memlimit=n' --- maximum amount of fs memory (in MB)\n");
  fprintf(f, "  `-timeout=n' --- maximum time (in s) spent on analyzing a sentence\n");
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
  fprintf(f, "  `-one-meaning[=n]' --- non exhaustive search for first [nth]\n"
             "                         valid semantic formula\n");
  fprintf(f, "  `-yy' --- YY input mode (highly experimental)\n");
#endif
  fprintf(f, "  `-failure-print' --- print failure paths\n");
  fprintf(f, "  `-interactive-online-morph' --- morphology only\n");
  fprintf(f, "  `-pg[=what]' --- print grammar in ASCII form ('s'ymbols, 'g'lbs, 't'ypes(fs), 'a'll)\n");
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
#define OPTION_INTERACTIVE_MORPH 19
#define OPTION_LATTICE 22
#define OPTION_NBEST 23
#define OPTION_NO_ONLINE_MORPH 24
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
#define OPTION_TIMEOUT 38
#define OPTION_SM 40

#ifdef YY
#define OPTION_ONE_MEANING 100
#define OPTION_YY 101
#endif

//typedef T (*fromfunc)(const std::string &s);
//typedef std::string (*tofunc)(const T &val);

#ifndef __BORLANDC__
char* parse_options(int argc, char* argv[])
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
    {"timeout", required_argument, 0, OPTION_TIMEOUT},
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
    {"pg", optional_argument, 0, OPTION_PG},
    {"interactive-online-morphology", no_argument, 0, OPTION_INTERACTIVE_MORPH},
    {"lattice", no_argument, 0, OPTION_LATTICE},
    {"no-online-morph", no_argument, 0, OPTION_NO_ONLINE_MORPH},
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
    {"sm", optional_argument, 0, OPTION_SM},
    {0, 0, 0, 0}
  }; /* struct option */

  if(!argc) return NULL;

  while((c = getopt_long_only(argc, argv, "+", options, &res)) != EOF)
  {
      switch(c)
      {
      case '?':
          return NULL;
      case OPTION_TSDB: {
          int opt_tsdb;
          if(optarg != NULL)
          {
              opt_tsdb = strtoint(optarg, "as argument to -tsdb");
              if(opt_tsdb < 0 || opt_tsdb > 2)
              {
                LOG(logAppl, FATAL,
                    "parse_options(): invalid tsdb++ protocol version");
               return NULL;
              }
          }
          else
          {
              opt_tsdb = 1;
          }
          set_opt("opt_tsdb", opt_tsdb);
          }
          break;
      case OPTION_NSOLUTIONS:
          if(optarg != NULL)
            set_opt_from_string("opt_nsolutions", optarg);
          else
            set_opt("opt_nsolutions", 1);
          break;
      case OPTION_DEFAULT_LES:
          set_opt("opt_default_les", true);
          break;
      case OPTION_NO_CHART_MAN:
          set_opt("opt_chart_man", false);
          break;
      case OPTION_SERVER:
          if(optarg != NULL)
            set_opt_from_string("opt_server", optarg);
          else
            set_opt("opt_server", CHEAP_SERVER_PORT);
          break;
      case OPTION_NO_SHRINK_MEM:
          set_opt("opt_shrink_mem", false);
          break;
      case OPTION_NO_FILTER:
          set_opt("opt_filter", false);
          break;
      case OPTION_NO_HYPER:
          set_opt("opt_hyper", false);
          break;
      case OPTION_NO_DERIVATION:
          set_opt("opt_derivation", false);
          break;
      case OPTION_RULE_STATISTICS:
          set_opt("opt_rulestatistics", true);
          break;
      case OPTION_COMPUTE_QC:
        // TODO: this is maybe the first application for a callback option to
        // handle the three cases
          set_opt("opt_compute_qc",
                  (optarg != NULL) ? optarg : "/tmp/qc.tdl");
          set_opt("opt_compute_qc_unif", true);
          set_opt("opt_compute_qc_subs", true);
          break;
      case OPTION_COMPUTE_QC_UNIF:
          set_opt("opt_compute_qc",
                  (optarg != NULL) ? optarg : "/tmp/qc.tdl");
          set_opt("opt_compute_qc_unif", true);
          break;
      case OPTION_COMPUTE_QC_SUBS:
          set_opt("opt_compute_qc",
                        (optarg != NULL) ? optarg : "/tmp/qc.tdl");
          set_opt("opt_compute_qc_subs", true);
          break;
      case OPTION_PRINT_FAILURE:
          set_opt("opt_print_failure", true);
          break;
      case OPTION_PG:
        set_opt("opt_pg", 's');
        if(optarg != NULL) {
          const char *pos = strchr("sgta", optarg[0]);
          if(pos != NULL) {
            set_opt("opt_pg", optarg[0]);
          } else {
            LOG(logAppl, WARN,
                "Invalid argument to -pg, printing only symbols\n");
          }
        }
        break;
      case OPTION_INTERACTIVE_MORPH:
          set_opt("opt_interactive_morph", true);
          break;
      case OPTION_LATTICE:
          set_opt("opt_lattice", true);
          break;
      case OPTION_VERBOSE:
          if(optarg != NULL)
              verbosity = strtoint(optarg, "as argument to `-verbose'");
          else
              verbosity++;
          break;
      case OPTION_NQC_UNIF:
          if(optarg != NULL)
              set_opt_from_string("opt_nqc_unif", optarg);
          break;
      case OPTION_NQC_SUBS:
          if(optarg != NULL)
              set_opt_from_string("opt_nqc_subs", optarg);
          break;
      case OPTION_KEY:
          if(optarg != NULL)
            set_opt_from_string("opt_key", optarg);
          break;
      case OPTION_LIMIT:
          if(optarg != NULL)
            set_opt_from_string("opt_pedgelimit", optarg);
          break;
      case OPTION_MEMLIMIT:
          if(optarg != NULL)
            set_opt_from_string("opt_memlimit", optarg);
          break;
      case OPTION_TIMEOUT:
        // \todo the option should store the raw value and leave the details to
        // the implementation
          if(optarg != NULL)
            set_opt("opt_timeout",
                        (int)(sysconf(_SC_CLK_TCK) *
                              strtoint(optarg, "as argument to -timeout")));
          break;
      case OPTION_LOG:
        if(optarg != NULL) {
          if(optarg[0] == '+') flog = fopen(&optarg[1], "a");
          else flog = fopen(optarg, "w");
        }
        break;
      case OPTION_NO_ONLINE_MORPH:
          set_opt("opt_online_morph", false);
          break;
      case OPTION_PACKING:
        if(optarg != NULL)
          set_opt_from_string("opt_packing", optarg);
        else
          set_opt("opt_packing",
                      (int)(PACKING_EQUI | PACKING_PRO |
                            PACKING_RETRO | PACKING_SELUNPACK));
        break;
      case OPTION_MRS:
        // either this way or the like in the next option. too bad there's no
        // automatic casting here
        set_opt("opt_mrs", std::string((optarg != NULL) ? optarg : "simple"));
        break;
      case OPTION_TSDB_DUMP:
        set_opt<std::string>("opt_tsdb_dir", optarg);
        break;
      case OPTION_PARTIAL:
          set_opt("opt_partial", true);
          break;
      case OPTION_NRESULTS:
          if(optarg != NULL)
            set_opt_from_string("opt_nresults", optarg);
          break;
      case OPTION_TOK:
          set_opt_from_string("opt_tok", optarg);
          break;
      case OPTION_JXCHG_DUMP:
        if (optarg[strlen(optarg) - 1] != '/')
          set_opt("opt_jxchg_dir", std::string(optarg) + "/");
        else
          set_opt("opt_jxchg_dir", std::string(optarg));
        break;
      case OPTION_COMMENT_PASSTHROUGH:
          if(optarg != NULL)
            set_opt_from_string("opt_comment_passthrough", optarg);
          else
            set_opt("opt_comment_passthrough", true);
          break;
      case OPTION_PREDICT_LES:
        if (optarg != NULL)
          set_opt_from_string("opt_predict_les", optarg);
        else
          set_opt("opt_predict_les", (int) 1);
        break;
      case OPTION_SM:
          if (optarg != NULL)
            set_opt("opt_sm", std::string(optarg));
          else
            set_opt("opt_sm", std::string("null"));
          break;
#ifdef YY
      case OPTION_ONE_MEANING:
          if(optarg != NULL)
            set_opt_from_string("opt_nth_meaning", optarg);
          else
            set_opt("opt_nth_meaning", 1);
          break;
      case OPTION_YY:
          set_opt("opt_yy", true);
          set_opt_from_string("opt_tok", "yy");
          break;
#endif
        }
    }

  if(optind != argc - 1) {
    LOG(logAppl, FATAL, "could not parse options: "
        "expecting grammar-file as last parameter");
    return NULL;
  }

  if( get_opt_bool("opt_hyper") &&
      get_opt_charp("opt_compute_qc") != NULL)
  {
    LOG(logAppl, WARN,"quickcheck computation doesn't work "
        "in hyperactive mode, disabling.");
      set_opt("opt_hyper", false);
  }

  return argv[optind];
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

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
#include <unistd.h>

char *grammar_file_name;
char *opt_mrs;
char *opt_tsdb_mrs;
int opt_pg;
int opt_tsdb;
int opt_server;
bool opt_interactive_morph;
tokenizer_id opt_tok;
int opt_comment_passthrough;
bool opt_default_les;
int opt_predict_les;
int opt_packing;
int opt_nsolutions;
bool opt_shaping;
bool opt_filter;
bool opt_hyper;
bool opt_chart_man;
int opt_key;
char *opt_compute_qc;
bool opt_compute_qc_unif;
bool opt_compute_qc_subs;
int opt_nqc_unif;
int opt_nqc_subs;
long int opt_memlimit;
int opt_timeout;
int opt_pedgelimit;
bool opt_shrink_mem;
int verbosity;
bool opt_print_failure;
bool opt_derivation;
bool opt_rulestatistics;
int opt_nresults;
bool opt_partial;
std::string opt_tsdb_dir;
bool opt_online_morph;
bool opt_lattice;
unsigned int opt_gplevel;
std::string opt_jxchg_dir;
bool opt_linebreaks;
#ifdef YY
bool opt_yy;
int opt_nth_meaning;
#endif


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
  fprintf(f, "  `-server[=n]' --- go into server mode, bind to port `n' (default: 4711)\n");
#ifdef YY
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
  fprintf(f, "  `-tsdb_mrs[=mrs|mrx|rmrs|rmrx]' --- compute (other) MRS semantics for [incr tsdb()]\n");
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
#define OPTION_TSDB_MRS 39

#ifdef YY
#define OPTION_ONE_MEANING 100
#define OPTION_YY 101
#endif


void init_options()
{
  grammar_file_name = 0;
  opt_mrs = 0;
  opt_tsdb_mrs = 0;
  opt_pg = 0;
  opt_tsdb = 0;
  opt_server = 0;
  opt_interactive_morph = false;
  opt_tok = TOKENIZER_STRING;
  opt_comment_passthrough = 0;
  opt_default_les = false;
  opt_predict_les = 0;
  opt_packing = 0;
  opt_nsolutions = 0;
  opt_shaping = true;
  opt_filter = true;
  opt_hyper = true;
  opt_chart_man = true;
  opt_key = 0;
  opt_compute_qc = 0;
  opt_compute_qc_unif = false;
  opt_compute_qc_subs = false;
  opt_nqc_unif = -1;
  opt_nqc_subs = -1;
  opt_memlimit = 0;
  opt_timeout = 0;
  opt_pedgelimit = 0;
  opt_shrink_mem = true;
  verbosity = 0;
  opt_print_failure = false;
  opt_derivation = true;
  opt_rulestatistics = false;
  opt_nresults = 0;
  opt_partial = false;
  opt_tsdb_dir = "";
  opt_online_morph = true;
  opt_lattice = false;
  opt_gplevel = 0;
  opt_jxchg_dir = "";
  opt_linebreaks = false;

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
    {"tsdb_mrs", optional_argument, 0, OPTION_TSDB_MRS},
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
                  fprintf(ferr, "parse_options(): invalid tsdb++ protocol"
                          " version\n");
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
              opt_nsolutions = strtoint(optarg, "as argument to -nsolutions");
          else
              opt_nsolutions = 1;
          break;
      case OPTION_DEFAULT_LES:
          opt_default_les = true;
          break;
      case OPTION_NO_CHART_MAN:
          opt_chart_man = false;
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
          opt_filter = false;
          break;
      case OPTION_NO_HYPER:
          opt_hyper = false;
          break;
      case OPTION_NO_DERIVATION:
          opt_derivation = false;
          break;
      case OPTION_RULE_STATISTICS:
          opt_rulestatistics = true;
          break;
      case OPTION_COMPUTE_QC:
          if(optarg != NULL)
            opt_compute_qc = strdup(optarg);
          else
            opt_compute_qc = strdup("/tmp/qc.tdl");
          opt_compute_qc_unif = true;
          opt_compute_qc_subs = true;
          break;
      case OPTION_COMPUTE_QC_UNIF:
          if(optarg != NULL)
            opt_compute_qc = strdup(optarg);
          else
            opt_compute_qc = strdup("/tmp/qc.tdl");
          opt_compute_qc_unif = true;
          break;
      case OPTION_COMPUTE_QC_SUBS:
          if(optarg != NULL)
            opt_compute_qc = strdup(optarg);
          else
            opt_compute_qc = strdup("/tmp/qc.tdl");
          opt_compute_qc_subs = true;
          break;
      case OPTION_PRINT_FAILURE:
          opt_print_failure = true;
          break;
      case OPTION_PG:
        opt_pg = 1;
        if(optarg != NULL) {
          const char *what = "sgta";
          char *pos = strchr(what, optarg[0]);
          if(pos != NULL) {
            opt_pg = 1 + (pos - what);
          } else {
            fprintf(ferr,"Invalid argument to -pg, printing only symbols\n");
          }
        }
        break;
      case OPTION_INTERACTIVE_MORPH:
          opt_interactive_morph = true;
          break;
      case OPTION_LATTICE:
          opt_lattice = true;
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
              opt_key = strtoint(optarg, "as argument to `-key'");
          break;
      case OPTION_LIMIT:
          if(optarg != NULL)
              opt_pedgelimit = strtoint(optarg, "as argument to -limit");
          break;
      case OPTION_MEMLIMIT:
          if(optarg != NULL)
              opt_memlimit = 1024 * 1024 * strtoint(optarg, "as argument to -memlimit");
          break;
      case OPTION_TIMEOUT:
          if(optarg != NULL)
              opt_timeout = sysconf(_SC_CLK_TCK) * strtoint(optarg, "as argument to -timeout");
          break;
      case OPTION_LOG:
          if(optarg != NULL)
              if(optarg[0] == '+') flog = fopen(&optarg[1], "a");
              else flog = fopen(optarg, "w");
          break;
      case OPTION_NO_ONLINE_MORPH:
          opt_online_morph = false;
          break;
      case OPTION_PACKING:
          if(optarg != NULL)
              opt_packing = strtoint(optarg, "as argument to `-packing'");
          else
              opt_packing
                = PACKING_EQUI|PACKING_PRO|PACKING_RETRO|PACKING_SELUNPACK;
          break;
      case OPTION_MRS:
          if(optarg != NULL)
            opt_mrs = strdup(optarg);
          else
            opt_mrs = strdup("simple");
          break;
      case OPTION_TSDB_MRS:
          if(optarg != NULL)
            opt_tsdb_mrs = strdup(optarg);
          else
            opt_tsdb_mrs = strdup("simple");
          break;
      case OPTION_TSDB_DUMP:
          opt_tsdb_dir = optarg;
          break;
      case OPTION_PARTIAL:
          opt_partial = true;
          break;
      case OPTION_NRESULTS:
          if(optarg != NULL)
              opt_nresults = strtoint(optarg, "as argument to -results");
          break;
      case OPTION_TOK:
          opt_tok = TOKENIZER_STRING; //todo: make FSR the default
          if (optarg != NULL) {
            if (strcasecmp(optarg, "string") == 0) opt_tok = TOKENIZER_STRING;
            else if (strcasecmp(optarg, "fsr") == 0) opt_tok = TOKENIZER_FSR;
            else if (strcasecmp(optarg, "yy") == 0) opt_tok = TOKENIZER_YY;
            else if (strcasecmp(optarg, "yy_counts") == 0) opt_tok = TOKENIZER_YY_COUNTS;
            else if (strcasecmp(optarg, "pic") == 0) opt_tok = TOKENIZER_PIC;
            else if (strcasecmp(optarg, "pic_counts") == 0) opt_tok = TOKENIZER_PIC_COUNTS;
            else if (strcasecmp(optarg, "smaf") == 0) opt_tok = TOKENIZER_SMAF;
            else if (strcasecmp(optarg, "xml") == 0) {
              fprintf(ferr, "WARNING: deprecated command-line option "
                  "-tok=xml, use -tok=pic instead\n");
              opt_tok = TOKENIZER_PIC;
            }
            else if (strcasecmp(optarg, "xml_counts") == 0) {
              fprintf(ferr, "WARNING: deprecated command-line option "
                  "-tok=xml_counts, use -tok=pic_counts instead\n");
              opt_tok = TOKENIZER_PIC;
            }
            else fprintf(ferr, "WARNING: unknown tokenizer mode \"%s\": using 'tok=string'\n", optarg);
          }
          break;
      case OPTION_JXCHG_DUMP:
          opt_jxchg_dir = optarg;
          if (*(opt_jxchg_dir.end()--) != '/')
            opt_jxchg_dir += '/';
          break;

      case OPTION_COMMENT_PASSTHROUGH:
          if(optarg != NULL)
            opt_comment_passthrough =
              strtoint(optarg, "as argument to -comment-passthrough");
          else
              opt_comment_passthrough = 1;
          break;

      case OPTION_PREDICT_LES:
          if (optarg != NULL)
            opt_predict_les = strtoint(optarg, "as argument to -predict-les");
          else
            opt_predict_les = 1;
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
          opt_tok = TOKENIZER_YY;
          break;
#endif
        }
    }

  if(optind != argc - 1)
    {
      fprintf(ferr, "parse_options(): expecting name of grammar to load\n");
      return false;
    }
  grammar_file_name = argv[optind];

  if(opt_hyper && opt_compute_qc)
  {
      fprintf(ferr, "quickcheck computation doesn't work "
              "in hyperactive mode, disabling hyperactive mode.\n");
      opt_hyper = false;
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
      opt_nsolutions = 1;
  else
      opt_nsolutions = 0;
  verbosity = int_setting(set, "verbose");
  opt_pedgelimit = int_setting(set, "limit");
  opt_memlimit = 1024 * 1024 * int_setting(set, "memlimit");
  opt_timeout = sysconf(_SC_CLK_TCK) * int_setting(set, "timeout");
  opt_hyper = bool_setting(set, "hyper");
  opt_default_les = bool_setting(set, "default-les");
  opt_predict_les = int_setting(set, "predict-les");
#ifdef YY
  opt_yy = bool_setting(set, "yy");
  if(bool_setting(set, "one-meaning"))
    opt_nth_meaning = 1;
  else
    opt_nth_meaning = 0;
#endif
}

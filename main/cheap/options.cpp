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

#include "pet-system.h"

#include "getopt.h"
#include "fs.h"
#include "cheap.h"
#include "options.h"
#include "../common/utility.h"

bool opt_shrink_mem, opt_shaping, opt_default_les,
  opt_filter, opt_print_failure,
  opt_hyper, opt_derivation, opt_rulestatistics, opt_pg,
  opt_linebreaks, opt_chart_man, opt_interactive_morph, opt_lattice,
  opt_nbest, opt_online_morph, opt_fullform_morph;
#ifdef YY
bool opt_yy, opt_k2y_segregation;
int opt_k2y, opt_nth_meaning;
#endif

int opt_nsolutions, opt_nqc, verbosity, pedgelimit, opt_key, opt_server;
int opt_tsdb;
long int memlimit;
char *grammar_file_name = 0;

char *opt_compute_qc = 0;

char *opt_supertag_file = 0;
int opt_supertag_norm;

int opt_packing = 0;

void usage(FILE *f)
{
  fprintf(f, "usage: `cheap [options] grammar-file'; valid options are:\n");
#ifdef TSDBAPI
  fprintf(f, "  `-tsdb[=n]' --- enable tsdb++ slave mode (protocol version = n)\n");
#endif
  fprintf(f, "  `-nsolutions[=n]' --- find best n only, 1 if n is not given\n");
  fprintf(f, "  `-verbose[=n]' --- set verbosity level to n\n");
  fprintf(f, "  `-limit=n' --- maximum number of passive edges\n");
  fprintf(f, "  `-memlimit=n' --- maximum amount of fs memory (in MB)\n");
  fprintf(f, "  `-no-shrink-mem' --- don't shrink process size after huge items\n"); 
  fprintf(f, "  `-no-filter' --- disable rule filter\n"); 
  fprintf(f, "  `-qc=n' --- use only top n quickcheck paths\n");
  fprintf(f, "  `-compute-qc[=file]' --- compute quickcheck paths (output to file,\n"
             "                           default /tmp/qc.tdl)\n");
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
  fprintf(f, "  `-interactive-online-morph' --- morphology only\n");
  fprintf(f, "  `-no-online-morph' --- disable online morphology\n");
#endif
  fprintf(f, "  `-no-fullform-morph' --- disable full form morphology\n");
  fprintf(f, "  `-pg' --- print grammar in ASCII form\n");
  fprintf(f, "  `-nbest' --- n-best parsing mode\n");
  fprintf(f, "  `-supertag=file' --- write supertagged data into file\n");
  fprintf(f, "  `-supertagnorm=n' --- set normalization value to n "
              "(default: 10)\n");
  fprintf(f, "  `-packing=n' --- set packing to n (bit coded)\n");
  fprintf(f, "  `-log=[+]file' --- "
             "log server mode activity to `file' (`+' appends)\n");
}

#define OPTION_TSDB 0
#define OPTION_NSOLUTIONS 1
#define OPTION_VERBOSE 2
#define OPTION_LIMIT 3
#define OPTION_NO_SHRINK_MEM 4
#define OPTION_NO_FILTER 5
#define OPTION_NQC 6
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
#define OPTION_SUPERTAGFILE 20
#define OPTION_SUPERTAGNORM 21
#define OPTION_LATTICE 22
#define OPTION_NBEST 23
#define OPTION_NO_ONLINE_MORPH 24
#define OPTION_NO_FULLFORM_MORPH 25
#define OPTION_PACKING 26

#ifdef YY
#define OPTION_ONE_MEANING 100
#define OPTION_YY 101
#define OPTION_K2Y 102
#define OPTION_K2Y_SEGREGATION 103
#endif

void init_options()
{
  opt_tsdb = 0;
  opt_nsolutions = 0;
  verbosity = 0;
  pedgelimit = 0;
  memlimit = 0;
  opt_shrink_mem = true;
  opt_shaping = true;
  opt_filter = true;
  opt_nqc = -1;
  opt_compute_qc = 0;
  opt_print_failure = false;
  opt_key = 0;
  opt_hyper = 1;
  opt_derivation = true;
  opt_rulestatistics = false;
  opt_default_les = false;
  opt_server = 0;
  opt_pg = false;
  opt_chart_man = true;
  opt_interactive_morph = false;
  opt_lattice = false;
  opt_supertag_file = 0;
  opt_supertag_norm = 10;
  opt_nbest = false;
  opt_online_morph = true;
  opt_fullform_morph = true;
  opt_packing = 0;
#ifdef YY
  opt_yy = false;
  opt_k2y = 0;
  opt_k2y_segregation = false;
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
    {"qc", required_argument, 0, OPTION_NQC},
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
    {"k2y", optional_argument, 0, OPTION_K2Y},
    {"k2y-segregation", no_argument, 0, OPTION_K2Y_SEGREGATION},
#endif
    {"server", optional_argument, 0, OPTION_SERVER},
    {"log", required_argument, 0, OPTION_LOG},
    {"pg", no_argument, 0, OPTION_PG},
    {"interactive-online-morphology", no_argument, 0, OPTION_INTERACTIVE_MORPH},
    {"supertag", required_argument, 0, OPTION_SUPERTAGFILE},
    {"supertagnorm", required_argument, 0, OPTION_SUPERTAGNORM},
    {"lattice", no_argument, 0, OPTION_LATTICE},
    {"nbest", no_argument, 0, OPTION_NBEST},
    {"no-online-morph", no_argument, 0, OPTION_NO_ONLINE_MORPH},
    {"no-fullform-morph", no_argument, 0, OPTION_NO_FULLFORM_MORPH},
    {"packing", optional_argument, 0, OPTION_PACKING},

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
              opt_compute_qc = "/tmp/qc.tdl";
          break;
      case OPTION_PRINT_FAILURE:
          opt_print_failure = true;
          break;
      case OPTION_PG:
          opt_pg = true;
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
      case OPTION_NQC:
          if(optarg != NULL)
              opt_nqc = strtoint(optarg, "as argument to `-qc'");
          break;
      case OPTION_KEY:
          if(optarg != NULL)
              opt_key = strtoint(optarg, "as argument to `-key'");
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
      case OPTION_SUPERTAGFILE:
          if(optarg != NULL)
              opt_supertag_file = strdup(optarg);
          break;
      case OPTION_SUPERTAGNORM:
          if(optarg != NULL)
              opt_supertag_norm = strtoint(optarg, "as argument to -supertagnorm");
          break;
      case OPTION_NBEST:
          opt_nbest = true;
          break;
      case OPTION_NO_ONLINE_MORPH:
          opt_online_morph = false;
          break;
      case OPTION_NO_FULLFORM_MORPH:
          opt_fullform_morph = false;
          break;
      case OPTION_PACKING:
          if(optarg != NULL)
              opt_packing = strtoint(optarg, "as argument to `-packing'");
          else
              opt_packing = 0xff;
          break;
#ifdef YY
      case OPTION_ONE_MEANING:
          if(optarg != NULL)
              opt_nth_meaning = strtoint(optarg, "as argument to -one-meaning");
          else
              opt_nth_meaning = 1;
          break;
      case OPTION_K2Y:
          if(optarg != NULL)
              opt_k2y = strtoint(optarg, "as argument to -k2y");
          else
              opt_k2y = 50;
          break;
      case OPTION_K2Y_SEGREGATION:
          opt_k2y_segregation = true;
          break;
      case OPTION_YY:
          opt_yy = true;
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
  pedgelimit = int_setting(set, "limit");
  memlimit = 1024 * 1024 * int_setting(set, "memlimit");
  opt_hyper = bool_setting(set, "hyper");
  opt_default_les = bool_setting(set, "default-les");
#ifdef YY
  opt_k2y = int_setting(set, "k2y");
  opt_yy = bool_setting(set, "yy");
  opt_k2y_segregation = bool_setting(set, "k2y-segregation");
  if(bool_setting(set, "one-meaning"))
    opt_nth_meaning = 1;
  else
    opt_nth_meaning = 0;
#endif
}

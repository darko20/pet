/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* command line options */

#include <stdlib.h>

#include "getopt.h"
#include "fs.h"
#include "cheap.h"
#include "options.h"
#include "utility.h"

bool opt_one_solution, opt_shrink_mem, opt_shaping, opt_default_les,
  opt_filter, opt_compute_qc, opt_print_failure,
  opt_hyper, opt_derivation, opt_rulestatistics, opt_tsdb, opt_pg,
  opt_linebreaks, opt_chart_man;
#ifdef YY
bool opt_one_meaning, opt_yy;
int unsigned opt_k2y;
#endif

int opt_nqc, verbosity, pedgelimit, opt_key, opt_server;
char *grammar_file_name;


void usage(FILE *f)
{
  fprintf(f, "usage: `cheap [options] grammar-file'; valid options are:\n");
#ifdef TSDBAPI
  fprintf(f, "  `-tsdb' --- self explanatory, isn't it?\n");
#endif
  fprintf(f, "  `-one-solution' --- non exhaustive search\n");  
  fprintf(f, "  `-verbose[=n]' --- set verbosity level to n\n");
  fprintf(f, "  `-limit=n' --- maximum number of passive edges\n");
  fprintf(f, "  `-no-shrink-mem' --- don't shrink process size after huge items\n"); 
  fprintf(f, "  `-no-filter' --- disable rule filter\n"); 
  fprintf(f, "  `-qc=n' --- use only top n quickcheck paths\n");
  fprintf(f, "  `-compute-qc' --- compute quickcheck paths\n");
  fprintf(f, "  `-key=n' --- select key mode (0=key-driven, 1=l-r, 2=r-l, 3=head-driven)\n");
  fprintf(f, "  `-no-hyper' --- disable hyper-active parsing\n");
  fprintf(f, "  `-no-derivation' --- disable output of derivations\n");
  fprintf(f, "  `-rulestats' --- enable tsdb output of rule statistics\n");
  fprintf(f, "  `-no-chart-man' --- disable chart manipulation\n");
  fprintf(f, "  `-server[=n]' --- go into server mode, bind to port `n' (default: 4711)\n");
  fprintf(f, "  `-default-les' --- enable use of default lexical entries)\n");
#ifdef YY
  fprintf(f, "  `-k2y[=n]' --- "
          "output K2Y, filter at `n' %% of raw atoms (default: 50)\n");
  fprintf(f, "  `-one-meaning' --- non exhaustive search for first valid semantic formula\n");
  fprintf(f, "  `-yy' --- YY input mode (highly experimental)\n");
#endif
  fprintf(f, "  `-failure-print' --- print failure paths\n");
  fprintf(f, "  `-pg' --- print grammar in ASCII form\n");
  fprintf(f, "  `-log=[+]file' --- "
             "log server mode activity to `file' (`+' appends)\n");
}

#define OPTION_TSDB 0
#define OPTION_ONE_SOLUTION 1
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

#ifdef YY
#define OPTION_ONE_MEANING 100
#define OPTION_YY 101
#define OPTION_K2Y 102

#endif

bool parse_options(int argc, char* argv[])
{
  int c,  res;

  struct option options[] = {
#ifdef TSDBAPI
    {"tsdb", no_argument, 0, OPTION_TSDB},
#endif
    {"one-solution", no_argument, 0, OPTION_ONE_SOLUTION},
    {"verbose", optional_argument, 0, OPTION_VERBOSE},
    {"limit", required_argument, 0, OPTION_LIMIT},
    {"no-shrink-mem", no_argument, 0, OPTION_NO_SHRINK_MEM},
    {"no-filter", no_argument, 0, OPTION_NO_FILTER},
    {"qc", required_argument, 0, OPTION_NQC},
    {"compute-qc", no_argument, 0, OPTION_COMPUTE_QC},
    {"failure-print", no_argument, 0, OPTION_PRINT_FAILURE},
    {"key", required_argument, 0, OPTION_KEY},
    {"no-hyper", no_argument, 0, OPTION_NO_HYPER},
    {"no-derivation", no_argument, 0, OPTION_NO_DERIVATION},
    {"rulestats", no_argument, 0, OPTION_RULE_STATISTICS},
    {"no-chart-man", no_argument, 0, OPTION_NO_CHART_MAN},
    {"default-les", no_argument, 0, OPTION_DEFAULT_LES},
#ifdef YY
    {"yy", no_argument, 0, OPTION_YY},
    {"one-meaning", no_argument, 0, OPTION_ONE_MEANING},
    {"k2y", optional_argument, 0, OPTION_K2Y},
#endif
    {"server", optional_argument, 0, OPTION_SERVER},
    {"log", required_argument, 0, OPTION_LOG},
    {"pg", no_argument, 0, OPTION_PG},
    {0, 0, 0, 0}
  }; /* struct option */

  opt_tsdb = false;
  opt_one_solution = false;
  verbosity = 0;
  pedgelimit = 0;
  opt_shrink_mem = true;
  opt_shaping = true;
  opt_filter = true;
  opt_nqc = -1;
  opt_compute_qc = false;
  opt_print_failure = false;
  opt_key = 0;
  opt_hyper = 1;
  opt_derivation = true;
  opt_rulestatistics = false;
  opt_default_les = false;
  opt_server = 0;
  opt_pg = false;
  opt_chart_man = true;
#ifdef YY
  opt_yy = false;
  opt_k2y = 0;
  opt_one_meaning = false;
#endif
  if(!argc) return true;

  while((c = getopt_long_only(argc, argv, "", options, &res)) != EOF)
    {
      switch(c)
        {
        case '?':
          return false;
        case OPTION_TSDB:
          opt_tsdb = true;
          break;
        case OPTION_ONE_SOLUTION:
          opt_one_solution = true;
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
          opt_compute_qc = true;
          break;
        case OPTION_PRINT_FAILURE:
          opt_print_failure = true;
          break;
        case OPTION_PG:
          opt_pg = true;
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
        case OPTION_LOG:
          if(optarg != NULL) 
            if(optarg[0] == '+') flog = fopen(&optarg[1], "a");
            else flog = fopen(optarg, "w");
          break;
#ifdef YY
        case OPTION_ONE_MEANING:
          opt_one_meaning = true;
          break;
        case OPTION_K2Y:
          if(optarg != NULL) 
            opt_k2y = strtoint(optarg, "as argument to -k2y");
          else
            opt_k2y = 50;
          break;
        case OPTION_YY:
          opt_yy = true;
          break;
#endif
        }
    }

  if(optind != argc - 1)
    {
      fprintf(stderr, "parse_options(): expecting name of grammar to load\n");
      return false;
    }
  grammar_file_name = argv[optind];

  return true;
}

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

#include <stdlib.h>

#include "flop.h"
#include "getopt.h"
#include "utility.h"
#include "options.h"

bool opt_pre, opt_expand_all_instances,
  opt_full_expansion, opt_unfill, opt_minimal, opt_no_sem,
  opt_propagate_status, opt_linebreaks, opt_glbdebug;

int verbosity;
int errors_to;

char *grammar_file_name;

void usage(FILE *f)
{
  fprintf(f, "usage: `flop [options] tdl-file'; valid options are:\n");
  fprintf(f, "  `-pre' --- do only syntactic preprocessing\n");
  fprintf(f, "  `-expand-all-instances' --- expand all (even lexicon) instances\n");
  fprintf(f, "  `-full-expansion' --- don't do partial expansion\n");
  fprintf(f, "  `-no-unfill' --- don't unfill after expansion\n");
  fprintf(f, "  `-minimal' --- minimal fixed arity encoding\n");
  fprintf(f, "  `-propagate-status' --- propagate status the PAGE way\n");
  fprintf(f, "  `-no-semantics' --- remove all semantics\n");
  fprintf(f, "  `-glbdebug' --- print information about glb types created\n");
  fprintf(f, "  `-verbose[=n]' --- set verbosity level to n\n");
  fprintf(f, "  `-errors-to=n' --- print errors to fd n\n");
}

#define OPTION_PRE 0
#define OPTION_EXPAND_ALL_INSTANCES 3
#define OPTION_FULL_EXPANSION 4
#define OPTION_NO_UNFILL 5
#define OPTION_VERBOSE 6
#define OPTION_ERRORS_TO 7
#define OPTION_MINIMAL 8
#define OPTION_NO_SEM 9
#define OPTION_PROPAGATE_STATUS 10
#define OPTION_GLBDEBUG 11

bool parse_options(int argc, char* argv[])
{
  int c,  res;

  struct option options[] = {
    {"pre", no_argument, 0, OPTION_PRE},
    {"expand-all-instances", no_argument, 0, OPTION_EXPAND_ALL_INSTANCES},
    {"full-expansion", no_argument, 0, OPTION_FULL_EXPANSION},
    {"no-unfill", no_argument, 0, OPTION_NO_UNFILL},
    {"minimal", no_argument, 0, OPTION_MINIMAL},
    {"no-semantics", no_argument, 0, OPTION_NO_SEM},
    {"propagate-status", no_argument, 0, OPTION_PROPAGATE_STATUS},
    {"glbdebug", no_argument, 0, OPTION_GLBDEBUG},
    {"verbose", optional_argument, 0, OPTION_VERBOSE},
    {"errors-to", required_argument, 0, OPTION_ERRORS_TO},
    {0, 0, 0, 0}
  }; /* struct option */
  
  opt_pre = false;
  opt_expand_all_instances = false;
  opt_full_expansion = false;
  opt_unfill = true;
  opt_minimal = false;
  opt_no_sem = false;
  opt_propagate_status = false;
  opt_linebreaks = false;
  opt_glbdebug = false;

  verbosity = 0;
  errors_to = -1;
  
  while((c = getopt_long_only(argc, argv, "", options, &res)) != EOF)
  {
    switch(c)
    {
    case '?':
      return false;
      break;
    case OPTION_PRE:
      opt_pre = true;
      break;
    case OPTION_NO_UNFILL:
      opt_unfill = false;
      break;
    case OPTION_MINIMAL:
      opt_minimal = true;
      break;
    case OPTION_FULL_EXPANSION:
      opt_full_expansion = true;
      break;
    case OPTION_EXPAND_ALL_INSTANCES:
      opt_expand_all_instances = true;
      break;
    case OPTION_NO_SEM:
      opt_no_sem = true;
      break;
    case OPTION_PROPAGATE_STATUS:
      opt_propagate_status = true;
      break;
    case OPTION_GLBDEBUG:
      opt_glbdebug = true;
      break;
    case OPTION_VERBOSE:
      if(optarg != NULL)
        verbosity = strtoint(optarg, "as argument to `-verbose'");
      else
        verbosity++;
      break;
    case OPTION_ERRORS_TO:
      if(optarg != NULL)
        errors_to = strtoint(optarg, "as argument to `-errors-to'");
      break;
    }
  }
  
  if(optind != argc - 1)
  {
    fprintf(ferr, "parse_options(): expecting name of TDL grammar to process\n");
    return false;
  }
  
  grammar_file_name = argv[optind];
  
  return true;
}
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

#include "flop.h"
#include "getopt.h"
#include "options.h"
#include "version.h"
#include "utility.h"
#include "logging.h"

//int verbosity;
// int errors_to;

void usage(std::ostream &out)
{
  out << "flop version " << version_string << std::endl
      << "usage: `flop [options] tdl-file'; valid options are:" << std::endl
      << "  `-pre' --- do only syntactic preprocessing" << std::endl
      << "  `-expand-all-instances' --- expand all (even lexicon) instances"
      << std::endl
      << "  `-full-expansion' --- don't do partial expansion" << std::endl
      << "  `-unfill' --- unfill after expansion" << std::endl
      << "  `-minimal' --- minimal fixed arity encoding" << std::endl
      << "  `-propagate-status' --- propagate status the PAGE way" << std::endl
      << "  `-no-semantics' --- remove all semantics" << std::endl
      << "  `-glbdebug' --- print information about glb types created"
      << std::endl
      << "  `-cmi=level' --- create morph info, level = 0..2, default 0"
      << std::endl
  //    << "  `-verbose[=n]' --- set verbosity level to n" << std::endl
  //    << "  `-errors-to=n' --- print errors to fd n" << std::endl
    ;
}

#define OPTION_PRE 0
#define OPTION_EXPAND_ALL_INSTANCES 3
#define OPTION_FULL_EXPANSION 4
#define OPTION_UNFILL 5
//#define OPTION_VERBOSE 6
//#define OPTION_ERRORS_TO 7
#define OPTION_MINIMAL 8
#define OPTION_NO_SEM 9
#define OPTION_PROPAGATE_STATUS 10
#define OPTION_GLBDEBUG 11
#define OPTION_CMI 12


char *parse_options(int argc, char* argv[])
{
  int c,  res;

  struct option options[] = {
    {"pre", no_argument, 0, OPTION_PRE},
    {"expand-all-instances", no_argument, 0, OPTION_EXPAND_ALL_INSTANCES},
    {"full-expansion", no_argument, 0, OPTION_FULL_EXPANSION},
    {"unfill", no_argument, 0, OPTION_UNFILL},
    {"minimal", no_argument, 0, OPTION_MINIMAL},
    {"no-semantics", no_argument, 0, OPTION_NO_SEM},
    {"propagate-status", no_argument, 0, OPTION_PROPAGATE_STATUS},
    {"cmi", required_argument, 0, OPTION_CMI},
    //{"verbose", optional_argument, 0, OPTION_VERBOSE},
    //{"errors-to", required_argument, 0, OPTION_ERRORS_TO},
    {0, 0, 0, 0}
  }; /* struct option */
  
  managed_opt("opt_expand_all_instances",
    "expand  expand all type definitions, except for pseudo types", false);
  managed_opt("opt_minimal", "", false);
  managed_opt("opt_no_sem", "", false);


  //verbosity = 0;
  //errors_to = -1;
  
  while((c = getopt_long_only(argc, argv, "", options, &res)) != EOF)
  {
    switch(c)
    {
    case '?':
      return NULL;
      break;
    case OPTION_PRE:
      set_opt("opt_pre", true);
      break;
    case OPTION_UNFILL:
      set_opt("opt_unfill", true);
      break;
    case OPTION_MINIMAL:
      set_opt("opt_minimal", true);
      break;
    case OPTION_FULL_EXPANSION:
      set_opt("opt_full_expansion", true);
      break;
    case OPTION_EXPAND_ALL_INSTANCES:
      set_opt("opt_expand_all_instances", true);
      break;
    case OPTION_NO_SEM:
      set_opt("opt_no_sem", true);
      break;
    case OPTION_PROPAGATE_STATUS:
      set_opt("opt_propagate_status", true);
      break;
    case OPTION_CMI:
      if(optarg != NULL)
        set_opt_from_string("opt_cmi", optarg);
      break;
    /*
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
    */
    }
  }
  
  if(optind != argc - 1)
  {
    LOG(root, ERROR,
        "parse_options(): expecting name of TDL grammar to process");
    return NULL;
  }
  
  return argv[optind];
}

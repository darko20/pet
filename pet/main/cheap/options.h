/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* command line options */

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <stdio.h>

#include "settings.h"

#define CHEAP_SERVER_PORT 4711

extern bool opt_one_solution, opt_shrink_mem, opt_shaping, opt_default_les,
  opt_filter, opt_compute_qc, opt_print_failure,
  opt_hyper, opt_derivation, opt_rulestatistics, opt_tsdb, opt_pg,
  opt_linebreaks, opt_chart_man;
#ifdef YY
extern bool opt_one_meaning, opt_yy;
extern unsigned int opt_k2y;
#endif
extern int verbosity, pedgelimit, opt_nqc, opt_key, opt_server;
extern long int memlimit;
extern bool opt_linebreaks, opt_chart_man;
extern char *grammar_file_name;

void usage(FILE *f);

#ifndef __BORLANDC__
bool parse_options(int argc, char* argv[]);
#endif

void options_from_settings(settings *);

#endif

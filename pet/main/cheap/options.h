/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* command line options */

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include "settings.h"

#define CHEAP_SERVER_PORT 4711

extern bool opt_shrink_mem, opt_shaping, opt_default_les,
  opt_filter, opt_compute_qc, opt_print_failure,
  opt_hyper, opt_derivation, opt_rulestatistics, opt_tsdb, opt_pg,
  opt_linebreaks, opt_chart_man, opt_interactive_morph, opt_lattice,
  opt_nbest;
#ifdef YY
extern bool opt_yy, opt_k2y_segregation;
extern int opt_k2y, opt_nth_meaning;
#endif
extern int opt_nsolutions, verbosity, pedgelimit, opt_nqc, opt_key, opt_server;
extern long int memlimit;
extern bool opt_linebreaks, opt_chart_man, opt_interactive_morph;
extern char *grammar_file_name;

extern char *opt_supertag_file;
extern int opt_supertag_norm;

void usage(FILE *f);

#ifndef __BORLANDC__
bool parse_options(int argc, char* argv[]);
#endif

void options_from_settings(settings *);

#endif

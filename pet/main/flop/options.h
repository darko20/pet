/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* command line options */

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <stdio.h>

extern bool opt_pre, opt_chic, opt_expand, opt_expand_all_instances,
  opt_full_expansion, opt_shrink, opt_minimal, opt_no_sem,
  opt_propagate_status, opt_linebreaks, opt_glbdebug;

extern int verbosity;
extern int errors_to;

extern char *grammar_file_name;

void usage(FILE *f);
bool parse_options(int argc, char* argv[]);

#endif

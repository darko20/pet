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
extern bool opt_linebreaks, opt_chart_man, opt_interactive_morph, opt_online_morph, opt_fullform_morph;
extern char *grammar_file_name;

extern char *opt_supertag_file;
extern int opt_supertag_norm;

#define PACKING_EQUI  (1 << 0);
#define PACKING_PRO   (1 << 1);
#define PACKING_RETRO (1 << 2);

extern int opt_packing;

void usage(FILE *f);

#ifndef __BORLANDC__
bool parse_options(int argc, char* argv[]);
#endif

void options_from_settings(settings *);

#endif

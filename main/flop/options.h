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

#include <stdio.h>

extern bool opt_pre, opt_chic, opt_expand, opt_expand_all_instances,
  opt_full_expansion, opt_unfill, opt_minimal, opt_no_sem,
  opt_propagate_status, opt_linebreaks, opt_glbdebug, opt_cmi;

extern int verbosity;
extern int errors_to;

extern char *grammar_file_name;

void usage(FILE *f);
bool parse_options(int argc, char* argv[]);

#endif

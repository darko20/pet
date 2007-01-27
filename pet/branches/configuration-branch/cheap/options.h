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

/** \file options.h
 * Command line options for cheap
 * @todo opt_fullform_morph is obsolete
 * @todo opt_comment_passthrough was initially int, but later it was changed
 *       to bool (during integration with configuration system). That changed
 *       behaviour of the program.
 */

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include "pet-config.h"
#include "settings.h"

#include "Configuration.h"

#define CHEAP_SERVER_PORT 4711

extern bool opt_shrink_mem, opt_shaping;

#ifdef YY
extern bool opt_yy, opt_nth_meaning;
#endif
extern int verbosity, pedgelimit, opt_nqc_unif, opt_nqc_subs, opt_server;
extern int opt_tsdb;

extern long int memlimit;
extern bool opt_linebreaks;
// extern bool opt_fullform_morph;
extern char *grammar_file_name;

enum tokenizer_id { 
  TOKENIZER_INVALID, TOKENIZER_STRING
  , TOKENIZER_YY, TOKENIZER_YY_COUNTS
  , TOKENIZER_XML, TOKENIZER_XML_COUNTS
  , TOKENIZER_FSR, TOKENIZER_SMAF
} ;

extern std::string opt_tsdb_dir, opt_jxchg_dir;

#define PACKING_EQUI  (1 << 0)
#define PACKING_PRO   (1 << 1)
#define PACKING_RETRO (1 << 2)
#define PACKING_SELUNPACK (1 << 3)
#define PACKING_NOUNPACK (1 << 7)

void usage(FILE *f);

#ifndef __BORLANDC__
bool parse_options(int argc, char* argv[]);
#endif

void options_from_settings(settings *);

#endif

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
 */

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include "pet-config.h"
#include "settings.h"

#define CHEAP_SERVER_PORT 4711

/** grammar file name */
extern char *grammar_file_name;

/** compute MRS (modes: C implementation, LKB bindings via ECL; default: no) */
extern char *opt_mrs;

/** @name Operating modes
 * If none of the following options is provided, cheap will go into the usual
 * parsing mode.
 */
//@{
/** print grammar */
extern int opt_pg;
/** [incr tsdb()] mode */
extern int opt_tsdb;
/** socket server mode (value: port or 0)*/
extern int opt_server;
/** interactive morphology (apply only morphological rules, without lexicon) */
extern bool opt_interactive_morph;
//@}

enum tokenizer_id {
  TOKENIZER_INVALID, TOKENIZER_STRING
  , TOKENIZER_YY, TOKENIZER_YY_COUNTS
  , TOKENIZER_PIC, TOKENIZER_PIC_COUNTS
  , TOKENIZER_FSR, TOKENIZER_SMAF
  , TOKENIZER_FSC
};
/** @name Tokenization and preprocessing options */
//@{
/** choose tokenizer */
extern tokenizer_id opt_tok;
/** allow input comments.
 * 2004/03/12 Eric Nichols <eric-n@is.naist.jp> */
extern int opt_comment_passthrough;
/**
 * If set to \c true, turn on the chart mapping engine on the input chart (aka
 * `token mapping') and on the lexical chart (aka `lexical filtering').  also
 * used to select debugging levels for chart mapping, with a bit-mapped scheme:
 * (to be determined).
 */
extern int opt_chart_mapping;
//@}

enum default_les_modes { 
  NO_DEFAULT_LES = 0, DEFAULT_LES_POSMAP_LEXGAPS, DEFAULT_LES_ALL
};
/** @name Unknown word handling */
//@{
/** generic lexical entries */
extern default_les_modes opt_default_les;
/** lexical type predictor */
extern int opt_predict_les;
//@}

/** @name Ambiguity packing modes */
//@{
#define PACKING_EQUI  (1 << 0)
#define PACKING_PRO   (1 << 1)
#define PACKING_RETRO (1 << 2)
#define PACKING_SELUNPACK (1 << 3)
#define PACKING_PCFGEQUI (1 << 4)
#define PACKING_NOUNPACK (1 << 7)
//@}

/** @name Performance tuning */
//@{
/** choose ambiguity packing mode. see Oepen & Carroll 2000a,b */
extern int opt_packing;
/** restrict search to the first n solutions */
extern int opt_nsolutions;
/** chart shape filter: would an item fit into the available chart cells? */
extern bool opt_shaping;
/** rule application filter (see Kiefer et al 1999) */
extern bool opt_filter;
/** hyper-active parsing (see Oepen & Carroll 2000b) */
extern bool opt_hyper;
/** allow/disallow chart manipulation (currently only dependency filter) */
extern bool opt_chart_man;
/** key mode (0=key-driven, 1=l-r, 2=r-l, 3=head-driven) */
extern int opt_key;
//@}

/** @name Quick check
 * see Oepen & Carroll 2000a,b
 */
//@{
/** compute quickcheck paths (path to results file or 0) */
extern char *opt_compute_qc;
/** compute quickcheck paths (unification) */
extern bool opt_compute_qc_unif;
/** compute quickcheck paths (subsumption) */
extern bool opt_compute_qc_subs;
/** use only top n quickcheck paths (unification) */
extern int opt_nqc_unif;
/** use only top n quickcheck paths (subsumption) */
extern int opt_nqc_subs;
//@}

/** @name Resources */
//@{
/** memory limit (in MB) for parsing and unpacking */
extern long int opt_memlimit;
/** timeout limit (in s) for parsing and unpacking */
extern int opt_timeout;
/** passive edge limit */
extern int opt_pedgelimit;
/** allow/disallow dag memory to shrink */
extern bool opt_shrink_mem;
//@}

/** @name Output control */
//@{
/** verbosity level */
extern int verbosity;
/** print unification failures */
extern bool opt_print_failure;
/** print derivations */
extern bool opt_derivation;
/** print rule statistics */
extern bool opt_rulestatistics;
/** print at most n (full) results */
extern int opt_nresults;
/** print partial results in case of parse failure */
extern bool opt_partial;
/** print robust PCFG parsing result in case of full parse failure */
extern int opt_robust;
/** dump [incr tsdb()] profiles to this directory */
extern std::string opt_tsdb_dir;
//@}

/** use computational morphology */
extern bool opt_online_morph;

/** select statistical model for parse ranking */
extern std::string opt_sm;

/** word lattice parsing (permissible token paths, cf tPath class) */
extern bool opt_lattice;

/** grand-parenting level in MEM-based parse selection.
 * 2006/10/01 Yi Zhang <yzhang@coli.uni-sb.de> */
extern unsigned int opt_gplevel;

/** ??? */ // TODO description of this option needed
extern std::string opt_jxchg_dir;

/** ??? */ // TODO description of this option needed
extern bool opt_linebreaks;

#ifdef YY
/** ??? */ // TODO description of this option needed
extern bool opt_yy;
/** ??? */ // TODO description of this option needed
extern int opt_nth_meaning;
#endif

void usage(FILE *f);

#ifndef __BORLANDC__
bool parse_options(int argc, char* argv[]);
#endif

void options_from_settings(settings *);

#endif

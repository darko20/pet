/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* dumped form of a grammar */

#ifndef _GRAMMAR_DUMP_H_
#define _GRAMMAR_DUMP_H_

#include <dumper.h>

#define DUMP_MAGIC 0x03422711
#define DUMP_VERSION 13

// version history
// 1 -> 2 : introduced featset information for each type (2-nov-99)
// 2 -> 3 : introduced new field headdtr to rule_dump (5-nov-99)
// 3 -> 4 : new: feattable info after featset info (15-nov-99)
// 4 -> 5 : unexpanded (qc) structures now have a negative type value (20-jan-00)
// 5 -> 6 : also dump appropriate types for attributes (20-jan-00)
// 6 -> 7 : dump number of bytes wasted per type by encoding (9-feb-00)
// 7 -> 8 : dump dags depth first, root is now last node in dump (28-may-00)
// 8 -> 9 : major cleanup - byteorder independence, bitcode compression (12-jun-00)
// 9 -> 10 : major cleanup, part II (13-jun-00)
// 10 -> 11 : specify encoding type
// 11 -> 12 : encode types as int rather than short in some places
// 12 -> 13 : major cleanup. no more instances and symbols, store status value
//            for all types instead; do not store waste information;
//            store morphological rules

// general layout of a dumped grammar (`.gram' file)
// 
// - header
// - type bit codes
// - symbol tables
// - grammar rules
// - lexicon entries
// - type constraints

void dump_header(dumper *f, char *desc);
char *undump_header(dumper *f);

#endif

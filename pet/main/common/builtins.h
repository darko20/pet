/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* PET builtins */

#ifndef _BUILTINS_H_
#define _BUILTINS_H_

#include <stdio.h>
#include "settings.h"

/* builtin types */
extern int BI_TOP;

/* special types and attributes */

extern int BI_SYMBOL, BI_STRING, BI_CONS, BI_LIST, BI_NIL, BI_DIFF_LIST;
extern int BIA_FIRST, BIA_REST, BIA_LIST, BIA_LAST, BIA_ARGS;

// initialize special types and attributes according to settings file
// defined in types.cpp
extern void initialize_specials(settings *);

#endif

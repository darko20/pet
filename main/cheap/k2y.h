//
// The entire contents of this file, in printed or electronic form, are
// (c) Copyright YY Software Corporation, La Honda, CA 2000, 2001.
// Unpublished work -- all rights reserved -- proprietary, trade secret
//

// K2Y semantics representation (dan, uc, oe)

#ifndef _K2Y_H_
#define _K2Y_H_

#include "fs.h"
#include "item.h"
#include "mrs.h"

extern "C" {
#  include "mfile.h"
}

//
// the roles in k2y
//

enum k2y_role {K2Y_SENTENCE, K2Y_MAINVERB,
                K2Y_SUBJECT, K2Y_DOBJECT, K2Y_IOBJECT,
                K2Y_VCOMP, K2Y_MODIFIER, K2Y_INTARG,
                K2Y_QUANTIFIER, K2Y_CONJ, K2Y_NNCMPND, K2Y_PARTICLE,
                K2Y_SUBORD, 
                K2Y_XXX};

extern char *k2name[K2Y_XXX];

//
// external entry point to this module: construct (at the moment: print)
// k2y representation from root, if eval is true just return number of
// relations in k2y representation built
//

int construct_k2y(int, item *, bool, struct MFILE *);

#endif


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

// K2Y semantics representation (dan, uc, oe)

#ifndef _K2Y_H_
#define _K2Y_H_

#include "fs.h"
#include "item.h"
#include "mrs.h"
#include "mfile.h"

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


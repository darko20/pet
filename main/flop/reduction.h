/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/** \file reduction.h
 * Transitive reduction of an acyclic directed graph.
 */

#ifndef _REDUCTION_H_
#define _REDUCTION_H_

#include "hierarchy.h"

void
acyclicTransitiveReduction(tHierarchy &G);

#endif

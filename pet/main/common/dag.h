/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* wrapper for the various dag implementations */

#include "dag-common.h"

#if defined(DAG_SIMPLE)
#if defined(WROBLEWSKI3)
#define DESTRUCTIVE_UNIFIER
#else
#error "No DAG_SIMPLE instantiation specified"
#endif
#include "dag-simple.h"
#elif defined(DAG_TOMABECHI)
# define QDESTRUCTIVE_UNIFIER
# define DAG_FAILURES
# include "dag-tomabechi.h"
#endif

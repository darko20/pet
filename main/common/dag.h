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

/** \file dag.h
 * Wrapper for the various dag implementations.
 */

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
# include "dag-tomabechi.h"
#endif

#define DAG_FORMAT_TRADITIONAL 0
#define DAG_FORMAT_FED 1
#define DAG_FORMAT_LUI 2

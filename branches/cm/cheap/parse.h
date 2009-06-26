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

/** \file parse.h
 * Hyperactive agenda-driven chart parser.
 */

#ifndef _PARSE_H_
#define _PARSE_H_

#include "item.h"
#include "errors.h"
#include "cheaptimer.h"
#include <string>
#include <list>
#include <vector>

/** @name Ambiguity packing modes */
//@{
#define PACKING_EQUI  (1 << 0)
#define PACKING_PRO   (1 << 1)
#define PACKING_RETRO (1 << 2)
#define PACKING_SELUNPACK (1 << 3)
#define PACKING_PCFGEQUI (1 << 4)
#define PACKING_NOUNPACK (1 << 7)
//@}

/** Gives the total time spent in parsing (more specifically: in the function
 *  parse_loop).
 */
extern timer TotalParseTime;

extern clock_t timeout;
extern clock_t timestamp;

/** Take some kind of input, run it through the registered preprocessing
 *  modules first and then do syntactic parsing.
 * \param input Any kind of input feasible for the registered preprocessing
 * \param C The chart, passed from the outside so as to be able to analyze it
 *          afterwards.
 * \param FSAS An feature structure allocation state, gives control over the
 *             allocated memory during and after parsing.
 * \param errors The errors occured during parsing, e.g., resource exhaustion
 * \param id A unique id for this parse.
 */
void analyze(std::string input, class chart *&C, class fs_alloc_state &FSAS,
             std::list<tError> &errors, int id = 0);

/** selective unpacking */
// int unpack_selectively(std::vector<tItem*> &trees, int upedgelimit, int nsolutions
//                        ,timer *UnpackTime , std::vector<tItem *> &readings);
int unpack_selectively(std::vector<tItem*> &trees, int upedgelimit,
                       long memlimit,
                       timer *UnpackTime , std::vector<tItem *> &readings);

/** exhaustive unpacking */
int unpack_exhaustively(std::vector<tItem*> &trees, int upedgelimit,
                        long memlimit,
                        timer *UnpackTime, std::vector<tItem *> &readings);

#endif

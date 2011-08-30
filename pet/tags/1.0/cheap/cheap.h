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

/** \file cheap.h 
 * Global variables for cheap.
 */

#ifndef _CHEAP_H_
#define _CHEAP_H_

#include "errors.h"

/** The global settings for this process, read from the settings file */
extern class settings *cheap_settings;
/** The file stream to direct log messages to.
 * \todo replace this with a general logging facility, e.g., log4cxx.
 */
extern FILE *flog;
/** The global grammar for this process, which means: only one language/grammar
 *  per CHEAP process.
 */
extern class tGrammar *Grammar;

/** The golbal VPM for this process */
extern class tVPM *vpm;
#endif

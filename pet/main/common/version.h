/* -*- Mode: C++ -*- 
 * PET
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

/** \file version.h
 * PET version information for use in flop and cheap
 */

#ifndef _VERSION_H
#define _VERSION_H

#include "pet-config.h"

#define VERSION_DATETIME "$LastChangedDate$"
#define VERSION_CHANGE "$Change: 850 $"

/** The version string for the current PET version. */
extern char *version_string;

/** The perforce change string for the current PET version.
 * This should correlate with the version string. It is mainly there to check
 * that the version number reflects the state of the package.
 */
extern char *version_change_string;

#endif

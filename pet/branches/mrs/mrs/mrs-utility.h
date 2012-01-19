/* Mode: -*- C++ -*-
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

/** \file utility.h
 * Helper functions.
 */

#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <cstring>
#include <list>
#include <string>
#include <cstdio>

/** @name Case Conversion
 * Destructively convert string to all lower/upper case.
 */
/*@{*/
void strtolower(char *s);
void strtoupper(char *s);
/*@}*/

/** @name Nondestructive Case Conversion
 * Nondesctructively convert and copy string to all lower/upper case.
 */
/*@{*/
void strtolower(char *dest, const char *src);
void strtoupper(char *dest, const char *src);
/*@}*/
/** Escape xml delimiters. */
std::string xml_escape(std::string s);


#endif

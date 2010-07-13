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

/** \file builtins.h
 * PET builtin types. The special types and attributes in this file have to be
 * defined appropriately in the settings file for grammar processing with flop
 * and cheap.
 */

#ifndef _BUILTINS_H_
#define _BUILTINS_H_

class settings;

/** Topmost type of the hierarchy */
extern int BI_TOP;

/** Special types: symbol, string, cons, list, nil (empty list) and difference
 *  list
 */
extern int BI_SYMBOL, BI_STRING, BI_CONS, BI_LIST, BI_NIL, BI_DIFF_LIST;

/** Special attributes: FIRST, REST (for lists) , LIST, LAST (for difference
 *  lists, ARGS, for list of rule arguments.
 */
extern int BIA_FIRST, BIA_REST, BIA_LIST, BIA_LAST, BIA_ARGS;

/** Initialize special types and attributes according to settings file.
 *  defined in types.cpp
 */
extern void initialize_specials(settings *);

#endif

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

/* parser */

#ifndef _PARSE_H_
#define _PARSE_H_

#include "tsdb++.h"

extern class settings *cheap_settings;

extern class tGrammar *Grammar;

extern timer TotalParseTime;

bool filter_rule_task(class grammar_rule *R, class item *passive);
bool filter_combine_task(class item *active, class item *passive);

void analyze(class input_chart &ic, string input, class chart *&C, 
	     class fs_alloc_state &FSAS, list<tError> &errors, int id = 0);

#endif

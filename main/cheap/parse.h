/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* parser */

#ifndef _PARSE_H_
#define _PARSE_H_

#include "tsdb++.h"

extern class settings *cheap_settings;

extern class grammar *Grammar;

extern timer TotalParseTime;

bool filter_rule_task(class grammar_rule *R, class item *passive);
bool filter_combine_task(class item *active, class item *passive);

void analyze(class input_chart &ic, string input, class chart *&C, 
	     class agenda *&R, class fs_alloc_state &FSAS, int id = 0);

#endif

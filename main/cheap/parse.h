/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* parser */

#ifndef _PARSE_H_
#define _PARSE_H_

#include "grammar.h"
#include "tokenlist.h"
#include "chart.h"
#include "agenda.h"
#include "settings.h"
#include "tsdb++.h"

extern settings *cheap_settings;

extern grammar *Grammar;
extern chart *Chart;
extern agenda *Agenda;
extern timer TotalParseTime;

bool filter_rule_task(grammar_rule *R, item *passive);
bool filter_combine_task(item *active, item *passive);

void parse(chart&, tokenlist*, int id);

#endif

/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class `agenda' */

#include "agenda.h"

//#define DEBUG

void agenda::push(basic_task *t)
{
#ifdef DEBUG
  fprintf(ferr, " -> ");
  t->print(ferr);
  fprintf(ferr, "\n");
#endif

#ifdef SIMPLE_AGENDA
  _A.push_front(t);
#else
  _A.push(t);
#endif
}

basic_task *agenda::pop()
{
  basic_task *t;
#ifdef SIMPLE_AGENDA
  t = _A.front();
  _A.pop_front();
#else
  t = _A.top();
  _A.pop();
#endif
  return t;
}

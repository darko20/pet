/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class `agenda' */

#include "pet-system.h"
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

basic_task *agenda::top()
{
  basic_task *t;
#ifdef SIMPLE_AGENDA
  t = _A.front();
#else
  t = _A.top();
#endif
  return t;
}

basic_task *agenda::pop()
{
  basic_task *t = top();

#ifdef SIMPLE_AGENDA
  _A.pop_front();
#else
  _A.pop();
#endif

  if(t->plateau() && !empty() && top()->priority() == t->priority())
    {
      basic_task *same_plateau = pop(); // recursive
      same_plateau->priority(MAX_PRIORITY);
      push(same_plateau);
    }

  return t;
}

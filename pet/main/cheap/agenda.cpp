/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class `agenda' */

#include "pet-system.h"
#include "agenda.h"

//#define DEBUG
#define PLATEAU_PARSING

void agenda::push(basic_task *t)
{
#ifdef DEBUG
  fprintf(ferr, " -> ");
  t->print(ferr);
  fprintf(ferr, "\n");
#endif

  _A.push(t);
}

basic_task *agenda::top()
{
  basic_task *t;
  t = _A.top();
  return t;
}

basic_task *agenda::pop()
{
  basic_task *t = top();

  _A.pop();

#ifdef PLATEAU_PARSING
  if(t->plateau() && !empty() && top()->priority() == t->priority())
    {
      basic_task *same_plateau = pop(); // recursive
      same_plateau->priority(MAX_TASK_PRIORITY);
      push(same_plateau);
    }
#endif

  return t;
}

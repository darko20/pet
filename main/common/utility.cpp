/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* general helper functions and classes */

#include <time.h>

#include "utility.h"
#include "errors.h"

void *salloc(size_t size)
{
  void *p = malloc(size);

  if(!p)
    throw error("out of memory");
  
  return p;
}

#ifdef STRDUP
char *strdup(const char *s)
{
  char *s1;

  s1 = (char *) salloc(strlen(s) + 1);
  strcpy(s1, s);
  return s1;
}
#endif

void strtolower(char *s)
{
  if(s == NULL) return;
  while(*s)
  {
    *s = tolower(*s);
    s++;
  }
}

void strtoupper(char *s)
{
  if(s == NULL) return;
  while(*s)
  {
    *s = toupper(*s);
    s++;
  }
}

int strtoint(const char *s, const char *errloc, bool quotedp)
{
  char *endptr = 0;
  char *foo = NULL;
  if(quotedp)
    {
      if(!*s == '"' || (foo = strrchr(s, '"')) == NULL)
	throw error(string("invalid quoted integer `") + string(s) +
		    string("' ") + string(errloc));
      s++;
    }
  int val = strtol(s, &endptr, 10);
  if(endptr == 0 || (quotedp ? endptr != foo :  *endptr != '\0'))
    throw error(string("invalid integer `") + string(s) + string("' ") 
                + string(errloc));

  return val;
}


char *current_time(void) {

  time_t foo;
  struct tm *now;
  static char *result = (char *)NULL;
  
  if(result == NULL) {
    if((result = (char *)malloc(42)) == NULL) return("now");
  } /* if */

  if((foo = time(&foo)) > 0 && (now = localtime(&foo)) != NULL) {
    sprintf(result, "%d-%d-%d %d:%02d:%02d",
            now->tm_mday, now->tm_mon + 1, now->tm_year + 1900,
            now->tm_hour, now->tm_min, now->tm_sec);
  } /* if */
  else {
    sprintf(result, "now");
  } /* else */
  return(result);

} /* current_time() */

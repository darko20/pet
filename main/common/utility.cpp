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
  const char *foo = NULL;
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

char *current_time(void)
{
  time_t foo = time(0);
  struct tm *now = localtime(&foo);
  static char *result = new char[80];

  if(result == 0)
    return "now";

  if(foo > 0 && now != 0)
    strftime(result, 80, "%c", now);
  else
    sprintf(result, "now");

  return(result);
}

#ifdef __BORLANDC__

#include <alloc.h>

void print_borland_heap(FILE *f)
{
  struct heapinfo hi;
  if(heapcheck() == _HEAPCORRUPT)
    {
      fprintf(f, "Heap is corrupted.\n");
      return;
    }

   hi.ptr = 0;
   fprintf(f, "   Block  Size   Status\n");
   fprintf(f, "   -----  ----   ------\n");
   while(heapwalk( &hi ) == _HEAPOK)
   {
     fprintf(f, "%7u    %s\n", hi.size, hi.in_use ? "used" : "free");
   }
}

#endif

/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* helpers */

#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <stdio.h>

// allocates .size. bytes of memory - throw error when out of memory
void *salloc(size_t size);

#ifdef STRDUP
// duplicate string
char *strdup(const char *s);
#endif

// destructively convert string to all lower/upper case
void strtolower(char *s);
void strtoupper(char *s);

// convert string to integer, throw error msg if invalid
extern int strtoint(const char *s, const char *errloc, bool = false);

// return current date and time in static string; client must not free()
char *current_time(void);

#ifdef __BORLANDC__
void print_borland_heap(FILE *f);
#endif

#endif

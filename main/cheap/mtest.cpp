/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* test chunk allocator */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "chunk-alloc.h"

int main()
{
  chunk_allocator down(4096*1280, true);
  chunk_allocator up(4096*1280, false);

  long l = sysconf(_SC_PAGESIZE);
  
  fprintf(stderr, "page size: %ld\n", l);

  void *p1, *p2;

  int i = 1;
  while(1)
    {
      p1 = down.allocate(1280 * i);
      p2 = up.allocate(1280 * i);

      fprintf(stderr, "down: %8xd, up: %8xd\n", (int) p1, (int) p2);
      
      i = ((i+1) % 5) + 1;
    }


}

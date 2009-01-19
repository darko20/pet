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

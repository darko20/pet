/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* support functions for dumping/undumping of the grammar */

#include <stdlib.h>
#include <string.h>

#include "errors.h"
#include "grammar-dump.h"

void dump_header(dumper *f, char *desc)
{
  f->dump_int(DUMP_MAGIC);
  f->dump_int(DUMP_VERSION);
  f->dump_string(desc);
}

char * undump_header(dumper *f)
{
  int magic, version;
  char *desc;

  magic = f->undump_int();

  if(magic != DUMP_MAGIC)
    throw error("not a valid grammar file");

  version = f->undump_int();
  if(version != DUMP_VERSION)
    throw error("grammar file has incompatible version");
  
  desc = f->undump_string();

  return desc;
}

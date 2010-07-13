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

/* sample program to illustrate structure of .grm file */

#include "grammar.h"
#include "dumper.h"
#include "errors.h"
#include "grammar-dump.h"

void dump_gram(const char *filename)
{
  // Instantiate object to undump the specified file
  dumper dmp(filename);

  // Read the header information
  int version;
  char *id = undump_header(&dmp, version);
  if(id) printf("%s v%d (%s)\n", filename, version, id);

  // Instantiate object to undump Table Of Contents
  dump_toc toc(&dmp);

  // symbol tables
  if(toc.goto_section(SEC_SYMTAB))
  {
    printf("[SYMTAB @ %ld]\n", dmp.tell());

    int nstatus = dmp.undump_int();
    int first_leaftype = dmp.undump_int();
    int nleaftypes = dmp.undump_int();
    int nattrs = dmp.undump_int();

    printf("  %d status values\n", nstatus);
    printf("  %d types (%d leaftypes)\n", first_leaftype + nleaftypes,
           nleaftypes);
    printf("  %d attributes\n", nattrs);
  }

  // print names
  if(toc.goto_section(SEC_PRINTNAMES))
  {
    printf("[PRINTNAMES @ %ld]\n", dmp.tell());
  }

  // hierarchy
  if(toc.goto_section(SEC_HIERARCHY))
  {
    printf("[HIERARCHY @ %ld]\n", dmp.tell());
  }

  // feature tables
  if(toc.goto_section(SEC_FEATTABS))
  {
    printf("[FEATTABS @ %ld]\n", dmp.tell());
  }
  
  // full forms
  if(toc.goto_section(SEC_FULLFORMS))
  {
    printf("[FULLFORMS @ %ld]\n", dmp.tell());
    int nffs = dmp.undump_int();
    printf("  %d full forms\n", nffs);
  }
  
  // inflectional rules
  if(toc.goto_section(SEC_INFLR))
  {
    printf("[INFLR @ %ld]\n", dmp.tell());
    int ninflrs = dmp.undump_int();
    printf("  %d inflectional rules\n", ninflrs);
  }

  // constraints
  if(toc.goto_section(SEC_CONSTRAINTS))
  {
    printf("[CONSTRAINTS @ %ld]\n", dmp.tell());
  }

  // irregular forms
  if(toc.goto_section(SEC_IRREGS))
  {
    printf("[IRREGS @ %ld]\n", dmp.tell());
    int nirregs = dmp.undump_int();
    printf("  %d irregular forms\n", nirregs);
  }
}

int main(int argc, char* argv[])
{
  try
  { 
    if(argc > 1)
      dump_gram(argv[1]);
    else
      throw tError("No grammar file specified");
  }

  catch(tError &e)
  {
    fprintf(stderr, "%s\n", e.getMessage().c_str());
    exit(1);
  }

  return 0;
}

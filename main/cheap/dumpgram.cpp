/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* sample program to illustrate structure of .grm file */

#include "pet-system.h"

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
      throw error("No grammar file specified");
  }

  catch(error &e)
  {
    e.print(stderr);  printf("\n");
    exit(1);
  }

  return 0;
}

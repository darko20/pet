/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* dumping the grammar to binary form for cheap*/

#include <vector>
#include <map>
#include "flop.h"
#include "utility.h"
#include "types.h"
#include "dumper.h"
#include "options.h"
#include "dag.h"

void dump_symbol_tables(dumper *f)
{
  // nstatus
  f->dump_int(nstatus);

  // npropertypes
  f->dump_int(first_leaftype);

  // nleaftypes
  f->dump_int(nleaftypes);

  // nattrs
  f->dump_int(nattrs);

  // status names
  for(int i = 0; i < nstatus; i++)
    f->dump_string(statusnames[i]);

  // type names and status
  for(int i = 0; i < ntypes; i++)
    {
      f->dump_string(typenames[rleaftype_order[i]]);
      f->dump_int(typestatus[rleaftype_order[i]]);
    }

  // attribute names
  for(int i = 0; i < nattrs; i++)
    f->dump_string(attrname[i]);
}

void dump_tables(dumper *f)
{
  // write encoding type
  if(opt_minimal == false)
    f->dump_int(0);
  else
    f->dump_int(1);

  // write set for each type
  for(int i = 0; i < first_leaftype; i++)
    {
      f->dump_int(featset[rleaftype_order[i]]);
    }

  f->dump_int(nfeatsets);
  // write descriptor for each set
  for(int i = 0; i < nfeatsets; i++)
    {
      short int n = featsetdesc[i].n;
      f->dump_short(n);
      for(int j = 0; j < n; j++)
	f->dump_short(featsetdesc[i].attr[j]);
    }

  for(int i = 0; i < nattrs; i++)
    f->dump_int(leaftype_order[apptype[i]]);
}

void dump_print_names(dumper *f)
{
  // print names
  for(int i = 0; i < ntypes; i++)
    {
      if(strcmp(printnames[rleaftype_order[i]],
		typenames[rleaftype_order[i]]) != 0)
	f->dump_string(printnames[rleaftype_order[i]]);
      else
	f->dump_string(0);
    }
}

void dump_fullforms(dumper *f)
{
  f->dump_int(fullforms.size());
  
  for(list<ff_entry>::iterator currentff = fullforms.begin();
      currentff != fullforms.end(); ++currentff)
    currentff->dump(f);
}

void dump_inflr(dumper *f, int t, char *r)
{
  f->dump_int(t);
  f->dump_string(r);
}

void dump_inflrs(dumper *f)
{
  int ninflr = 0;
  int ninflr_var = f->dump_int_variable();

  ninflr++;
  dump_inflr(f, -1, global_inflrs);

  for(int i = 0; i < ntypes; i++)
    {
      if(types[i]->inflr != 0)
	{
	  ninflr++;
	  dump_inflr(f, i, types[i]->inflr);
	}
    }
  
  f->set_int_variable(ninflr_var, ninflr);
}

void dump_irregs(dumper *f)
{
  f->dump_int(irregforms.size());
  for(list<irreg_entry>::iterator it = irregforms.begin();
      it != irregforms.end(); ++it)
    it->dump(f);
}

int kbwritten(dumper *f)
{
  static long int lpos = 0;

  long int diff = f->tell() - lpos;
  lpos = f->tell();

  return diff / 1024;
}

void dump_grammar(dumper *f, char *desc)
{
  dump_header(f, desc);

  dump_toc_maker toc(f);
  toc.add_section(SEC_SYMTAB);
  toc.add_section(SEC_PRINTNAMES);
  toc.add_section(SEC_HIERARCHY);
  toc.add_section(SEC_FEATTABS);
  toc.add_section(SEC_FULLFORMS);
  toc.add_section(SEC_INFLR);
  toc.add_section(SEC_IRREGS);
  toc.add_section(SEC_CONSTRAINTS);
  toc.close();

  toc.start_section(SEC_SYMTAB);
  dump_symbol_tables(f);

  toc.start_section(SEC_PRINTNAMES);
  dump_print_names(f);

  fprintf(fstatus, "symbols %dk", kbwritten(f));

  toc.start_section(SEC_HIERARCHY);
  dump_hierarchy(f);

  toc.start_section(SEC_FEATTABS);
  dump_tables(f);

  fprintf(fstatus, ", hierarchy %dk", kbwritten(f));

  toc.start_section(SEC_FULLFORMS);
  dump_fullforms(f);

  toc.start_section(SEC_INFLR);
  dump_inflrs(f);

  toc.start_section(SEC_IRREGS);
  dump_irregs(f);

  fprintf(fstatus, ", lexicon %dk", kbwritten(f));

  toc.start_section(SEC_CONSTRAINTS);

  for(int i = 0; i < types.number(); i++)
    dag_dump(f, types[rleaftype_order[i]]->thedag);

  fprintf(fstatus, ", types %dk", kbwritten(f));

}

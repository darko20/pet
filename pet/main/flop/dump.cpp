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

/* dumping the grammar to binary form for cheap*/

#include "hierarchy.h"
#include "dumper.h"
#include "flop.h"
#include "dag.h"
#include "grammar-dump.h"
#include "logging.h"
#include "configs.h"

using std::list;

/** dump a bunch of string pairs that describe various properties of the
 *  grammar.
 */
void
dump_properties(dumper *f)
{
    // nproperties
    f->dump_int(grammar_properties.size());
    
    for(std::map<std::string, std::string>::iterator it =
            grammar_properties.begin(); it != grammar_properties.end(); ++it)
    {
        f->dump_string(it->first.c_str());
        f->dump_string(it->second.c_str());
    }
}

/** Dump information about the number of status values, leaftypes, types, and
 *  attributes as well as the tables of status value names, type names and
 *  their status, and attribute names.
 */
void
dump_symbol_tables(dumper *f)
{
  // nstatus
  f->dump_int(nstatus);

  // npropertypes
  f->dump_int(first_leaftype);

  // nstaticleaftypes
  f->dump_int(nstaticleaftypes);

  // nattrs
  f->dump_int(nattrs);

  // status names
  for(int i = 0; i < nstatus; i++)
    f->dump_string(statusnames[i]);

  // type names and status
  for(int i = 0; i < nstatictypes; i++)
    {
      f->dump_string(type_name(leaftype_order[i]));
      f->dump_int(typestatus[leaftype_order[i]]);
    }

  // attribute names
  for(int i = 0; i < nattrs; i++)
    f->dump_string(attrname[i]);
}

/** Dump the type-to-featureset mappings and the feature sets for fixed arity
 *  encoding, as well as the table of appropriate types for all attributes.
 */
void
dump_tables(dumper *f)
{
  // write encoding type
  if(!get_opt_bool("opt_minimal"))
    f->dump_int(0);
  else
    f->dump_int(1);

  // write set for each type
  // _fix_doc_ Why only for non-leaftypes ??
  for(int i = 0; i < first_leaftype; i++)
    {
      f->dump_int(featset[leaftype_order[i]]);
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
    f->dump_int(rleaftype_order[apptype[i]]);
}


/** write (immediate) supertypes for each type */
void
dump_supertypes(dumper *f)
{
  for(int i = 0; i < first_leaftype; i++)
  {
      list<int> supertypes = immediate_supertypes(leaftype_order[i]);
      f->dump_short(supertypes.size());
      for(list<int>::iterator it = supertypes.begin(); it != supertypes.end();
          ++it)
          f->dump_int(rleaftype_order[*it]);
  }
}

/** \brief Dump print name for every type. The print name is only dumped if it
 *  differs from the type name, which is primarily true for instances, where a
 *  '$' character is prepended to the typename.
 */
void
dump_print_names(dumper *f)
{
  // print names
  for(int i = 0; i < nstatictypes; i++)
    {
      if(get_printname(leaftype_order[i]) != get_typename(leaftype_order[i]))
        f->dump_string(print_name(leaftype_order[i]));
      else
        f->dump_string(0);
    }
}

/** Dump the full form table. Here, dumping is delegated to the full form
 *   objects.
 */
void
dump_fullforms(dumper *f)
{
  f->dump_int(fullforms.size());
  
  for(list<ff_entry>::iterator currentff = fullforms.begin();
      currentff != fullforms.end(); ++currentff)
    currentff->dump(f);
}

/** Dump a single inflection rule, first the type of the feature structure rule
 *  and then the string representation of the transformation rule.
 */
void
dump_inflr(dumper *f, int t, char *r)
{
  f->dump_int(t);
  f->dump_string(r);
}

/** Dump all inflection rules. */
void
dump_inflrs(dumper *f)
{
  int ninflr = 0;
  int ninflr_var = f->dump_int_variable();

  if(global_inflrs != 0)
  {
      ninflr++;
      dump_inflr(f, -1, global_inflrs);
  }
  
  for(int i = 0; i < nstatictypes; i++)
    {
      if(types[i]->inflr != 0)
        {
          ninflr++;
          dump_inflr(f, rleaftype_order[i], types[i]->inflr);
        }
    }
  
  f->set_int_variable(ninflr_var, ninflr);
}

/** Dump the irregular forms table. Here, dumping is delegated to the irregular
 *  form objects.
 */
void
dump_irregs(dumper *f)
{
  f->dump_int(irregforms.size());
  for(list<irreg_entry>::iterator it = irregforms.begin();
      it != irregforms.end(); ++it)
    it->dump(f);
}

/** Return the number of kilobytes written since the last call of this
 *  function.
 */
int
kbwritten(dumper *f)
{
  static long int lpos = 0;

  long int diff = f->tell() - lpos;
  lpos = f->tell();

  return diff / 1024;
}

void logkb(const char *what, dumper *f) {
  LOG(logApplC, INFO, what << " " << kbwritten(f) << "k");
}

/** Dump the whole grammar to a binary data file.
 * \param f low-level dumper class
 * \param desc readable description of the current grammar
 */
void
dump_grammar(dumper *f, const char *desc)
{
  dump_header(f, desc);

  dump_toc_maker toc(f);
  toc.add_section(SEC_PROPERTIES);
  toc.add_section(SEC_SYMTAB);
  toc.add_section(SEC_PRINTNAMES);
  toc.add_section(SEC_HIERARCHY);
  toc.add_section(SEC_FEATTABS);
  toc.add_section(SEC_SUPERTYPES);
  toc.add_section(SEC_FULLFORMS);
  toc.add_section(SEC_INFLR);
  toc.add_section(SEC_IRREGS);
  toc.add_section(SEC_CONSTRAINTS);
  toc.close();

  toc.start_section(SEC_PROPERTIES);
  dump_properties(f);

  toc.start_section(SEC_SYMTAB);
  dump_symbol_tables(f);

  toc.start_section(SEC_PRINTNAMES);
  dump_print_names(f);
  logkb("symbols", f);

  toc.start_section(SEC_HIERARCHY);
  dump_hierarchy(f);
  logkb(", hierarchy", f);

  toc.start_section(SEC_FEATTABS);
  dump_tables(f);
  logkb(", feature tables", f);

  toc.start_section(SEC_SUPERTYPES);
  dump_supertypes(f);
  logkb(", supertypes", f);

  toc.start_section(SEC_FULLFORMS);
  dump_fullforms(f);

  toc.start_section(SEC_INFLR);
  dump_inflrs(f);

  toc.start_section(SEC_IRREGS);
  dump_irregs(f);

  logkb(", lexicon", f);

  toc.start_section(SEC_CONSTRAINTS);

  for(int i = 0; i < types.number(); i++)
    dag_dump(f, types[leaftype_order[i]]->thedag);
  logkb(", types", f);

  toc.dump();

}

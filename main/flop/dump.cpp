/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* dumping the grammar to binary form for cheap*/

#include <vector>
#include "flop.h"
#include "utility.h"
#include "types.h"
#include "dumper.h"
#include "options.h"
#include "dag.h"
#include "grammar-dump.h"

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

void dump_rules(dumper *f)
{
  struct type *t;
  list<int> rules;

  if(verbosity > 4)
    {
      fprintf(fstatus, "\ntable of rules:\n");
    }

  // construct list `rules' containing id's of all types that are rules
  for(int i = 0; i < types.number(); i++)
    if((t = types[i])->status == RULE_STATUS && t->tdl_instance)
      rules.push_back(i);

  f->dump_int(rules.size());

  for(list<int>::iterator currentrule = rules.begin();
      currentrule != rules.end(); ++currentrule)
    {
      struct dag_node *dag, *head_dtr;
      list <struct dag_node *> argslist;
      
      dag = dag_get_path_value(types[*currentrule]->thedag,
			       flop_settings->req_value("rule-args-path"));
      argslist = dag_get_list(dag);
      head_dtr = dag_get_path_value(types[*currentrule]->thedag,
				    flop_settings->req_value("head-dtr-path"));

      int n = 1, keyarg = -1, head = -1;
      for(list<dag_node *>::iterator currentdag = argslist.begin();
	  currentdag != argslist.end(); ++ currentdag)
	{
	  if(flop_settings->lookup("keyarg-marker-path"))
	    {
	      struct dag_node *k;

	      k = dag_get_path_value(*currentdag, flop_settings->value("keyarg-marker-path"));
	  
	      if(k != FAIL && dag_type(k) ==
		 types.id(flop_settings->req_value("true-type")))
		keyarg = n;
	    }
	  
          if(*currentdag == head_dtr)
            head = n;

	  n++;
	}

      if(flop_settings->lookup("rule-keyargs"))
	{
	  if(keyarg != -1)
	    {
	      fprintf(ferr, "warning: both keyarg-marker-path and rule-keyargs supply information on key argument...\n");
	    }
	  char *s = flop_settings->assoc("rule-keyargs", types.name(*currentrule).c_str());
	  if(s && strtoint(s, "in `rule-keyargs'"))
	    {
	      keyarg = strtoint(s, "in `rule-keyargs'");
	    }
	}

      if(verbosity > 4)
	{
	  fprintf(fstatus, "%s/%d (key: %d, head: %d)\n",
		  types.name(*currentrule).c_str(), argslist.size(), keyarg, head);
	}
      
      f->dump_int(*currentrule);
      f->dump_char(argslist.size());
      f->dump_char(keyarg);
      f->dump_char(head);
    }
}

void dump_le(dumper *f, const morph_entry &M)
{
  int preterminal;
  int affix;
  int inflpos;

  preterminal = types.id(M.preterminal);
  if(preterminal == -1)
    {
      fprintf(ferr, "unknown preterminal `%s'\n", M.preterminal.c_str());
    }
  preterminal = leaftype_order[preterminal];

  affix = types.id(M.affix);
  if(affix != -1)
    affix = leaftype_order[affix];

  if(!M.affix.empty() && affix == -1)
    {
      fprintf(ferr, "affix `%s' is not known\n", M.affix.c_str());
    }

  inflpos = M.inflpos == 0 ? 0 : M.inflpos - 1;
  
  f->dump_int(preterminal);
  f->dump_int(affix);
  f->dump_char(inflpos);
  f->dump_char(M.nstems);

  // construct list `orth' containing surface words
  vector<string> orth;
  
  if(M.nstems > 1)
    { // this is a multi word lexeme

      orth = get_le_stems(types[preterminal]->thedag);

      orth[inflpos] = string(M.form);
    }
  else
    {
      orth.push_back(string(M.form));
    }

  for(vector<string>::iterator currentword = orth.begin();
      currentword != orth.end(); ++currentword)
    f->dump_string(currentword->c_str());
}

void dump_lexicon(dumper *f)
{
  f->dump_int(vocabulary.size());
  
  for(list<morph_entry>::iterator currentle = vocabulary.begin();
      currentle != vocabulary.end(); ++currentle)
    {
      dump_le(f, *currentle);
    }
}

int toc_hierarchy, toc_tables, toc_rules, toc_lexicon, toc_constraints;

void dump_toc(dumper *f)
{
  toc_hierarchy = f->dump_int_variable();
  toc_tables = f->dump_int_variable();
  toc_rules = f->dump_int_variable();
  toc_lexicon = f->dump_int_variable();
  toc_constraints = f->dump_int_variable();
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
  dump_toc(f);
  dump_symbol_tables(f);

  fprintf(fstatus, "symbols %dk", kbwritten(f));

  f->set_int_variable(toc_hierarchy, f->tell());
  dump_hierarchy(f);

  f->set_int_variable(toc_tables, f->tell());
  dump_tables(f);
  
  fprintf(fstatus, ", hierarchy %dk", kbwritten(f));

  f->set_int_variable(toc_rules, f->tell());
  dump_rules(f);

  f->set_int_variable(toc_lexicon, f->tell());
  dump_lexicon(f);

  fprintf(fstatus, ", rules & lexicon %dk", kbwritten(f));

  f->set_int_variable(toc_constraints, f->tell());

  for(int i = 0; i < types.number(); i++)
    dag_dump(f, types[rleaftype_order[i]]->thedag);

  fprintf(fstatus, ", types %dk", kbwritten(f));
}

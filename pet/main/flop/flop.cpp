/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* main module (preprocessor) */

#include <assert.h>
#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include <algorithm>

#include <LEDA/graph.h>
#include <LEDA/graph_misc.h> 
#include <LEDA/graph_iterator.h>
#include <LEDA/sortseq.h>

#include "flop.h"
#include "types.h"
#include "options.h"

/*** global variables ***/

symtab<struct type *> types;
symtab<int> statustable;
symtab<struct templ *> templates;
symtab<int> attributes;

char *global_inflrs = 0;

settings *flop_settings = 0;

/*** variables local to module */

static char *grammar_version;

void check_undefined_types()
{
  for(int i = 0; i < types.number(); i++)
    {
      if(types[i]->implicit)
	{
	  lex_location *loc = types[i]->def;

	  if(loc == 0)
	    loc = new_location("unknown", 0, 0);

	  fprintf(ferr, "warning: type `%s' (introduced at %s:%d) has no definition\n",
		  types.name(i).c_str(),
		  loc->fname, loc->linenr);
	}
    }
}

void process_conjunctive_subtype_constraints()
{
  int i, j;
  struct type *t;
  struct conjunction *c;
  
  for(i = 0; i<types.number(); i++)
    if((c = (t = types[i])->constraint) != 0)
      for(j = 0; j < c->n; j++)
	if( c->term[j]->tag == TYPE)
	  {
	    t->parents = cons(c->term[j]->type, t->parents);
	    subtype_constraint(t->id, c->term[j]->type);
	    c->term[j--] = c->term[--c->n];
	  }
}

char *synth_type_name(int offset = 0)
{
  char name[80];
  sprintf(name, "synth%d", types.number() + offset);

  if(types.id(name) != -1)
    return synth_type_name(offset + (int) (10.0*rand()/(RAND_MAX+1.0)));

  return strdup(name);
}

void process_multi_instances()
{
  int i, n = types.number();
  struct type *t;

  map<list_int *, int, list_int_compare> ptype;

  for(i = 0; i < n; i++)
    if((t = types[i])->tdl_instance && length(t->parents) > 1)
      {
	if(verbosity > 4)
	  {
	    fprintf(fstatus, "TDL instance `%s' has multiple parents: ",
		    types.name(i).c_str());
	    for(list_int *l = t->parents; l != 0; l = rest(l))
	      fprintf(fstatus, " %s", types.name(first(l)).c_str());
	    fprintf(fstatus, "\n");
	  }

	if(ptype[t->parents] == 0)
	  {
	    char *name = synth_type_name();
	    struct type *p = new_type(name, false);
	    p->def = new_location("synthesized", 0, 0);
	    p->parents = t->parents;
	    
	    for(list_int *l = t->parents; l != 0; l = rest(l))
	      subtype_constraint(p->id, first(l));

	    ptype[t->parents] = p->id;

	    if(verbosity > 4)
	      fprintf(fstatus, "Synthesizing new parent type `%d'\n", p->id); 
	  }

	undo_subtype_constraints(t->id);
	subtype_constraint(t->id, ptype[t->parents]);
	
	t->parents = cons(ptype[t->parents], 0);
      }
}

void propagate_status()
{
  struct type *t, *chld;
  leda_edge e;

  fprintf(fstatus, "- status values\n");

  TOPO_It it(hierarchy);
  while(it.valid())
    {
      t = types[hierarchy.inf(it.get_node())];

      if(t->status != NO_STATUS)
	forall_out_edges(e, type_node[t->id])
	  {
	    chld = types[hierarchy.inf(hierarchy.target(e))];
	    
	    if(chld->defines_status == 0)
	      if(chld->status == NO_STATUS || !flop_settings->member("weak-status-values", statustable.name(t->status).c_str()))
		{
		  if(chld->status != NO_STATUS &&
		     chld->status != t->status &&
		     !flop_settings->member("weak-status-values", statustable.name(chld->status).c_str()))
		    {
		      fprintf(ferr, "`%s': status `%s' from `%s' overwrites old status `%s' from `%s'...\n",
			      types.name(chld->id).c_str(),
			      statustable.name(t->status).c_str(),
			      types.name(t->id).c_str(),
			      statustable.name(chld->status).c_str(),
			      types.name(chld->status_giver).c_str());
		    }
		  
		  chld->status_giver = t->id;
		  chld->status = t->status;
		}
	  }

      ++it;
    }
}

int *leaftype_order = 0;
int *rleaftype_order = 0;

void reorder_leaftypes()
// we want all leaftypes in one consecutive range after all nonleaftypes
{
  int i;

  leaftype_order = (int *) malloc(ntypes * sizeof(int));
  for(i = 0; i < ntypes; i++)
    leaftype_order[i] = i;

  int curr = ntypes - 1;

  for(i = 0; i < ntypes && i < curr; i++)
    {
      // look for next non-leaftype starting downwards from curr
      while(leaftypeparent[curr] != -1 && curr > i) curr--;

      if(leaftypeparent[i] != -1)
        {
          leaftype_order[i] = curr;
          leaftype_order[curr] = i;
          curr --;
        }
    }

  rleaftype_order = (int *) malloc(ntypes * sizeof(int));
  for(i = 0; i < ntypes; i++) rleaftype_order[i] = -1;

  for(i = 0; i < ntypes; i++) rleaftype_order[leaftype_order[i]] = i;

  for(i = 0; i < ntypes; i++)
    {
      if(rleaftype_order[i] == -1)
	{
	  fprintf(stderr, "conception error in leaftype reordering\n");
	  exit(1);
	}
      if(rleaftype_order[i] != leaftype_order[i])
	fprintf(stderr, "gotcha: %d: %d <-> %d", i, leaftype_order[i], rleaftype_order[i]);
    }
}

void assign_printnames()
{
  for(int i = 0; i < types.number(); i++)
    if(types[i]->printname == 0)
      types[i]->printname = strdup(types.name(i).c_str());
}

void preprocess_types()
{
  expand_templates();

  find_corefs();
  process_conjunctive_subtype_constraints();
  process_multi_instances();

  process_hierarchy();

  if(opt_propagate_status)
    propagate_status();

  assign_printnames();
}

void log_types(char *title)
{
  fprintf(stderr, "------ %s\n", title);
  for(int i = 0; i < types.number(); i++)
    {
      fprintf(stderr, "\n--- %s[%d]:\n", typenames[i], i);
      dag_print(stderr, types[i]->thedag);
    }
}

void demote_instances()
{
  // for TDL instances we want the parent's type in the fs
  for(int i = 0; i < types.number(); i++)
    {
      if(types[i]->tdl_instance)
	{
	  // all instances should be leaftypes
	  if(leaftypeparent[i] == -1)
	    {
	      fprintf(stderr, "warning: tdl instance `%s' is not a leaftype\n",
		      types.name(i).c_str());
	      // assert(leaftypeparent[i] != -1);
	    }
	  else
	    dag_set_type(types[i]->thedag, leaftypeparent[i]);
	}
    }
}

void process_types()
{
  fprintf(fstatus, "- building dag representation\n");
  unify_wellformed = false;
  dagify_symtabs();
  dagify_types();
  reorder_leaftypes();

  if(verbosity > 9)
    log_types("after creation");

  if(!compute_appropriateness())
    {
      fprintf(ferr, "non maximal introduction of features\n");
      exit(1);
    }
  
  if(!apply_appropriateness())
    {
      fprintf(ferr, "non well-formed feature structures\n");
      exit(1);
    }

  if(!delta_expand_types())
    exit(1);

  if(!fully_expand_types())
    exit(1);

  compute_maxapp();
  
  if(opt_shrink)
    shrink_types();

  demote_instances();

  if(verbosity > 9)
    log_types("before dumping");

  compute_feat_sets(opt_minimal);
}

char *parse_version()
{
  char *fname;
  char *version = 0;

  fname = flop_settings->value("version-file");
  if(fname)
    {
      fname = find_file(fname, SET_EXT);
      if(!fname) return 0;

      push_file(fname, "reading");
      while(LA(0)->tag != T_EOF)
        {
          if(LA(0)->tag == T_ID && flop_settings->member("version-string", LA(0)->text))
            {
              consume(1);
              if(LA(0)->tag != T_STRING)
                {
                  fprintf(ferr, "string expected for version at %s:%d\n",
                          LA(0)->loc->fname, LA(0)->loc->linenr);
                }
              else
                {
                  version = LA(0)->text; LA(0)->text = 0;
                }
            } 
          consume(1);
        }
      consume(1);
    }
  else
    {
      return 0;
    }

  return version;
}

void initialize_status()
{
  // NO_STATUS
  statustable.add("*none*");
  // ATOM_STATUS
  statustable.add("*atom*");
}

extern int dag_dump_grand_total_nodes, dag_dump_grand_total_atomic,
  dag_dump_grand_total_arcs;

void process(char *ofname)
{
  char *fname, *outfname;
  FILE *outf;

  clock_t t_start = clock();

  fname = find_file(ofname, TDL_EXT);
  
  if(!fname)
    {
      fprintf(ferr, "file `%s' not found - skipping...\n", ofname);
      return;
    }

  initialize_builtins();

  flop_settings = new settings("flop", fname);

  if(flop_settings->member("output-style", "stefan"))
    {
      opt_linebreaks = true;
    }

  grammar_version = parse_version();
  if(grammar_version == 0) grammar_version = "unknown";

  outfname = output_name(fname, TDL_EXT, opt_pre ? PRE_EXT : GRAMMAR_EXT);
  outf = fopen(outfname, "wb");
  
  if(outf)
    {
      struct setting *set;
      int i;

      initialize_specials(flop_settings);
      initialize_status();

      fprintf(fstatus, "\nconverting `%s' (%s) into `%s' ...\n", fname, grammar_version, outfname);
      
      if((set = flop_settings->lookup("postload-files")) != 0)
	for(i = set->n - 1; i >= 0; i--)
	  {
	    char *fname;
	    fname = find_file(set->values[i], TDL_EXT);
	    if(fname) push_file(fname, "postloading");
	  }
      
      push_file(fname, "loading");
      
      if((set = flop_settings->lookup("preload-files")) != 0)
	for(i = set->n - 1; i >= 0; i--)
	  {
	    char *fname;
	    fname = find_file(set->values[i], TDL_EXT);
	    if(fname) push_file(fname, "preloading");
	  }
      
      tdl_start(1);
      fprintf(fstatus, "\n");

      char *fffname;
      if((fffname = flop_settings->value("fullform-file")) != 0)
	{
	  if((fffname = find_file(fffname, VOC_EXT)))
	    read_morph(fffname);
	}

      char *irregfname;
      if((irregfname = flop_settings->value("irregs-file")) != 0)
	{
	  if((irregfname = find_file(irregfname, IRR_EXT)))
	    read_irregs(irregfname);
	}

      if(!opt_pre)
	check_undefined_types();

      fprintf(fstatus, "\nfinished parsing - %d syntax errors, %d lines in %0.3g s\n",
	      syntax_errors, total_lexed_lines, (clock() - t_start) / (float) CLOCKS_PER_SEC);
      
      fprintf(fstatus, "processing type constraints (%d types):\n",
	      types.number());
      
      t_start = clock();

      if(flop_settings->value("grammar-info") != 0)
	create_grammar_info(flop_settings->value("grammar-info"),
			    grammar_version);
      
      preprocess_types();
      
      if(!opt_pre)
	process_types();

      if(opt_pre)
	{
	  write_pre_header(outf, outfname, fname, grammar_version);
	  write_pre(outf);
	}
      else
	{
	  dumper dmp(outf, true);
	  fprintf(fstatus, "dumping grammar (");
	  dump_grammar(&dmp, grammar_version);
	  fprintf(fstatus, ")\n");
	  if(verbosity > 10)
	    fprintf(fstatus,
		    "%d[%d]/%d (%0.2g) total grammar nodes[atoms]/arcs (ratio) dumped\n",
		    dag_dump_grand_total_nodes, 
		    dag_dump_grand_total_atomic,
		    dag_dump_grand_total_arcs,
		    double(dag_dump_grand_total_arcs)/dag_dump_grand_total_atomic);
	}
      
      fclose(outf);
      
      fprintf(fstatus, "finished conversion - output generated in %0.3g s\n",
	      (clock() - t_start) / (float) CLOCKS_PER_SEC);
    }
  else
    {
      fprintf(ferr, "couldn't open output file `%s' for `%s' - skipping...\n", outfname, fname);
    }
}

FILE *fstatus, *ferr;

void setup_io()
{
  // connect fstatus to fd 2, and ferr to fd errors_to, unless -1, 2 otherwise

  int val;

  val = fcntl(2, F_GETFL, 0);
  if(val < 0)
    {
      perror("setup_io() [status of fd 2]");
      exit(1);
    }

  fstatus = stderr;
  setvbuf(fstatus, 0, _IONBF, 0);

  if(errors_to != -1)
    {
      val = fcntl(errors_to, F_GETFL, 0);
      if(val < 0 && errno == EBADF)
	{
	  ferr = stderr;
	}
      else
	{
	  if(val < 0)
	    {
	      perror("setup_io() [status of fd errors_to]");
	      exit(1);
	    }
	  if((val & O_ACCMODE) == O_RDONLY)
	    {
	      fprintf(stderr, "setup_io(): fd errors_to is read only\n");
	      exit(1);
	    }
	  ferr = fdopen(errors_to, "w");
	}
    }
  else
    ferr = stderr;

  setvbuf(ferr, 0, _IONBF, 0);
}

int main(int argc, char* argv[])
{
  // set up the streams for error and status reports

  ferr = fstatus = stderr; // preliminary setup

  setlocale(LC_ALL, "" );

  if(!parse_options(argc, argv))
    {
      usage(stderr);
      exit(1);
    }

  setup_io();

  try { process(grammar_file_name); }
  catch(error &e)
    {
      e.print(ferr);  fprintf(ferr, "\n");
      exit(1);
    }

  catch(bad_alloc)
    {
      fprintf(ferr, "out of memory\n");
      exit(1);
    }

  catch(...)
    {
      fprintf(ferr, "unknown exception\n");
      exit(1);
    }
  
  return 0;
}

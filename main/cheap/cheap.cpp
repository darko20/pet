/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* main module (standalone parser) */

#include "pet-system.h"
#include "cheap.h"
#include "parse.h"
#include "agenda.h"
#include "chart.h"
#include "fs.h"
#include "tsdb++.h"
#include "mfile.h"
#include "grammar-dump.h"
#include "inputchart.h"

#ifdef DAG_FAILURES
#include "qc.h"
#endif

#ifdef YY
#include "yy.h"
#include "k2y.h"
#endif

string get_input(FILE *);
void interactive();
void process(char *);

FILE *ferr, *fstatus, *flog;

const int ASBS = 4096; // arbitrary, small buffer size

string get_input(FILE *f)
{
  static char buff[ASBS];

  if(fgets(buff, ASBS, f) == NULL)
    return string();
  
  if(buff[0] == '\0' || buff[0] == '\n')
    return string();

  buff[strlen(buff) - 1] = '\0';

  return string(buff);
}

// global variables for parsing

grammar *Grammar;
settings *cheap_settings;

#ifdef ONLINEMORPH
#include "morph.h"

void interactive_morph()
{
  morph_analyzer *m = Grammar->morph();

  string input;
  while(!(input = get_input(stdin)).empty())
    {
#if 1
      list<morph_analysis> res = m->analyze(input);
      for(list<morph_analysis>::iterator it = res.begin(); it != res.end(); ++it)
	{
	  fprintf(stdout, "%s: ", input.c_str());
	  it->print_lkb(stdout);
	  fprintf(stdout, "\n");
	}
#else
      list<full_form> res = Grammar->lookup_form(input);
      for(list<full_form>::iterator it = res.begin(); it != res.end(); ++it)
	{
	  if(length(it->affixes()) <= 1)
	  fprintf(stdout, "  {\"%s\", \"%s\", NULL, %s%s%s, %d, %d},\n",
		  it->stem()->printname(), it->key().c_str(),
		  it->affixes() ? "\"" : "",
		  it->affixes() ? printnames[first(it->affixes())] : "NULL",
		  it->affixes() ? "\"" : "",
		  it->offset() > 0 ? it->offset() + 1 : it->offset(), it->length()); 
	}
#endif
    }
}
#endif

void interactive()
{
  string input;
  int id = 1;

  while(!(input = get_input(stdin)).empty())
    {
      chart *Chart = 0;
      agenda *Roots = 0;
      try {
	fs_alloc_state FSAS;

	input_chart i_chart(New end_proximity_position_map);

	analyze(i_chart, input, Chart, Roots, FSAS, id);
        
	if(verbosity == -1)
	  fprintf(stdout, "%d\t%d\t%d\n",
		  stats.id, stats.readings, stats.pedges);

        fprintf(fstatus, 
                "(%d) `%s' [%d] --- %d (%.1f|%.1fs) <%d:%d> (%.1fK) [%.1fs]\n",
                stats.id, input.c_str(), pedgelimit, stats.readings, 
                stats.first/1000., stats.tcpu / 1000.,
                stats.words, stats.pedges, stats.dyn_bytes / 1024.0,
		TotalParseTime.elapsed_ts() / 10.);

        if(verbosity > 1) stats.print(fstatus);

#ifdef YY
        if(opt_k2y || verbosity > 1)
#else
        if(verbosity > 1)
#endif

	  {
	    int nres = 0;
	    struct MFILE *mstream = mopen();

	    while(!Roots->empty())
	      {
		basic_task *t = Roots->pop();
		item *it = t->execute();
		delete t;
		
		if(it == 0) continue;

		nres++;
		fprintf(fstatus, "derivation[%d]: ", nres);
		it->print_derivation(fstatus, false);
		fprintf(fstatus, "\n");
#ifdef YY
		if(opt_k2y)
		  {
		    mflush(mstream);
		    int n = construct_k2y(nres, it, false, mstream);
		    if(n >= 0)
		      fprintf(fstatus, "\n%s\n\n", mstring(mstream));
		    else 
		      {
			fprintf(fstatus, 
				"\n K2Y error:  %s .\n\n", 
				mstring(mstream));
		      }
		  }
#endif
	      }
	    mclose(mstream);
	  }
        fflush(fstatus);
      } /* try */
      
      catch(error &e)
	{
	  e.print(ferr); fprintf(ferr, "\n");
	  stats.readings = -1;
	}

      if(Chart != 0) delete Chart;
      if(Roots != 0) delete Roots;

      id++;
    } /* while */

#ifdef DAG_FAILURES
  if(opt_compute_qc)
    {
      FILE *qc = fopen("/tmp/qc.tdl", "w");
      compute_qc_paths(qc, 10000);
      fclose(qc);
    }
#endif
}

void dump_glbs(FILE *f)
{
  int i, j;
  for(i = 0; i < ntypes; i++)
    {
      prune_glbcache();
      for(j = 0; j < i; j++)
	if(glb(i,j) != -1) fprintf(f, "%d %d %d\n", i, j, glb(i,j));
    }
}

void print_symbol_tables(FILE *f)
{
  fprintf(f, "type names (print names)\n");
  for(int i = 0; i < ntypes; i++)
    {
      fprintf(f, "%d\t%s (%s)\n", i, typenames[i], printnames[i]);
    }

  fprintf(f, "attribute names\n");
  for(int i = 0; i < nattrs; i++)
    {
      fprintf(f, "%d\t%s\n", i, attrname[i]);
    }
}

void print_grammar(FILE *f)
{
  if(verbosity > 10)
    dump_glbs(f);

  print_symbol_tables(f);
}


void process(char *s)
{
  cheap_settings = New settings(settings::basename(s), s, "reading");
  fprintf(fstatus, "\n");

  timer t_start;
  fprintf(fstatus, "loading `%s' ", s);

  try { Grammar = New grammar(s); }

  catch(error &e)
    {
      fprintf(fstatus, "- aborted\n");
      e.print(ferr);
      delete Grammar;
      delete cheap_settings;
      return;
    }

  fprintf(fstatus, "- %d types in %0.2g s\n",
	  ntypes, t_start.convert2ms(t_start.elapsed()) / 1000.);

  if(opt_pg)
    {
      print_grammar(stdout);
    }
  else
    {
      initialize_version();

#if defined(YY) && defined(SOCKET_INTERFACE)
      if(opt_server)
	cheap_server(opt_server);
      else 
#endif
#ifdef TSDBAPI
      if(opt_tsdb)
	tsdb_mode();
      else
#endif
	{
#ifdef ONLINEMORPH
	  if(opt_interactive_morph)
	    interactive_morph();
	  else
#endif
	    interactive();
	}
    }

  delete Grammar;
  delete cheap_settings;
}

#ifdef __BORLANDC__
int real_main(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  ferr = stderr;
  fstatus = stderr;
  flog = (FILE *)NULL;

  setlocale(LC_ALL, "" );

#ifndef __BORLANDC__
  if(!parse_options(argc, argv))
    {
      usage(ferr);
      exit(1);
    }
#else
  if(argc > 1)
    grammar_file_name = argv[1];
  else
    grammar_file_name = "english";
#endif

#if defined(YY) && defined(SOCKET_INTERFACE)
  if(opt_server)
    {
      if(cheap_server_initialize(opt_server))
	exit(1);
    }
#endif

  grammar_file_name = find_file(grammar_file_name, GRAMMAR_EXT);
  if(grammar_file_name == 0)
    {
      fprintf(ferr, "Grammar not found\n");
      exit(1);
    }

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

  if(flog != NULL) fclose(flog);
  return 0;
}

/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* main module (standalone parser) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cheap.h"
#include "parse.h"
#include "fs.h"
#include "tsdb++.h"

#ifdef DAG_FAILURES
#include "qc.h"
#endif

#ifdef YY
#include "yy.h"
#include "k2y.h"
#endif

char *get_input(FILE *);
void interactive();
void process(char *);

FILE *ferr, *fstatus, *flog;

const int ASBS = 4096; // arbitrary, small buffer size

char *get_input(FILE *f)
{
  char *buff;
  buff = new char[ASBS];

  if(fgets(buff, ASBS, f) == NULL)
    return 0;

  if(buff[0] == '\0' || buff[0] == '\n') return 0;

  buff[strlen(buff) - 1] = '\0';

  return buff;
}

// global variables for parsing

grammar *Grammar;
settings *cheap_settings;

void interactive()
{
  char *input = 0;
  int id = 1;

  while((input = get_input(stdin)) != 0)
    {
      try {
	tokenlist *Input;
#ifdef YY
	if(opt_yy)
	  Input = new yy_tokenlist(input);
	else
#endif
	  Input = new lingo_tokenlist(input);
	
        chart Chart(Input->length());
      
	parse(Chart, Input, id);
        
	if(verbosity == -1)
	  fprintf(stdout, "%d\t%d\t%d\n",
		  stats.id, stats.readings, stats.pedges);

        fprintf(fstatus, 
                "(%d) `%s' [%d] --- %d (%.1f|%.1fs) <%d:%d> (%.1fK) [%.1fs]\n",
                stats.id, input, pedgelimit, stats.readings, 
                stats.first/1000., stats.tcpu / 1000.,
                stats.words, stats.pedges, stats.dyn_bytes / 1024.0,
		TotalParseTime.convert2s(TotalParseTime.elapsed()));

        if(verbosity > 1) stats.print(fstatus);

#ifdef YY
        if(opt_k2y || verbosity > 1)
#else
        if(verbosity > 1)
#endif

	  {
	    int nres = 0;
	    struct MFILE *mstream = mopen();
	    for(chart_iter it(Chart); it.valid(); it++)
	      {
		if(it.current()->result_root())
		  {
		    nres++;
                    fprintf(fstatus, "derivation[%d]: ", nres);
                    it.current()->print_derivation(fstatus, false);
                    fprintf(fstatus, "\n");
#ifdef YY
		    if(opt_k2y)
		      {
                        mflush(mstream);
			int n = construct_k2y(nres, it.current(), 
                                              false, mstream);
			if(n >= 0)
                          fprintf(fstatus, "\n%s\n\n", mstring(mstream));
                        else {
                          fprintf(fstatus, 
                                  "\n K2Y error:  %s .\n\n", 
                                  mstring(mstream));
                        } /* else */
		      } /* if */
#endif
		  } /* if */
	      } /* for */
	    mclose(mstream);
	  } /* if */
        fflush(fstatus);
	delete Input;
      } /* catch */
      
      catch(error &e)
	{
	  e.print(ferr); fprintf(ferr, "\n");
	  stats.readings = -1;
	}
      
      delete[] input;
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
  fprintf(f, "type names\n");
  for(int i = 0; i < ntypes; i++)
    {
      fprintf(f, "%d\t%s\n", i, typenames[i]);
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
  cheap_settings = new settings("cheap", s, "reading");
  fprintf(fstatus, "\n");

  timer t_start;
  fprintf(fstatus, "loading `%s' ", s);
  try { Grammar = new grammar(s); }
  
  catch(error &e)
    {
      fprintf(fstatus, "- aborted\n");
      e.print(ferr);
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

#ifdef YY
      if(opt_server)
	cheap_server(opt_server);
      else 
#endif
#ifdef TSDBAPI
      if(opt_tsdb)
	tsdb_mode();
      else
#endif
        interactive();
    }

  delete Grammar;
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

  if(!parse_options(argc, argv))
    {
      usage(ferr);
      exit(1);
    }

#ifdef YY
  if(opt_server)
    {
      if(cheap_server_initialize(opt_server))
	exit(1);
    }
#endif

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


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

/* main module (standalone parser) */

#include "pet-system.h"
#include "cheap.h"
#include "parse.h"
#include "chart.h"
#include "fs.h"
#include "tsdb++.h"
#include "grammar-dump.h"
#include "lexparser.h"
#include "morph.h"
#include "yy-tokenizer.h"
#include "lingo-tokenizer.h"
#include "xml-tokenizer.h"
#include "item-printer.h"
#include "version.h"

#ifdef QC_PATH_COMP
#include "qc.h"
#endif

#ifdef YY
#include "yy.h"
#endif

#ifdef ECL
#include "petecl.h"
#include "cppbridge.h"
#endif

char * version_string = VERSION ;
char * version_change_string = VERSION_CHANGE " " VERSION_DATETIME ;

FILE *ferr, *fstatus, *flog;

// global variables for parsing

tGrammar *Grammar = 0;
settings *cheap_settings = 0;
bool XMLServices = false;

void interactive()
{
    string input;
    int id = 1;

    //tFegramedPrinter chp("/tmp/fed-", true);
    //chp.print(type_dag(lookup_type("quant-rel")));
    //exit(1);

    tTsdbDump tsdb_dump(opt_tsdb_file);
    if (tsdb_dump.active()) {
      opt_tsdb = 1;
    } else {
      if (! opt_tsdb_file.empty())
        fprintf(ferr, "Could not open TSDB dump files in directory %s\n"
                , opt_tsdb_file.c_str());
    }

    while(!(input = read_line(stdin)).empty())
    {
        chart *Chart = 0;

        tsdb_dump.start(input);

        try {
            fs_alloc_state FSAS;

            // input_chart i_chart(new end_proximity_position_map);

            list<tError> errors;
            analyze(input, Chart, FSAS, errors, id);
            if(!errors.empty())
                throw errors.front();
                
            if(verbosity == -1)
                fprintf(stdout, "%d\t%d\t%d\n",
                        stats.id, stats.readings, stats.pedges);

            fprintf(fstatus, 
                    "(%d) `%s' [%d] --- %d (%.2f|%.2fs) <%d:%d> (%.1fK) [%.1fs]\n",
                    stats.id, input.c_str(), pedgelimit, stats.readings, 
                    stats.first/1000., stats.tcpu / 1000.,
                    stats.words, stats.pedges, stats.dyn_bytes / 1024.0,
                    TotalParseTime.elapsed_ts() / 10.);

            if(verbosity > 0) stats.print(fstatus);

            tsdb_dump.finish(Chart);

            //tTclChartPrinter chp("/tmp/final-chart-bernie", 0);
            //tFegramedPrinter chp("/tmp/fed-", true);
            //Chart->print(&chp);
            
            if(verbosity > 1 || opt_mrs)
            {
                int nres = 0;
                
                list< tItem * > results(Chart->readings().begin()
                                        , Chart->readings().end());
                results.sort(item_better_than());
                for(list<tItem *>::iterator iter = results.begin()
                      ; (iter != results.end())
                         && ((opt_nresults == 0) || (opt_nresults > nres))
                      ; ++iter)
                {
                  //tFegramedPrinter fedprint("/tmp/fed-", true);
                  //tDelegateDerivationPrinter deriv(fstatus, fedprint);
                    tCompactDerivationPrinter deriv(fstatus);
                    tItem *it = *iter;
                    
                    nres++;
                    fprintf(fstatus, "derivation[%d]", nres);
                    fprintf(fstatus, " (%.4g)", it->score());
                    fprintf(fstatus, ":");
                    it->print_yield(fstatus);
                    fprintf(fstatus, "\n");
                    if(verbosity > 2)
                    {
                        deriv.print(it);
                        fprintf(fstatus, "\n");
                    }
#ifdef ECL
                    if(opt_mrs)
                    {
                        string mrs;
                        mrs = ecl_cpp_extract_mrs(it->get_fs().dag(), opt_mrs);
                        if (mrs.empty()) {
                          if (strcmp(opt_mrs, "xml") == 0)
                            fprintf(fstatus, "\n<rmrs cfrom='-2' cto='-2'>\n"
                                    "</rmrs>\n");
                          else
                            fprintf(fstatus, "\nNo MRS\n");
                        } else {
                          fprintf(fstatus, "%s\n", mrs.c_str());
                        }
                    }
#endif
                }

#ifdef ECL
                if(opt_partial && (Chart->readings().empty())) {
                  list< tItem * > partials;
                  Chart->shortest_path(partials);
                  bool rmrs_xml = (strcmp(opt_mrs, "xml") == 0);
                  if (rmrs_xml) fprintf(fstatus, "\n<rmrs-list>\n");
                  for(list<tItem *>::iterator it = partials.begin()
                        ; it != partials.end(); ++it) {
                    if(opt_mrs)
                      {
                        tPhrasalItem *item = dynamic_cast<tPhrasalItem *>(*it);
                        if (item != NULL) {
                          string mrs;
                          mrs = ecl_cpp_extract_mrs(item->get_fs().dag()
                                                    , opt_mrs);
                          if (! mrs.empty()) {
                            fprintf(fstatus, "%s\n", mrs.c_str());
                          }
                        }
                      }
                  }
                  if (rmrs_xml) fprintf(fstatus, "</rmrs-list>\n");
                }
#endif           
            }
            fflush(fstatus);
        } /* try */
        
        catch(tError &e)
        {
            fprintf(ferr, "%s\n", e.getMessage().c_str());
            if(verbosity > 0) stats.print(fstatus);
            stats.readings = -1;

            tsdb_dump.error(Chart, e);

        }

        if(Chart != 0) delete Chart;

        id++;
    } /* while */

#ifdef QC_PATH_COMP
    if(opt_compute_qc)
    {
        FILE *qc = fopen(opt_compute_qc, "w");
        compute_qc_paths(qc);
        fclose(qc);
    }
#endif
}

void nbest()
{
    string input;

    while(!feof(stdin))
    {
        int id = 0;
        int selected = -1;
        int time = 0;
        
        while(!(input = read_line(stdin)).empty())
        {
            if(selected != -1)
                continue;

            chart *Chart = 0;
            try
            {
                fs_alloc_state FSAS;
                
                // input_chart i_chart(new end_proximity_position_map);
                
                list<tError> errors;
                analyze(input, Chart, FSAS, errors, id);
                if(!errors.empty())
                    throw errors.front();
                
                if(stats.readings > 0)
                {
                    selected = id;
                    fprintf(stdout, "[%d] %s\n", selected, input.c_str());
                }
                
                stats.print(fstatus);
                fflush(fstatus);
            } /* try */
            
            catch(tError &e)
            {
                fprintf(ferr, "%s\n", e.getMessage().c_str());
                stats.print(fstatus);
                fflush(fstatus);
                stats.readings = -1;
            }
            
            if(Chart != 0) delete Chart;
            
            time += stats.tcpu;
            
            id++;
        }
        if(selected == -1)
            fprintf(stdout, "[]\n");

        fflush(stdout);

        fprintf(fstatus, "ttcpu: %d\n", time);
    }
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
      fprintf(f, "%d\t%s (%s)\n", i, type_name(i), print_name(i));
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
    timer t_start;
  
    try {
      cheap_settings = new settings(settings::basename(s), s, "reading");
      fprintf(fstatus, "\n");
      fprintf(fstatus, "loading `%s' ", s);
      Grammar = new tGrammar(s); 
    }
    catch(tError &e)
    {
      fprintf(fstatus, "\naborted\n%s\n", e.getMessage().c_str());
      delete cheap_settings;
      return;
    }
    
    try {
      init_characterization();
      Lexparser.init();

      dumper dmp(s);
      tFullformMorphology *ff = tFullformMorphology::create(dmp);
      if (ff != NULL) {
        Lexparser.register_morphology(ff);
        // ff->print(fstatus);
      }
      if(opt_online_morph) {
        tLKBMorphology *lkbm = tLKBMorphology::create(dmp);
        if (lkbm != NULL)
          Lexparser.register_morphology(lkbm);
        else
          opt_online_morph = false;
      }
      Lexparser.register_lexicon(new tInternalLexicon());

      tTokenizer *tok;
      switch (opt_tok) {
      case TOKENIZER_YY: 
      case TOKENIZER_YY_COUNTS: 
        {
          char *classchar = cheap_settings->value("class-name-char");
          if (classchar != NULL)
            tok = new tYYTokenizer(opt_tok == TOKENIZER_YY_COUNTS, classchar[0]);
          else
            tok = new tYYTokenizer(opt_tok == TOKENIZER_YY_COUNTS);
        }
        break;
      case TOKENIZER_STRING: 
      case TOKENIZER_INVALID: 
        tok = new tLingoTokenizer(); break;
      case TOKENIZER_XML:
      case TOKENIZER_XML_COUNTS: 
        xml_initialize();
        XMLServices = true;
        tok = new tXMLTokenizer(opt_tok == TOKENIZER_XML_COUNTS); break;
      default:
        tok = new tLingoTokenizer(); break;
      }
      Lexparser.register_tokenizer(tok);
      
    }
    
    catch(tError &e)
    {
      fprintf(fstatus, "\naborted\n%s\n", e.getMessage().c_str());
      delete Grammar;
      delete cheap_settings;
      return;
   }

    fprintf(fstatus, "\n%d types in %0.2g s\n",
            ntypes, t_start.convert2ms(t_start.elapsed()) / 1000.);
    
#ifdef ECL
    char *cl_argv[] = {"cheap", 0};
    ecl_initialize(1, cl_argv, s);
#endif

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
#if 0 // #ifdef ONLINEMORPH
                if(opt_interactive_morph)
                    interactive_morph();
                else
#endif
                {
                    if(opt_nbest)
                        nbest();
                    else
                        interactive();
                }
            }
    }
    
    if (XMLServices) xml_finalize();
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

  setlocale(LC_ALL, "C" );

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

  catch(tError &e)
    {
      fprintf(ferr, "%s\n", e.getMessage().c_str());
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

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
#include "agenda.h"
#include "chart.h"
#include "fs.h"
#include "tsdb++.h"
#include "mfile.h"
#include "grammar-dump.h"
#include "inputchart.h"

#ifdef QC_PATH_COMP
#include "qc.h"
#endif

#ifdef YY
#include "yy.h"
#endif

FILE *ferr, *fstatus, *flog;

// global variables for parsing

grammar *Grammar = 0;
settings *cheap_settings = 0;

#ifdef ONLINEMORPH
#include "morph.h"

void interactive_morph()
{
  morph_analyzer *m = Grammar->morph();

  string input;
  while(!(input = read_line(stdin)).empty())
    {
#if 1
      list<morph_analysis> res = m->analyze(input, false);
      for(list<morph_analysis>::iterator it = res.begin(); it != res.end(); ++it)
	{
	  fprintf(stdout, "%s\t", it->base().c_str());
	  it->print_lkb(stdout);
	  fprintf(stdout, "\n");
	}
#elif 0
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

map<string, set<string> > supertagMap;
map<string, int> overflowMap;

#define MAX_WORDS_PER_TAG 10

void
writeSuperTagged(FILE *fTagged, FILE *fData, chart *Chart, int nderivations)
{
    int j = 0;
    while(j < opt_supertag_norm)
    {
        for(vector<item *>::iterator iter = Chart->readings().begin();
            iter != Chart->readings().end() && j < opt_supertag_norm;
            ++iter, ++j)
        {
            item *it = *iter;
            list<string> tags;
            list<list<string> > words;
            
            it->getTagSequence(tags, words);
            assert(tags.size() == words.size());

            list<string>::iterator itTags;
            list<list<string> >::iterator itWords;
            
            for(itTags = tags.begin(), itWords = words.begin();
                itTags != tags.end(); ++itTags, ++itWords)
            {
                int i = 1, n = itWords->size();                for(list<string>::iterator itWord = itWords->begin();
                    itWord != itWords->end(); ++itWord)
                {
                    string thisTag = string("LinGO-") + *itTags;

                    if(n > 1)
                    {
                        // This is part of a multi word
                        ostringstream tmp;
                        tmp << "-" << i;
                        thisTag += tmp.str();
                    }
                    else
                    {
                        string wordPlusTag = *itWord + thisTag;
                        if(overflowMap[wordPlusTag] == 0)
                        {
                            // This is a new pair

                            int n;
                            
                            n = overflowMap[thisTag]++;
                            overflowMap[wordPlusTag] =
                                (n / MAX_WORDS_PER_TAG) + 1;
                        }

                        ostringstream tmp;
                        tmp << "-" << overflowMap[wordPlusTag];
                        thisTag += tmp.str();
                    }
                        
                    supertagMap[thisTag].insert(*itWord);

                    fprintf(fTagged, "%s ", thisTag.c_str());
                    if(fData)
                    {
                        fprintf(fData, "%s ", itWord->c_str());
                    }
                }
            }
            fprintf(fTagged, "\n");
            if(fData)
                fprintf(fData, "\n");
        }
    }
}

void
writeSuperTagVoc(FILE *f, const map<string, set<string> > &tagMap)
{
    fprintf(f, ";; LinGO vocabulary\n\n");

    for(map<string, set<string> >::const_iterator itMap = tagMap.begin();
        itMap != tagMap.end(); ++itMap)
    {
        fprintf(f, "%s [", itMap->first.c_str());
        for(set<string>::const_iterator itSet = itMap->second.begin();
            itSet != itMap->second.end(); ++itSet)
            fprintf(f, " %s", itSet->c_str());
        fprintf(f, " ]\n");
    }
}

void interactive()
{
    string input;
    int id = 1;

    FILE *fTags = 0;
    FILE *fData = 0;
    FILE *fVoc = 0;
    
    if(opt_supertag_file)
    {
        char *tmp = new char[strlen(opt_supertag_file) + 42];

        strcpy(tmp, opt_supertag_file);
        strcat(tmp, ".tagged");
        fTags = fopen(tmp, "w");

        strcpy(tmp, opt_supertag_file);
        strcat(tmp, ".data");
        fData = fopen(tmp, "w");

        strcpy(tmp, opt_supertag_file);
        strcat(tmp, ".voc");
        fVoc = fopen(tmp, "w");

        delete[] tmp;
    }

    while(!(input = read_line(stdin)).empty())
    {
        chart *Chart = 0;
        try {
            fs_alloc_state FSAS;

            input_chart i_chart(New end_proximity_position_map);

            list<error> errors;
            analyze(i_chart, input, Chart, FSAS, errors, id);
            if(!errors.empty())
                throw errors.front();
                
            if(verbosity == -1)
                fprintf(stdout, "%d\t%d\t%d\n",
                        stats.id, stats.readings, stats.pedges);

            fprintf(fstatus, 
                    "(%d) `%s' [%d] --- %d (%.1f|%.1fs) <%d:%d> (%.1fK) [%.1fs]\n",
                    stats.id, input.c_str(), pedgelimit, stats.readings, 
                    stats.first/1000., stats.tcpu / 1000.,
                    stats.words, stats.pedges, stats.dyn_bytes / 1024.0,
                    TotalParseTime.elapsed_ts() / 10.);

            if(verbosity > 0) stats.print(fstatus);

            if(fTags && stats.readings > 0)
                writeSuperTagged(fTags, fData, Chart,
                                   stats.readings);

            if(verbosity > 1)

            {
                int nres = 0;
                struct MFILE *mstream = mopen();
                
                for(vector<item *>::iterator iter = Chart->readings().begin();
                    iter != Chart->readings().end(); ++iter)
                {
                    item *it = *iter;
                    
                    nres++;
                    fprintf(fstatus, "derivation[%d]", nres);
                    if(Grammar->sm())
                    {
                        fprintf(fstatus, " (%.4g)", it->score(Grammar->sm()));
                    }
                    fprintf(fstatus, ":");
                    it->print_yield(fstatus);
                    fprintf(fstatus, "\n");
                    if(verbosity > 2)
                    {
                        it->print_derivation(fstatus, false);
                        fprintf(fstatus, "\n");
                    }
                }
                mclose(mstream);
            }
            fflush(fstatus);
        } /* try */
        
        catch(error &e)
        {
            e.print(ferr); fprintf(ferr, "\n");
            if(verbosity > 1) stats.print(fstatus);
            stats.readings = -1;
        }

        if(Chart != 0) delete Chart;

        id++;
    } /* while */

    if(opt_supertag_file && fVoc)
    {
        writeSuperTagVoc(fVoc, supertagMap);
    }

#ifdef QC_PATH_COMP
    if(opt_compute_qc)
    {
        FILE *qc = fopen(opt_compute_qc, "w");
        compute_qc_paths(qc, 10000);
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
                
                input_chart i_chart(New end_proximity_position_map);
                
                list<error> errors;
                analyze(i_chart, input, Chart, FSAS, errors, id);
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
            
            catch(error &e)
            {
                e.print(ferr); fprintf(ferr, "\n");
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
    timer t_start;
  

    try {
      cheap_settings = New settings(settings::basename(s), s, "reading");
      fprintf(fstatus, "\n");
      fprintf(fstatus, "loading `%s' ", s);
      Grammar = New grammar(s); 
    }
    
    catch(error &e)
    {
        fprintf(fstatus, "\naborted\n");
        e.print(ferr);
        delete Grammar;
        delete cheap_settings;
        return;
    }
    
    fprintf(fstatus, "\n%d types in %0.2g s\n",
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
                {
                    if(opt_nbest)
                        nbest();
                    else
                        interactive();
                }
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

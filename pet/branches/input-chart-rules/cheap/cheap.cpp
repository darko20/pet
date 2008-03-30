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

#include <iostream>

#include "pet-config.h"
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
#ifdef HAVE_XML
#include "pic-tokenizer.h"
#include "smaf-tokenizer.h"
#endif
#include "item-printer.h"
#include "version.h"
#include "mrs.h"
#include "vpm.h"
#include "qc.h"

#ifdef YY
#include "yy.h"
#endif

#ifdef HAVE_ECL
#include "petecl.h"
#endif
#ifdef HAVE_MRS
#include "petmrs.h"
#include "cppbridge.h"
#endif
#ifdef HAVE_PREPROC
#include "eclpreprocessor.h"
#endif

char * version_string = VERSION ;
char * version_change_string = VERSION_CHANGE " " VERSION_DATETIME ;

FILE *ferr, *fstatus, *flog;

// global variables for parsing

tGrammar *Grammar = 0;
settings *cheap_settings = 0;
bool XMLServices = false;
tVPM *vpm = 0;

struct passive_weights : public unary_function< tItem *, unsigned int > {
  unsigned int operator()(tItem * i) {
    // return a weight close to infinity if this is not a phrasal item
    // thus, we guarantee that phrasal items are drastically preferred
    if (dynamic_cast<tPhrasalItem *>(i) != NULL) return 1;
    else if (dynamic_cast<tLexItem *>(i) != NULL) return 1000;
    else return 1000000;
  }
};

struct input_only_weights : public unary_function< tItem *, unsigned int > {
  unsigned int operator()(tItem * i) {
    // return a weight close to infinity if this is not an input item
    // prefer shorter items over longer ones
    if (dynamic_cast<tInputItem *>(i) != NULL)
      return i->span();
    else
      return 1000000;
  }
};

string get_surface_string(chart *ch) {
  list< tItem * > inputs;
  input_only_weights io;

  ch->shortest_path<unsigned int>(inputs, io, false);
  string surface;
  for(list<tItem *>::iterator it = inputs.begin()
        ; it != inputs.end(); it++) {
    tInputItem *inp = dynamic_cast<tInputItem *>(*it);
    if (inp != NULL) {
      surface = surface + inp->orth() + " ";
    }
  }

  int len = surface.length();
  if (len > 0) {
    surface.erase(surface.length() - 1);
  }
  return surface;
}


void dump_jxchg(string surface, chart *current) {
  if (! opt_jxchg_dir.empty()) {
    string yieldname = surface;
    replace(yieldname.begin(), yieldname.end(), ' ', '_');
    yieldname = opt_jxchg_dir + yieldname;
    ofstream out(yieldname.c_str());
    if (! out) {
      fprintf(ferr, "Can not open file %s\n", yieldname.c_str());
    } else {
      out << "0 " << current->rightmost() << endl;
      tJxchgPrinter chp(out);
      current->print(&chp);
    }
  }
}


void interactive() {
  string input;
  int id = 1;

  //tFegramedPrinter chp("/tmp/fed-");
  //chp.print(type_dag(lookup_type("quant-rel")));
  //exit(1);

  tTsdbDump tsdb_dump(opt_tsdb_dir);
  if (tsdb_dump.active()) {
    opt_tsdb = 1;
  } else {
    if (! opt_tsdb_dir.empty())
      fprintf(ferr, "Could not open TSDB dump files in directory %s\n"
              , opt_tsdb_dir.c_str());
  }

  while(Lexparser.next_input(std::cin, input)) {
    chart *Chart = 0;

    tsdb_dump.start();

    try {
      fs_alloc_state FSAS;

      list<tError> errors;
      analyze(input, Chart, FSAS, errors, id);
      if(!errors.empty())
        throw errors.front();
                
      if(verbosity == -1)
        fprintf(stdout, "%d\t%d\t%d\n",
                stats.id, stats.readings, stats.pedges);

      string surface = get_surface_string(Chart);

      fprintf(fstatus, 
              "(%d) `%s' [%d] --- %d (%.2f|%.2fs) <%d:%d> (%.1fK) [%.1fs]\n",
              stats.id, surface.c_str(), pedgelimit, stats.readings, 
              stats.first/1000., stats.tcpu / 1000.,
              stats.words, stats.pedges, stats.dyn_bytes / 1024.0,
              TotalParseTime.elapsed_ts() / 10.);

      if(verbosity > 0) stats.print(fstatus);

      tsdb_dump.finish(Chart, surface);

      //tTclChartPrinter chp("/tmp/final-chart-bernie", 0);
      //tFegramedPrinter chp("/tmp/fed-");
      //Chart->print(&chp);

      //dump_jxchg(surface, Chart);

      if(verbosity > 1 || opt_mrs) {
        int nres = 0;
                
        list< tItem * > results(Chart->readings().begin()
                                , Chart->readings().end());
        // sorting was done already in parse_finish
        // results.sort(item_greater_than_score());
        for(list<tItem *>::iterator iter = results.begin()
              ; (iter != results.end())
              && ((opt_nresults == 0) || (opt_nresults > nres))
              ; ++iter) {
          //tFegramedPrinter fedprint("/tmp/fed-");
          //tDelegateDerivationPrinter deriv(fstatus, fedprint);
          tCompactDerivationPrinter deriv(fstatus);
          tItem *it = *iter;
                    
          nres++;
          fprintf(fstatus, "derivation[%d]", nres);
          fprintf(fstatus, " (%.4g)", it->score());
          fprintf(fstatus, ":");
          it->print_yield(fstatus);
          fprintf(fstatus, "\n");
          if(verbosity > 2) {
            deriv.print(it);
            fprintf(fstatus, "\n");
          }
#ifdef HAVE_MRS
          if (opt_mrs && (strcmp(opt_mrs, "new") != 0)) {
            string mrs;
            mrs = ecl_cpp_extract_mrs(it->get_fs().dag(), opt_mrs);
            if (mrs.empty()) {
              if (strcmp(opt_mrs, "rmrx") == 0)
                fprintf(fstatus, "\n<rmrs cfrom='-2' cto='-2'>\n"
                        "</rmrs>\n");
              else
                fprintf(fstatus, "\nNo MRS\n");
            } else {
              fprintf(fstatus, "%s\n", mrs.c_str());
            }
          }
#endif
          if (opt_mrs && (strcmp(opt_mrs, "new") == 0)) {
            mrs::tPSOA* mrs = new mrs::tPSOA(it->get_fs().dag());
            if (mrs->valid()) {
              mrs::tPSOA* mapped_mrs = vpm->map_mrs(mrs, true); 
              if (mapped_mrs->valid()) {
                fprintf(fstatus, "\n");
                mapped_mrs->print(fstatus);
                fprintf(fstatus, "\n");
              }
              delete mapped_mrs;
            }
            delete mrs;
          }
        }

#ifdef HAVE_MRS
        if(opt_partial && (Chart->readings().empty())) {
          list< tItem * > partials;
          passive_weights pass;
          Chart->shortest_path<unsigned int>(partials, pass, true);
          bool rmrs_xml = (strcmp(opt_mrs, "rmrx") == 0);
          if (rmrs_xml) fprintf(fstatus, "\n<rmrs-list>\n");
          for(list<tItem *>::iterator it = partials.begin()
                ; it != partials.end(); ++it) {
            if(opt_mrs) {
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
          else fprintf(fstatus, "EOM\n");
        }
#endif           
      }
      fflush(fstatus);
    } /* try */
        
    catch(tError e) {
      fprintf(ferr, "%s\n", e.getMessage().c_str());
      if(verbosity > 0) stats.print(fstatus);
      stats.readings = -1;

      string surface = get_surface_string(Chart);
      dump_jxchg(surface, Chart);
      tsdb_dump.error(Chart, surface, e);

    }

    if(Chart != 0) delete Chart;

    id++;
  } /* while */

  if(opt_compute_qc) {
    FILE *qc = fopen(opt_compute_qc, "w");
    compute_qc_paths(qc);
    fclose(qc);
  }
}

void interactive_morphology() {
  string input;

  while(Lexparser.next_input(std::cin, input)) {
    timer clock;
    list<tMorphAnalysis> res = Lexparser.morph_analyze(input);
    
    for(list<tMorphAnalysis>::iterator it = res.begin(); 
        it != res.end(); 
        ++it) {
      fprintf(stdout, "%s\t", it->base().c_str());
      it->print_lkb(stdout);
      fprintf(stdout, "\n");
    } // for
    fprintf(fstatus,
            "\n%d chains in %0.2g s\n",
            res.size(), clock.convert2ms(clock.elapsed()) / 1000.);
  } // while

} // interactive_morphology()


void dump_glbs(FILE *f) {
  fprintf(f, "i j GLB(i,j)\n");
  int i, j;
  for(i = 0; i < nstatictypes; i++) {
    prune_glbcache();
    for(j = 0; j < i; j++)
      if(glb(i,j) != -1) fprintf(f, "%d %d %d\n", i, j, glb(i,j));
  }
  fprintf(f, "\n");
}

void print_symbol_tables(FILE *f) {
  fprintf(f, "ID\tTYPE NAME (PRINT NAME)\n");
  for(int i = 0; i < nstatictypes; i++) {
    fprintf(f, "%d\t%s (%s)\n", i, type_name(i), print_name(i));
  }
  fprintf(f, "\n");

  fprintf(f, "ID\tATTRIBUTE NAME\n");
  for(int i = 0; i < nattrs; i++) {
    fprintf(f, "%d\t%s\n", i, attrname[i]);
  }
  fprintf(f, "\n");
}

void print_type_hierarchy(FILE* f)
{
  fprintf(f, "%-30s %-7s %s\n", "TYPE", "PROPER", "ALL SUPERTYPES (BUT *top*)");
  for (int t = 1; t < nstatictypes; t++) {
    char t_str[100] = "";
    snprintf(t_str, 100, "%s(%d)", type_name(t), t);
    char st_str[1000] = "";
    std::list<type_t> sts = all_supertypes(t);
    bool first = true;
    for (std::list<type_t>::iterator it  = sts.begin(); it != sts.end(); it++) {
      if ((*it) != t) {
        char str[30] = "";
        snprintf(str, 30, "%s%s(%d)", first ? "":",", type_name((*it)), (*it));
        strncat(st_str, str, 30);
        first = false;
      }
    }
    fprintf(f, "%-30s %-7d %s\n", t_str, is_proper_type(t), st_str);
  }
  fprintf(f, "\n");
}

void print_grammar(FILE *f) {
  if(verbosity > 10)
    dump_glbs(f);

  print_symbol_tables(f);
  print_type_hierarchy(f);
}

void cleanup() {
#ifdef HAVE_XML
  if (XMLServices) xml_finalize();
#endif
  delete Grammar;
  delete cheap_settings;
  delete vpm;
}

void process(const char *s) {
  timer t_start;
  
  try {
    string base = raw_name(s);
    cheap_settings = new settings(base.c_str(), s, "reading");
    fprintf(fstatus, "\n");
    fprintf(fstatus, "loading `%s' ", s);
    Grammar = new tGrammar(s); 

#ifdef HAVE_ECL
    char *cl_argv[] = {"cheap", 0};
    ecl_initialize(1, cl_argv);
    // make the redefinition warnings go away
    ecl_eval_sexpr("(setq cl-user::erroutsave cl-user::*error-output* "
                   "cl-user::*error-output* nil)");
#endif  // HAVE_ECL

#ifdef DYNAMIC_SYMBOLS
    init_characterization();
#endif
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
          tok = new tYYTokenizer((opt_tok == TOKENIZER_YY_COUNTS 
                                  ? STANDOFF_COUNTS : STANDOFF_POINTS),
                                 classchar[0]);
        else
          tok = new tYYTokenizer((opt_tok == TOKENIZER_YY_COUNTS
                                  ? STANDOFF_COUNTS : STANDOFF_POINTS));
      }
      break;
    case TOKENIZER_STRING: 
    case TOKENIZER_INVALID: 
      tok = new tLingoTokenizer(); break;

    case TOKENIZER_PIC:
    case TOKENIZER_PIC_COUNTS: 
#ifdef HAVE_XML
      xml_initialize();
      XMLServices = true;
      tok = new tPICTokenizer((opt_tok == TOKENIZER_PIC_COUNTS
                               ? STANDOFF_COUNTS : STANDOFF_POINTS)); break;
#else
      fprintf(ferr, "No XML input mode compiled into this cheap\n");
      exit(1);
#endif

    case TOKENIZER_FSR:
#ifdef HAVE_PREPROC
#ifdef HAVE_ICU
      tok = new tFSRTokenizer(s); break;
#endif
#else
      fprintf(ferr, "No ecl/Lisp based FSR preprocessor "
              "compiled into this cheap\n");
      exit(1);
#endif

    case TOKENIZER_SMAF:
#ifdef HAVE_XML
#ifdef HAVE_ICU
      xml_initialize();
      XMLServices = true;
      tok = new tSMAFTokenizer(); break;
#else
      fprintf(ferr, "\nERROR: No ICU (Unicode) support compiled into this cheap.\n");
      exit(1);
#endif
#else
      fprintf(ferr, "\nERROR: No XML support compiled into this cheap.\n");
      exit(1);
#endif

    default:
      tok = new tLingoTokenizer(); break;
    }
    tok->set_comment_passthrough(opt_comment_passthrough);
    Lexparser.register_tokenizer(tok);
  }
    
  catch(tError &e) {
    fprintf(fstatus, "\naborted\n%s\n", e.getMessage().c_str());
    cleanup();
    return;
  }

#ifdef HAVE_MRS
  //
  // when requested, initialize the MRS variable property mapping from a file
  // specified in the grammar configuration.                   (24-aug-06; oe)
  //
  char *name = cheap_settings->value("vpm");
  if(name) {
    string file = find_file(name, ".vpm", s);
    mrs_initialize(s, file.c_str());
  } else mrs_initialize(s, NULL);

#endif

  vpm = new tVPM();
  char *name2 = cheap_settings->value("vpm");
  if (name2) {
    string file = find_file(name2, ".vpm", s);
    vpm->read_vpm(file);
  }

#ifdef HAVE_ECL
  // reset the error stream so warnings show up again
  ecl_eval_sexpr("(setq cl-user::*error-output* cl-user::erroutsave)");
#endif // HAVE_ECL

  fprintf(fstatus, "\n%d types in %0.2g s\n",
          nstatictypes, t_start.convert2ms(t_start.elapsed()) / 1000.);
  fflush(fstatus);

  if(opt_pg) {
    print_grammar(stdout);
  }
  else {
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
          if(opt_interactive_morph)
            interactive_morphology();
          else
            interactive();
        }
  }
  cleanup();
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
  if(!parse_options(argc, argv)) {
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
  if(opt_server) {
    if(cheap_server_initialize(opt_server))
      exit(1);
  }
#endif

  string grammar_name = find_file(grammar_file_name, GRAMMAR_EXT);
  if(grammar_name.empty()) {
    fprintf(ferr, "Grammar not found\n");
    exit(1);
  }

  try { process(grammar_name.c_str()); }

  catch(tError &e) {
    fprintf(ferr, "%s\n", e.getMessage().c_str());
    exit(1);
  }

  catch(bad_alloc) {
    fprintf(ferr, "out of memory\n");
    exit(1);
  }

  if(flog != NULL) fclose(flog);
  return 0;
}

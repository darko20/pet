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

/* interface to the [incr tsdb()] system */

#include "pet-system.h"

#include "cheap.h"
#include "parse.h"
#include "agenda.h"
#include "chart.h"
#include "inputchart.h"
#include "tokenizer.h"
#include "tsdb++.h"
#include "mfile.h"
#include "qc.h"
#ifdef YY
# include "k2y.h"
# include "yy.h"
#endif

statistics stats;

void statistics::reset()
{
  id = 0;
  trees = 0;
  readings = 0;
  words = 0;
  words_pruned = 0;
  first = -1;
  tcpu = 0;
  ftasks_fi = 0;
  ftasks_qc = 0;
  etasks = 0;
  stasks = 0;
  aedges = 0;
  pedges = 0;
  raedges = 0;
  rpedges = 0;
  medges = 0;
  unifications_succ = 0;
  unifications_fail = 0;
  subsumptions_succ = 0;
  subsumptions_fail = 0;
  copies = 0;
  dyn_bytes = 0;
  stat_bytes = 0;
  cycles = 0;
  nmeanings = 0;
  unify_cost_succ = 0;
  unify_cost_fail = 0;

  p_trees = 0;
  p_equivalent = 0;
  p_proactive = 0;
  p_retroactive = 0;
  p_frozen = 0;
  p_utcpu = 0;

  // rule stuff
  for(rule_iter rule(Grammar); rule.valid(); rule++)
    {
      grammar_rule *R = rule.current();
      R->actives = R->passives = 0;
    }
}

void statistics::print(FILE *f)
{
  fprintf (f,
	   "id: %d\ntrees: %d\nreadings: %d\nwords: %d\nwords_pruned: %d\nfirst: %d\ntcpu: %d\n"
           "utcpu: %d\n"
	   "ftasks_fi: %d\nftasks_qc: %d\netasks: %d\nstasks: %d\n"
	   "aedges: %d\npedges: %d\nraedges: %d\nrpedges: %d\n"
	   "medges: %d\n"
	   "unifications_succ: %d\nunifications_fail: %d\n"
	   "subsumptions_succ: %d\nsubsumptions_fail: %d\ncopies: %d\n"
	   "dyn_bytes: %ld\n"
	   "stat_bytes: %ld\n"
	   "cycles: %d\nfssize: %d\n"
	   "unify_cost_succ: %d\nunify_cost_fail: %d\n"
           "trees: %d\nequivalent: %d\nproactive: %d\nretroactive: %d\n"
           "frozen: %d\n",
	   id, trees, readings, words, words_pruned, first, tcpu,
           p_utcpu,
	   ftasks_fi, ftasks_qc, etasks, stasks,
	   aedges, pedges, raedges, rpedges,
	   medges,
	   unifications_succ, unifications_fail,
	   subsumptions_succ, subsumptions_fail, copies,
	   dyn_bytes,
	   stat_bytes,
	   cycles, fssize,
	   unify_cost_succ, unify_cost_fail,
           p_trees, p_equivalent, p_proactive, p_retroactive,
           p_frozen
	   );
}

#define ABSBS 80 /* arbitrary small buffer size */
char CHEAP_VERSION [4096];
char CHEAP_PLATFORM [1024];

void initialize_version()
{
#if defined(DAG_TOMABECHI)
  char da[ABSBS]="tom";
#elif defined(DAG_FIXED)
  char da[ABSBS]="fix";
#elif defined(DAG_SYNTH1)
  char da[ABSBS]="syn";
#elif defined(DAG_SIMPLE)
#if defined(WROBLEWSKI1)
  char da[ABSBS]="sim[1]";
#elif defined (WROBLEWSKI2)
  char da[ABSBS]="sim[2]";
#elif defined (WROBLEWSKI3)
  char da[ABSBS]="sim[3]";
#else
  char da[ABSBS]="sim[?]";
#endif
#else
  char da[ABSBS]="unknown";
#endif

  char *qcs = cheap_settings->value("qc-structure");
  if(qcs == NULL) qcs = "";

  string sts("");
  struct setting *set;

  if((set = cheap_settings->lookup("start-symbols")) != 0)
    {
      for(int i = 0; i < set->n; i++)
        {
          if(i!=0) sts += string(", ");
          sts += string(set->values[i]);
        }                 
    }

  if((set = cheap_settings->lookup("weighted-start-symbols")) != 0)
    {
      for(int i = 0; i < set->n; i += 2)
        {
          if(i!=0) sts += string(", ");
          sts += string(set->values[i]);
          sts += string(" [") + string(set->values[i + 1]) + string("]");
        }                 
    }

  sprintf(CHEAP_VERSION,
          "PET(%s cheap) [%d] %sPA(%d) RI[%s] %s(%d) %s %s[%d(%s)] %s[%d] "
          "%s %s {ns %d} (%s/%s) <%s>",
          da,
          pedgelimit,
          opt_packing ? "+" : "-",
          opt_packing,
          opt_key == 0 ? "key" : (opt_key == 1 ? "l-r" : (opt_key == 2 ? "r-l" : (opt_key == 3 ? "head" : "unknown"))),
          opt_hyper ? "+HA" : "-HA",
          Grammar->nhyperrules(),
          opt_filter ? "+FI" : "-FI",
          opt_nqc != 0 ? "+QC" : "-QC", opt_nqc, qcs,
          ((opt_nsolutions != 0) ? "+OS" : "-OS"), opt_nsolutions, 
          opt_shrink_mem ? "+SM" : "-SM", 
          opt_shaping ? "+SH" : "-SH",
	  sizeof(dag_node),
          __DATE__, __TIME__,
          sts.c_str());

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
  sprintf(CHEAP_PLATFORM, "gcc %d.%d", __GNUC__, __GNUC_MINOR__);
#if defined(__OPTIMIZED__)
  sprintf(CHEAP_PLATFORM, " (optim)");
#endif
#else
  sprintf(CHEAP_PLATFORM, "unknown");
#endif
}

#ifdef TSDBAPI

int cheap_create_test_run(char *data, int run_id, char *comment,
                         int interactive, char *custom)
{
  cheap_tsdb_summarize_run();
  return 0;
}


void cheap_tsdb_summarize_run(void)
{
  capi_printf("(:application . \"%s\") ", CHEAP_VERSION);
  capi_printf("(:platform . \"%s\") ", CHEAP_PLATFORM);
  capi_printf("(:grammar . %s) ", Grammar->info().version);
  capi_printf("(:avms . %d) ", ntypes);
  capi_printf("(:leafs . %d) ", ntypes - first_leaftype);
  capi_printf("(:lexicon . %d) ", Grammar->nstems());
  capi_printf("(:rules . %d) ", Grammar->nrules());
#if 0
  capi_printf("(:templates . %s) ", Grammar->info().ntemplates);
#else
  capi_printf("(:templates . -1) ");
#endif
} /* cheap_tsdb_summarize_run() */

int nprocessed = 0;

int cheap_process_item(int i_id, char *i_input, int parse_id, 
                       int edges, int exhaustive, 
                       int nderivations, int interactive)
{
  struct timeval tA, tB; int treal = 0;
  chart *Chart = 0;

  try {
    fs_alloc_state FSAS;

    input_chart i_chart(New end_proximity_position_map);

    pedgelimit = edges;
    if(exhaustive)
        opt_nsolutions = 0;
    else
        opt_nsolutions = 1;
    
    gettimeofday(&tA, NULL);

    TotalParseTime.save();

    analyze(i_chart, i_input, Chart, FSAS, i_id);
 
    nprocessed++;

    gettimeofday(&tB, NULL);

    treal = (tB.tv_sec - tA.tv_sec ) * 1000 +
      (tB.tv_usec - tA.tv_usec) / (MICROSECS_PER_SEC / 1000);

    tsdb_parse T;
    cheap_tsdb_summarize_item(*Chart, i_chart.max_position(), treal,
			      nderivations, 0, T);
    T.capi_print();

    delete Chart;

    return 0;
  } /* try */

  catch(error &e) {
    gettimeofday(&tB, NULL);

    treal = (tB.tv_sec - tA.tv_sec ) * 1000 +
      (tB.tv_usec - tA.tv_usec) / (MICROSECS_PER_SEC / 1000);

    TotalParseTime.restore();

    tsdb_parse T;
    cheap_tsdb_summarize_error(e, treal, T);
    T.capi_print();
    
  } /* catch */

  delete Chart;

  return 0;
}


int cheap_complete_test_run(int run_id, char *custom)
{
  fprintf(ferr, "total elapsed parse time %.3fs; %d items; avg time per item %.4fs\n",
	  TotalParseTime.elapsed_ts() / 10.,
	  nprocessed,
	  (TotalParseTime.elapsed_ts() / double(nprocessed)) / 10.);

#ifdef QC_PATH_COMP
    if(opt_compute_qc)
    {
        fprintf(ferr, "computing quick check paths\n");
        FILE *qc = fopen("/tmp/qc.tdl", "w");
        compute_qc_paths(qc, 10000);
        fclose(qc);
    }
#endif

  return 0;
}

int cheap_reconstruct_item(char *derivation)
{
  fprintf(ferr, "cheap_reconstruct_item(%s)\n", derivation);
  return 0;
}

void tsdb_mode()
{
  if(!capi_register(cheap_create_test_run, cheap_process_item, 
                    cheap_reconstruct_item, cheap_complete_test_run))
  {
    slave();
  }
}

void tsdb_result::capi_print()
{
  capi_printf(" (");
  capi_printf("(:result-id . %d) ", result_id);
  capi_printf("(:derivation . \"%s\") ",  escape_string(derivation).c_str());
  if(!mrs.empty())
    capi_printf("(:mrs . \"%s\") ", escape_string(mrs).c_str());
  if(!tree.empty())
    capi_printf("(:tree . \"%s\") ", escape_string(tree).c_str());
  capi_printf(")\n");
}

void tsdb_rule_stat::capi_print()
{
  capi_printf(" ((");
  capi_printf("(:rule . \"%s\") ", escape_string(rule).c_str());
  capi_printf("(:actives . %d) ", actives);
  capi_printf("(:passives . %d) ", passives);
  capi_printf("))\n");
}

void tsdb_parse::capi_print()
{
  if(!results.empty())
  {
    capi_printf("(:results\n");
    for(list<tsdb_result>::iterator it = results.begin();
        it != results.end(); ++it)
      it->capi_print();
    capi_printf(")\n");
  }

  if(!rule_stats.empty())
  {
    capi_printf("(:statistics\n");
    for(list<tsdb_rule_stat>::iterator it = rule_stats.begin();
        it != rule_stats.end(); ++it)
      it->capi_print();
    capi_printf(")\n");
  }
  
  if(!err.empty())
    capi_printf("(:error . \"%s\")", escape_string(err).c_str());
  
  if(readings != -1)
    capi_printf("(:readings . %d) ", readings);
  
  if(words != -1)
    capi_printf("(:words . %d) ", words);
  
  if(first != -1)
    capi_printf("(:first . %d) ", first);

  if(total != -1)
    capi_printf("(:total . %d) ", total);

  if(tcpu != -1)
    capi_printf("(:tcpu . %d) ", tcpu); 

  if(tgc != -1)
    capi_printf("(:tgc . %d) ", tgc); 

  if(treal != -1)
    capi_printf("(:treal . %d) ", treal); 

  if(others != -1)
    capi_printf("(:others . %ld) ", others);

  if(p_ftasks != -1)
    capi_printf("(:p-ftasks . %d) ", p_ftasks);

  if(p_etasks != -1)
    capi_printf("(:p-etasks . %d) ", p_etasks);

  if(p_stasks != -1)
    capi_printf("(:p-stasks . %d) ", p_stasks);

  if(aedges != -1)
    capi_printf("(:aedges . %d) ", aedges);
  
  if(pedges != -1)
    capi_printf("(:pedges . %d) ", pedges);

  if(raedges != -1)
    capi_printf("(:raedges . %d) ", raedges);

  if(rpedges != -1)
    capi_printf("(:rpedges . %d) ", rpedges);

  if(unifications != -1)
    capi_printf("(:unifications . %d) ", unifications);

  if(copies != -1)
    capi_printf("(:copies . %d) ", copies);
  
  capi_printf("(:comment . \""
              "(:nmeanings . %d) "
              "(:failures . %d) "
              "(:pruned . %d) "
              "(:subsumption . %d) "
              "(:trees . %d) "
              "(:frozen . %d) "
              "(:equivalence . %d) "
              "(:proactice . %d) "
              "(:retroactive . %d) "
              "(:utcpu . %d) " 
              "\")",
              nmeanings, failures, pruned,
              subsumptions,
              p_trees,
              p_equivalent,
              p_proactive, 
              p_retroactive, 
              p_frozen,
              p_utcpu);
}

#endif

static int tsdb_unique_id = 1;

void cheap_tsdb_summarize_item(chart &Chart, int length,
                               int treal, int nderivations, 
                               const char *meaning,
                               tsdb_parse &T)
{
  if(opt_derivation)
  {
    int nres = 1;
    stats.nmeanings = 0;
    struct MFILE *mstream = mopen();
    if(nderivations >= 0) // default case, report results
    {
      if(!nderivations) nderivations = Chart.readings().size();
      for(vector<item *>::iterator iter = Chart.readings().begin();
          nderivations && iter != Chart.readings().end(); ++iter, --nderivations)
      {
        tsdb_result R;

        R.parse_id = tsdb_unique_id;
        R.result_id = nres;
        R.derivation = (*iter)->tsdb_derivation();

#ifdef YY
        if(opt_k2y)
        {
          mflush(mstream);
          int nrelations
            = construct_k2y(nres, *iter, false, mstream);
          if(nrelations >= 0)
          {
            stats.nmeanings++;
            R.mrs = mstring(mstream);
          }
          else
          {
            T.err +=
              string(T.err.empty() ? "" : " ") +
              string(mstring(mstream));
          }
          //
          // _hack_
          // use :tree field to record YY role table, when available; from
          // the control strategy, we know that it must correspond to the
          // last reading that was computed.           (6-feb-01  -  oe@yy)
          //
          if(nres == stats.readings && meaning != NULL)
          {
            R.tree = string(meaning);
          }
        } // if
#endif
        T.push_result(R);
        nres++;
      } // for
    } // if
    else // report all passive edges
    {
      for(chart_iter it(Chart); it.valid(); it++)
      {
        if(it.current()->passive())
        {
          tsdb_result R;
          
          R.result_id = nres;
          R.derivation = it.current()->tsdb_derivation();
          
          T.push_result(R);
          nres++;
        }
      }
    }
    mclose(mstream);
  }

  if(opt_rulestatistics)
  {  // slow in tsdb++, thus disabled by default
    for(rule_iter rule(Grammar); rule.valid(); rule++)
    {
      tsdb_rule_stat S;
      grammar_rule *R = rule.current();

      S.rule = R->printname();
      S.actives = R->actives;
      S.passives = R->passives;

      T.push_rule_stat(S);
    }
  }

  T.run_id = 1;
  T.parse_id = tsdb_unique_id;
  T.i_id = tsdb_unique_id;
  T.date = string(current_time());
  tsdb_unique_id++;

  T.readings = stats.readings;
  T.words = stats.words;
  T.first = stats.first;
  T.total = stats.tcpu;
  T.tcpu = stats.tcpu;
  T.tgc = 0;
  T.treal = treal;
  
  T.others = stats.stat_bytes;
  
  T.p_ftasks = stats.ftasks_fi + stats.ftasks_qc;
  T.p_etasks = stats.etasks;
  T.p_stasks = stats.stasks;
  T.aedges = stats.aedges;
  T.pedges = stats.pedges;
  T.raedges = stats.raedges;
  T.rpedges = stats.rpedges;
  
  T.unifications = stats.unifications_succ + stats.unifications_fail;
  T.copies = stats.copies;
  
  T.nmeanings = (meaning != NULL && *meaning ? 1 : 0);
  T.failures = stats.unifications_fail;
  T.pruned = stats.words_pruned;

  T.subsumptions = stats.subsumptions_succ + stats.subsumptions_fail;
  T.p_trees = stats.p_trees;
  T.p_equivalent = stats.p_equivalent;
  T.p_proactive = stats.p_proactive; 
  T.p_retroactive = stats.p_retroactive;           
  T.p_frozen = stats.p_frozen;               
  T.p_utcpu = stats.p_utcpu;                    
}

void cheap_tsdb_summarize_error(error &condition, int treal, tsdb_parse &T)
{
  T.run_id = 1;
  T.parse_id = tsdb_unique_id;
  T.i_id = tsdb_unique_id;
  T.date = string(current_time());
  tsdb_unique_id++;

  T.readings = -1;
  T.pedges = stats.pedges;
  T.words = stats.words;

  T.first = stats.first;
  T.total = stats.tcpu;
  T.tcpu = stats.tcpu;
  T.tgc = 0;
  T.treal = treal;

  T.others = stats.stat_bytes;
  
  T.p_ftasks = stats.ftasks_fi + stats.ftasks_qc;
  T.p_etasks = stats.etasks;
  T.p_stasks = stats.stasks;
  T.aedges = stats.aedges;
  T.pedges = stats.pedges;
  T.raedges = stats.raedges;
  T.rpedges = stats.rpedges;
  
  T.unifications = stats.unifications_succ + stats.unifications_fail;
  T.copies = stats.copies;
  
  T.err = condition.msg();
}

void tsdb_parse::set_rt(const string &rt)
{
  if(results.empty())
    return;
  
  tsdb_result r = results.front();
  results.pop_front();
  
  r.tree = rt;
  
  results.push_front(r);
}

string tsdb_escape_string(const string &s)
{
  string res;

  bool lastblank = false;
  for(string::const_iterator it = s.begin(); it != s.end(); ++it)
  {
    if(isspace(*it))
    {
      if(!lastblank)
        res += " ";
      lastblank = true;
    }
    else if(*it == '@')
    {
      lastblank = false;
      res += "\\s";
    }
    else if(*it == '\\')
    {
      lastblank = false;
      res += string("\\\\");
    }
    else
    {
      lastblank = false;
      res += string(1, *it);
    }
  }
  
  return res;
}

void tsdb_result::file_print(FILE *f)
{
  fprintf(f, "%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@",
          parse_id, result_id, time,
          r_ctasks, r_ftasks, r_etasks, r_stasks, 
          size, r_aedges, r_pedges);
  fprintf(f, "%s@%s@%s\n",
          tsdb_escape_string(derivation).c_str(),
          tsdb_escape_string(tree).c_str(),
          tsdb_escape_string(mrs).c_str());
}

void tsdb_parse::file_print(FILE *f_parse, FILE *f_result, FILE *f_item)
{
  if(!results.empty())
  {
    for(list<tsdb_result>::iterator it = results.begin();
        it != results.end(); ++it)
      it->file_print(f_result);
  }

  fprintf(f_parse,
          "%d@%d@%d@"
          "%d@%d@%d@%d@%d@%d@"
          "%d@%d@%d@%d@%d@%d@"
          "%d@%d@%d@%d@"
          "%d@%d@%d@%d@"
          "%ld@%d@%d@%d@",
          parse_id, run_id, i_id,
          readings, first, total, tcpu, tgc, treal,
          words, l_stasks, p_ctasks, p_ftasks, p_etasks, p_stasks,
          aedges, pedges, raedges, rpedges,
          unifications, copies, conses, symbols, 
          others, gcs, i_load, a_load);

  fprintf(f_parse, "%s@%s@(:nmeanings . %d) "
          "(:failures . %d) (:pruned . %d)\n",
          tsdb_escape_string(date).c_str(),
          tsdb_escape_string(err).c_str(),
          nmeanings, failures, pruned);

  fprintf(f_item, "%d@unknown@unknown@unknown@1@unknown@%s@1@%d@@yy@%s\n",
          parse_id, tsdb_escape_string(i_input).c_str(), i_length, current_time());
}

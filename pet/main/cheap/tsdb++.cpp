/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
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
#ifdef YY
# include "k2y.h"
# include "yy.h"
#endif

statistics stats;

void statistics::reset()
{
  id = 0;
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
  copies = 0;
  dyn_bytes = 0;
  stat_bytes = 0;
  cycles = 0;
  fssize = 0;
  nmeanings = 0;
  unify_cost_succ = 0;
  unify_cost_fail = 0;

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
	   "id: %d\nreadings: %d\nwords: %d\nwords_pruned: %d\nfirst: %d\ntcpu: %d\n"
	   "ftasks_fi: %d\nftasks_qc: %d\netasks: %d\nstasks: %d\n"
	   "aedges: %d\npedges: %d\nraedges: %d\nrpedges: %d\n"
	   "medges: %d\n"
	   "unifications_succ: %d\nunifications_fail: %d\ncopies: %d\n"
	   "dyn_bytes: %ld\n"
	   "stat_bytes: %ld\n"
	   "cycles: %d\nfssize: %d\n"
	   "unify_cost_succ: %d\nunify_cost_fail: %d\n",
	   id, readings, words, words_pruned, first, tcpu,
	   ftasks_fi, ftasks_qc, etasks, stasks,
	   aedges, pedges, raedges, rpedges,
	   medges,
	   unifications_succ, unifications_fail, copies,
	   dyn_bytes,
	   stat_bytes,
	   cycles, fssize,
	   unify_cost_succ, unify_cost_fail
	   );
}

#define ABSBS 80 /* arbitrary small buffer size */
char CHEAP_VERSION [256];
char CHEAP_PLATFORM [256];

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
          "PET(%s cheap) [%d] {RI[%s] %s(%d) %s %s[%d(%s)] %s "
#ifdef YY
          "%s K2Y(%d) "
#endif
          "%s %s} {ns %d} (%s/%s) <%s>",
          da,
          pedgelimit,
          opt_key == 0 ? "key" : (opt_key == 1 ? "l-r" : (opt_key == 2 ? "r-l" : (opt_key == 3 ? "head" : "unknown"))),
          opt_hyper ? "+HA" : "-HA",
          Grammar->nhyperrules(),
          opt_filter ? "+FI" : "-FI",
          opt_nqc != 0 ? "+QC" : "-QC", opt_nqc, qcs,
          opt_one_solution ? "+OS" : "-OS", 
#ifdef YY
          (opt_one_meaning ? "+OM" : "-OM"), opt_k2y,
#endif
          opt_shrink_mem ? "+SM" : "-SM", 
          opt_shaping ? "+SH" : "-SH",
	  sizeof(dag_node),
          __DATE__, __TIME__,
          sts.c_str());

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
  sprintf(CHEAP_PLATFORM, "gcc %d.%d", __GNUC__, __GNUC_MINOR__);
#else
  sprintf(CHEAP_PLATFORM, "unknown");
#endif
}

#ifdef TSDBAPI

//
// _hack_
// really, capi_printf() should do appropriate escaping, depending on the
// context; but we would need to elaborate it significantly, i guess, so 
// hack around it for the time being                 (1-feb-01  -  oe@yy)
//

void capi_putstr(const char *s, bool strctx)
{
  if(strctx) capi_putc('\\');
  capi_putc('"');

  for(; s && *s; s++)
    {
      if(*s == '"' || *s == '\\')
	{
	  if(strctx)
	    {
	      capi_putc('\\');
	      capi_putc('\\');
	    }

	  capi_putc('\\');
	}
      capi_putc(*s);
    }

  if(strctx) capi_putc('\\');
  capi_putc('"');
}

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
  capi_printf("(:templates . %s) ", Grammar->info().ntemplates);

} /* cheap_tsdb_summarize_run() */

int nprocessed = 0;

int cheap_process_item(int i_id, char *i_input, int parse_id, 
                       int edges, int exhaustive, 
                       int derivationp, int interactive)
{
  struct timeval tA, tB; int treal = 0;
  chart *Chart = 0;
  agenda *Roots = 0;

  try {
    fs_alloc_state FSAS;

    input_chart i_chart(New end_proximity_position_map);

    pedgelimit = edges;
    opt_one_solution = !exhaustive;

    gettimeofday(&tA, NULL);

    TotalParseTime.save();

    analyze(i_chart, i_input, Chart, Roots, FSAS, i_id);
 
    nprocessed++;

    gettimeofday(&tB, NULL);

    treal = (tB.tv_sec - tA.tv_sec ) * 1000 +
      (tB.tv_usec - tA.tv_usec) / (MICROSECS_PER_SEC / 1000);

    cheap_tsdb_summarize_item(*Chart, Roots, i_chart.max_position(), treal,
			      derivationp);

    delete Chart;
    delete Roots;

    return 0;
  } /* try */

  catch(error &e) {
    gettimeofday(&tB, NULL);

    treal = (tB.tv_sec - tA.tv_sec ) * 1000 +
      (tB.tv_usec - tA.tv_usec) / (MICROSECS_PER_SEC / 1000);

    TotalParseTime.restore();

    cheap_tsdb_summarize_error(e, treal);
  } /* catch */

  delete Chart;
  delete Roots;

  return 0;
}


void cheap_tsdb_summarize_item(chart &Chart, agenda *Roots, int length,
                               int treal, int derivationp, char *meaning)
{
#ifdef YY
  int k2y_status = 0;
  string error = "";
#endif

  if(opt_derivation)
    {
      capi_printf("(:results\n");
      int nres = 1;
      stats.nmeanings = 0;
      struct MFILE *mstream = mopen();
      if(!derivationp) // default case, report results
	{
	  while(!Roots->empty())
	    {
	      basic_task *t = Roots->pop();
	      item *it = t->execute();
	      delete t;
		
	      if(it != 0) continue;
	
	      capi_printf("((:result-id . %d) ", nres);
	      capi_printf("(:derivation . \"");
	      it->tsdb_print_derivation();
	      capi_printf("\")");
        
#ifdef YY
	      if(opt_k2y)
		{
		  mflush(mstream);
		  int nrelations 
		    = construct_k2y(nres, it, false, mstream);
		  if(nrelations >= 0)
		    {
		      stats.nmeanings++;
		      capi_printf("(:mrs . ");
		      capi_putstr(mstring(mstream));
		      capi_printf(")");
		    }
		  else
		    {
		      error += 
			string(error.empty() ? "" : " ") +
			string(mstring(mstream));
		      k2y_status = nrelations;
		    }
	  //
          // _hack_
          // use :tree field to record YY role table, when available; from
          // the control strategy, we know that it must correspond to the
          // last reading that was computed.           (6-feb-01  -  oe@yy)
          //
		  if(nres == stats.readings && meaning != NULL)
		    {
		      capi_printf("(:tree . ");
		      capi_putstr(meaning);
		      capi_printf(")");
		    }
		}
#endif
	      capi_printf(")");
	      nres++;
	    } // while
	}
      else // report all passive edges
	{
	  for(chart_iter it(Chart); it.valid(); it++)
	    {
	      if(it.current()->passive())
		{
		  capi_printf("((:result-id . %d) ", nres);
		  capi_printf("(:derivation . \"");
		  it.current()->tsdb_print_derivation();
		  capi_printf("\"))");
		  nres++;
		}
	    }
	}
      capi_printf(")");
      mclose(mstream);
    }

#ifdef YY
  if(!error.empty())
    {
      capi_printf("(:error . ");
      capi_putstr(error.c_str());
      capi_printf(")");
    }
#endif

  if(opt_rulestatistics)
    {  // slow in tsdb++, thus disabled by default
      capi_printf("(:statistics . (");
      for(rule_iter rule(Grammar); rule.valid(); rule++)
	{
	  grammar_rule *R = rule.current();
	  capi_printf("((:rule . \"%s\") (:actives . %d) (:passives . %d))",
		      R->printname(), R->actives, R->passives);
	}
      capi_printf("))\n");
    }
  
  capi_printf("(:readings . %d) ", stats.readings);
  capi_printf("(:words . %d) ", stats.words);
  
  capi_printf("(:first . %d) ", stats.first);
  capi_printf("(:total . %d) ", stats.tcpu); /* total + lexicon = tcpu */
  capi_printf("(:tcpu . %d) ", stats.tcpu); 
  capi_printf("(:tgc . %d) ", 0); 
  capi_printf("(:treal . %d) ", treal); 

  capi_printf("(:others . %ld) ", stats.stat_bytes);

  capi_printf("(:p-ftasks . %d) ", stats.ftasks_fi + stats.ftasks_qc);
  capi_printf("(:p-etasks . %d) ", stats.etasks);
  capi_printf("(:p-stasks . %d) ", stats.stasks);
  capi_printf("(:aedges . %d) ", stats.aedges);
  capi_printf("(:pedges . %d) ", stats.pedges);
  capi_printf("(:raedges . %d) ", stats.raedges);
  capi_printf("(:rpedges . %d) ", stats.rpedges);

  capi_printf("(:unifications . %d) ", 
              stats.unifications_succ + stats.unifications_fail);
  capi_printf("(:copies . %d) ", stats.copies);
  
  capi_printf("(:comment . \"(:cycles . %d) (:fssize . %d) "
              "(:dspace . %ld) "
              "(:nk2ys . %d) (:nmeanings . %d) "
#ifdef YY
              "(:k2ystatus . %d)"
#endif
              "(:total . %d) (:scost . %d) (:fcost . %d) "
              "(:failures . %d) (:pruned . %d)\")",
              stats.cycles, stats.fssize, 
              stats.dyn_bytes, 
              stats.nmeanings, (meaning != NULL && *meaning ? 1 : 0),
#ifdef YY
              k2y_status,
#endif
              TotalParseTime.convert2ms(TotalParseTime.elapsed()),
              stats.unify_cost_succ, stats.unify_cost_fail, 
              stats.unifications_fail, stats.words_pruned);

} /* cheap_tsdb_summarize_item() */

void cheap_tsdb_summarize_error(error &condition, int treal) {

  capi_printf("(:readings . -1) ");
  capi_printf("(:pedges . %d) ", stats.pedges);
  capi_printf("(:words . %d) ", stats.words);
  
  capi_printf("(:first . %d) ", stats.first);
  capi_printf("(:total . %d) ", stats.tcpu); /* total + lexicon = tcpu */
  capi_printf("(:tcpu . %d) ", stats.tcpu); 
  capi_printf("(:tgc . %d) ", 0); 
  capi_printf("(:treal . %d) ", treal); 

  capi_printf("(:others . %ld) ", stats.stat_bytes);

  capi_printf("(:p-ftasks . %d) ", stats.ftasks_fi + stats.ftasks_qc);
  capi_printf("(:p-etasks . %d) ", stats.etasks);
  capi_printf("(:p-stasks . %d) ", stats.stasks);
  capi_printf("(:aedges . %d) ", stats.aedges);
  capi_printf("(:pedges . %d) ", stats.pedges);
  capi_printf("(:raedges . %d) ", stats.raedges);
  capi_printf("(:rpedges . %d) ", stats.rpedges);

  capi_printf("(:unifications . %d) ", 
              stats.unifications_succ + stats.unifications_fail);
  capi_printf("(:copies . %d) ", stats.copies);

  capi_printf("(:error .\"");
  condition.tsdb_print();
  capi_printf("\")");

} /* cheap_tsdb_summarize_error() */

int cheap_complete_test_run(int run_id, char *custom)
{
  fprintf(ferr, "total elapsed parse time %.3fs; %d items; avg time per item %.4fs\n",
	  TotalParseTime.elapsed_ts() / 10.,
	  nprocessed,
	  (TotalParseTime.elapsed_ts() / double(nprocessed)) / 10.);
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

#endif

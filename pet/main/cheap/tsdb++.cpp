/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* interface to the [incr tsdb()] system */

#include <time.h>

#ifndef __BORLANDC__
#include <sys/time.h>
#include <unistd.h>
#endif

#include "cheap.h"
#include "parse.h"
#include "tsdb++.h"
#pragma hdrstop
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
	   "unifications_succ: %d\nunifications_fail: %d\ncopies: %d\n"
	   "dyn_bytes: %ld\n"
	   "stat_bytes: %ld\n"
	   "cycles: %d\nfssize: %d\n"
	   "unify_cost_succ: %d\nunify_cost_fail: %d\n",
	   id, readings, words, words_pruned, first, tcpu,
	   ftasks_fi, ftasks_qc, etasks, stasks,
	   aedges, pedges, raedges, rpedges,
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

int cheap_create_test_run(char *data, int run_id, char *comment,
                         int interactive, char *custom)
{
  cheap_tsdb_summarize_run();
  return 0;
}


void cheap_tsdb_summarize_run(void) {

  capi_printf("(:application . \"%s\") ", CHEAP_VERSION);
  capi_printf("(:platform . \"%s\") ", CHEAP_PLATFORM);
  capi_printf("(:grammar . %s) ", Grammar->info().version);
  capi_printf("(:vocabulary . %s) ", Grammar->info().vocabulary);
  capi_printf("(:avms . %d) ", ntypes);
  capi_printf("(:leafs . %d) ", ntypes - first_leaftype);
  capi_printf("(:lexicon . %d) ", Grammar->nlexentries());
  capi_printf("(:rules . %d) ", Grammar->nrules());
  capi_printf("(:templates . %s) ", Grammar->info().ntemplates);

} /* cheap_tsdb_summarize_run() */

int nprocessed = 0;

int cheap_process_item(int i_id, char *i_input, int parse_id, 
                       int edges, int exhaustive, 
                       int derivationp, int interactive)
{
  tokenlist *Input = (tokenlist *)NULL;
  struct timeval tA, tB; int treal = 0;
  
  try {

#ifdef YY
    if(opt_yy)
      Input = new yy_tokenlist(i_input);
    else
#endif
      Input = new lingo_tokenlist(i_input);

    chart Chart(Input->length());

    pedgelimit = edges;
    opt_one_solution = !exhaustive;

    gettimeofday(&tA, NULL);

    TotalParseTime.save();

    parse(Chart, Input, i_id);

    nprocessed++;

    gettimeofday(&tB, NULL);

    treal = (tB.tv_sec - tA.tv_sec ) * 1000 +
      (tB.tv_usec - tA.tv_usec) / (MICROSECS_PER_SEC / 1000);

    cheap_tsdb_summarize_item(Chart, Input->length(), treal, derivationp);

    delete Input;
    return 0;
  } /* try */

  catch(error &e) {
    TotalParseTime.restore();
    cheap_tsdb_summarize_error(e);
    if(Input != NULL) delete Input;
    return 0;
  } /* catch */
}


void cheap_tsdb_summarize_item(chart &Chart, int length,
                               int treal, int derivationp,
                               char *meaning) {

#ifdef YY
  int k2y_status = 0;
  string error = "";
#endif

  if(opt_derivation) {
    capi_printf("(:results\n");
    int nres = 1;
    stats.nmeanings = 0;
    struct MFILE *mstream = mopen();
    for(chart_iter it(Chart); it.valid(); it++) {
      if((derivationp && it.current()->passive()) 
         || (!derivationp && it.current()->result_root())) {
        capi_printf("((:result-id . %d) ", nres);
        capi_printf("(:derivation . \"");
        it.current()->tsdb_print_derivation();
        capi_printf("\")");
        
#ifdef YY
        if(!derivationp && opt_k2y) {
          mflush(mstream);
          int nrelations 
            = construct_k2y(nres, it.current(), false, mstream);
          if(nrelations >= 0) {
            stats.nmeanings++;
            capi_printf("(:mrs . \"");
            for(char *foo = mstring(mstream); *foo; foo++) {
              if(*foo == '"' || *foo == '\\') capi_putc('\\');
              capi_putc(*foo);
            } /* for */
            capi_printf("\")");
          } /* if */
          else {
            error += 
              string(error.empty() ? "" : " ") + string(mstring(mstream));
            k2y_status = nrelations;
          } /* else */
          //
          // _hack_
          // use :tree field to record YY role table, when available; from
          // the control strategy, we know that it must correspond to the
          // last reading that was computed.           (6-feb-01  -  oe@yy)
          //
          if(nres == stats.readings && meaning != NULL) {
            capi_printf("(:tree . \"");
            for(char *foo = meaning; *foo; foo++) {
              if(*foo == '"' || *foo == '\\') capi_putc('\\');
              capi_putc(*foo);
            } /* for */
            capi_printf("\")");
          } /* if */
        } /* if */
#endif
        capi_printf(")");
        nres++;
      } /* if */
    } /* for */
    capi_printf(")");
    mclose(mstream);
  } /* if */

#ifdef YY
  if(!error.empty()) {
    capi_printf("(:error . \"");
    for(const char *foo = error.c_str(); *foo; foo++) {
      if(*foo == '"' || *foo == '\\') capi_putc('\\');
      capi_putc(*foo);
    } /* for */
    capi_printf("\")");
  } /* if */
#endif

  if(opt_rulestatistics) {  // slooooow in tsdb++, thus disabled by default
    capi_printf("(:statistics . (");
    for(rule_iter rule(Grammar); rule.valid(); rule++) {
      grammar_rule *R = rule.current();
      capi_printf("((:rule . \"%s\") (:actives . %d) (:passives . %d))",
                  R->name(), R->actives, R->passives);
    } /* for */
    capi_printf("))\n");
  } /* if */
  
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


void cheap_tsdb_summarize_error(error &condition) {

  capi_printf("(:readings . -1) ");
  capi_printf("(:pedges . %d) ", stats.pedges);
  capi_printf("(:error .\"");
  condition.tsdb_print();
  capi_printf("\")");

} /* cheap_tsdb_summarize_error() */


int cheap_complete_test_run(int run_id, char *custom)
{
  fprintf(ferr, "total elapsed parse time %.3fs; %d items; avg time per item %.4fs\n",
	  TotalParseTime.convert2s(TotalParseTime.elapsed()),
	  nprocessed,
	  (TotalParseTime.convert2ms(TotalParseTime.elapsed()) / double(nprocessed)) / 1000.);
  return 0;
}

int cheap_reconstruct_item(char *derivation)
{
  fprintf(ferr, "cheap_reconstruct_item(%s)\n", derivation);
  return 0;
}

void tsdb_mode()
{
  // int foo = 1;
  // while(foo);
  if(!capi_register(cheap_create_test_run, cheap_process_item, 
                    cheap_reconstruct_item, cheap_complete_test_run))
     {
       slave();
     }
}

#endif

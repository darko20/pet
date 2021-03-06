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

#include "tsdb++.h"
#include "cheap.h"
#include "parse.h"
#include "chart.h"
#include "qc.h"
#include "cppbridge.h"
#include "version.h"
#include "item-printer.h"
#include "sm.h"
#include "mrs.h"
#include "vpm.h"
#include "mrs-printer.h"
#include "settings.h"
#include "configs.h"
#include "logging.h"

#ifdef YY
# include "yy.h"
#endif

#include<sys/time.h>
#include<sstream>
#include<fstream>

using namespace std;

extern tVPM* vpm;

static bool init();
static bool tsdb_init = init();
static bool init(){
  managed_opt("opt_derivation",
    "Store derivations in tsdb profile", true);

  managed_opt("opt_rulestatistics",
    "dump the per-rule statistics to the tsdb database", false);
  return true;
}


statistics stats;

void
statistics::reset()
{
  id = 0;
  p_input.clear();
  p_tokens.clear();
  trees = 0;
  rtrees = 0;
  readings = 0;
  rreadings = 0;
  words = 0;
  words_pruned = 0;
  mtcpu = 0;
  first = -1;
  tcpu = 0;
  ftasks_fi = 0;
  ftasks_qc = 0;
  fsubs_fi = 0;
  fsubs_qc = 0;
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

  p_equivalent = 0;
  p_proactive = 0;
  p_retroactive = 0;
  p_frozen = 0;
  p_utcpu = 0;
  p_upedges = 0;
  p_failures = 0;
  p_hypotheses = 0;
  p_dyn_bytes = 0;
  p_stat_bytes = 0;

  // rule stuff
  for(ruleiter rule = Grammar->rules().begin(); rule != Grammar->rules().end();
      ++rule) {
    grammar_rule *R = *rule;
    R->actives = R->passives = 0;
  }
}

void
statistics::print(FILE *f)
{
  fprintf (f,
           "id: %d\ntrees: %d\nrtrees: %d\nreadings: %d\nrreadings: %d\n"
           "words: %d\nwords_pruned: %d\n"
           "mtcpu: %d\nfirst: %d\ntcpu: %d\nutcpu: %d\n"
           "ftasks_fi: %d\nftasks_qc: %d\n"
           "fsubs_fi: %d\nfsubs_qc: %d\n"
           "etasks: %d\nstasks: %d\n"
           "aedges: %d\npedges: %d\nupedges: %d\n"
           "raedges: %d\nrpedges: %d\n"
           "medges: %d\n"
           "unifications_succ: %d\nunifications_fail: %d\n"
           "subsumptions_succ: %d\nsubsumptions_fail: %d\ncopies: %d\n"
           "dyn_bytes: %ld\nstat_bytes: %ld\n"
           "p_dyn_bytes: %ld\np_nstat_bytes: %ld\n"
           "cycles: %d\nfssize: %d\n"
           "unify_cost_succ: %d\nunify_cost_fail: %d\n"
           "equivalent: %d\nproactive: %d\nretroactive: %d\n"
           "frozen: %d\nfailures: %d\nhypotheses: %d\n",
           id, trees, rtrees, readings, rreadings,
           words, words_pruned,
           mtcpu, first, tcpu, p_utcpu,
           ftasks_fi, ftasks_qc,
           fsubs_fi, fsubs_qc,
           etasks, stasks,
           aedges, pedges, p_upedges,
           raedges, rpedges,
           medges,
           unifications_succ, unifications_fail,
           subsumptions_succ, subsumptions_fail, copies,
           dyn_bytes, stat_bytes,
           p_dyn_bytes, p_stat_bytes,
           cycles, fssize,
           unify_cost_succ, unify_cost_fail,
           p_equivalent, p_proactive, p_retroactive,
           p_frozen, p_failures, p_hypotheses
           );
}

#define ABSBS 80 /* arbitrary small buffer size */
char CHEAP_VERSION [4096];
char CHEAP_PLATFORM [1024];

// bool to char
inline char btc(bool b) { return b ? '+' : '-'; }
// bool opt to char
inline char botc(const char *key) { return btc(get_opt_bool(key)); }
// int opt to char
inline char iotc(const char *key) { return btc(0 != get_opt_int(key)); }

void
initialize_version()
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

    const char *qcsu = cheap_settings->value("qc-structure-unif");
    if(qcsu == NULL) qcsu = "";
    const char *qcss = cheap_settings->value("qc-structure-subs");
    if(qcss == NULL) qcss = "";

    string sts("");
    setting *set = cheap_settings->lookup("start-symbols");

    if(set != 0)
    {
        for(int i = 0; i < set->n; ++i)
        {
            if(i!=0) sts += string(", ");
            sts += string(set->values[i]);
        }
    }

    const char *keypos[] = { "key", "l-r", "r-l", "head", "unknown" };
    int packing;
    get_opt("opt_packing", packing);
    sprintf(CHEAP_VERSION,
            "PET(%s cheap v%s) [%d] %cPA(%d) %cSM(%s) RI[%s] %cHA(%d) %cFI "
            "%cQCU[%d(%s)] %cQCS[%d(%s)]"
            " %cOS[%d] %cSM %cSH {ns %d} (%s/%s) <%s>",
            da,
            version_string,
            get_opt_int("opt_pedgelimit"),
            btc(0 != packing),
            packing,
            btc(NULL != Grammar->sm()),
            (NULL != Grammar->sm()) ? Grammar->sm()->description().c_str() : "",
            keypos[max(get_opt_int("opt_key"), 4)],
            botc("opt_hyper"), Grammar->nhyperrules(),
            botc("opt_filter"),
            iotc("opt_nqc_unif"), get_opt_int("opt_nqc_unif"), qcsu,
            iotc("opt_nqc_subs"), get_opt_int("opt_nqc_subs"), qcss,
            iotc("opt_nsolutions"), get_opt_int("opt_nsolutions"),
            botc("opt_shrink_mem"),
            botc("opt_shaping"),
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

static tGrammarUpdate *run_grammar_update = NULL;

int
cheap_create_test_run(const char *data, int run_id, const char *comment,
                      int interactive, int protocol_version,
                      char const *custom)
{
  int foo = protocol_version & 31;
  if(foo >= 1 && foo <= 2) set_opt("opt_tsdb", protocol_version);

  if(custom != NULL) {
    std::string foo(custom);
    run_grammar_update = new tGrammarUpdate(Grammar, foo);
  } // if

  cheap_tsdb_summarize_run();
  return 0;
}

void
cheap_tsdb_summarize_run(void)
{
    capi_printf("(:application . \"%s\") ", CHEAP_VERSION);
    capi_printf("(:platform . \"%s\") ", CHEAP_PLATFORM);
    capi_printf("(:grammar . \"%s\") ", Grammar->property("version").c_str());
    capi_printf("(:avms . %d) ", nstatictypes);
    capi_printf("(:leafs . %d) ", nstatictypes - first_leaftype);
    capi_printf("(:lexicon . %d) ", Grammar->nstems());
    capi_printf("(:rules . %d) ", Grammar->rules().size());
    if(!Grammar->property("ntemplates").empty())
      capi_printf("(:templates . %s) ",
                  Grammar->property("ntemplates").c_str());
    capi_printf("(:environment . \"");
    map<string, string> properties = Grammar->properties();
    for(map<string, string>::iterator it = properties.begin();
        it != properties.end(); ++it)
    {
        capi_printf("(:%s . \\\"%s\\\") ",
                    it->first.c_str(), it->second.c_str());
    }
    capi_printf("\") ");

}

static int nprocessed = 0;

int
cheap_process_item(int i_id, const char *i_input, int parse_id,
                   int edges, int nanalyses,
                   int nderivations, int interactive, const char *custom)
{
    struct timeval tA, tB; int treal = 0;
    chart *Chart = 0;

    try
    {
        fs_alloc_state FSAS;
        std::string foo(custom);
        tGrammarUpdate update(Grammar, foo);

        set_opt("opt_pedgelimit", edges);
        set_opt("opt_nsolutions", nanalyses);

        gettimeofday(&tA, NULL);

        TotalParseTime.save();

        list<tError> errors;
        analyze(i_input, Chart, FSAS, errors, i_id);

        ++nprocessed;

        gettimeofday(&tB, NULL);

        treal = (tB.tv_sec - tA.tv_sec ) * 1000 +
            (tB.tv_usec - tA.tv_usec) / (MICROSECS_PER_SEC / 1000);

        tsdb_parse T;
        if(!errors.empty())
          cheap_tsdb_summarize_error(errors, treal, T);

        cheap_tsdb_summarize_item(*Chart
                                  , Chart->rightmost()
                                  , treal, nderivations, T);
        T.capi_print();

        delete Chart;

        return 0;
    }

    catch(tError &e)
    {
        gettimeofday(&tB, NULL);

        treal = (tB.tv_sec - tA.tv_sec ) * 1000 
          + (tB.tv_usec - tA.tv_usec) / (MICROSECS_PER_SEC / 1000);

        TotalParseTime.restore();

        tsdb_parse T;
        list<tError> errors;
        errors.push_back(e);
        cheap_tsdb_summarize_error(errors, treal, T);
        T.capi_print();

    }

    delete Chart;

    return 0;
}

int
cheap_complete_test_run(int run_id, const char *custom)
{
    LOG(logAppl, INFO,
        "total elapsed parse time " << std::setprecision(3)
        << TotalParseTime.elapsed_ts() / 10.<< "s; "
        << nprocessed << " items; avg time per item " << std::setprecision(4)
        << (TotalParseTime.elapsed_ts() / double(nprocessed)) / 10. << "s");

    if(get_opt_charp("opt_compute_qc") != NULL)
    {
        LOG(logAppl, INFO, "computing quick check paths");
        ofstream qc(get_opt_charp("opt_compute_qc"));
        compute_qc_paths(qc, get_opt_int("opt_packing"));
    }

    if(run_grammar_update != NULL) {
      delete run_grammar_update;
      run_grammar_update = NULL;
    } // if

    return 0;
}

int
cheap_reconstruct_item(const char *derivation)
{
    LOG(logTsdb, INFO, "cheap_reconstruct_item(" << derivation << ")");
    return 0;
}

void
tsdb_mode()
{
    if(!capi_register(cheap_create_test_run, cheap_process_item,
                      cheap_reconstruct_item, cheap_complete_test_run))
    {
        slave();
    }
}

void
tsdb_result::capi_print()
{
    capi_printf(" (");

    capi_printf("(:result-id . %d) ", result_id);

    if(scored)
        capi_printf("(:score . %.4g) ", score);

    if((get_opt_int("opt_tsdb") & 31) == 1)
    {
        capi_printf("(:derivation . \"%s\") ",
                    escape_string(derivation).c_str());
    }
    else
    {
        capi_printf("(:edge . %d) ", edge_id);
    }

    if(!mrs.empty())
        capi_printf("(:mrs . \"%s\") ", escape_string(mrs).c_str());

    if(!tree.empty())
        capi_printf("(:tree . \"%s\") ", escape_string(tree).c_str());

    capi_printf(")\n");
}

void
tsdb_edge::capi_print()
{
    capi_printf("(");
    capi_printf("(:id . %d) ", id);
    capi_printf("(:label . \"%s\") ", escape_string(label).c_str());
    capi_printf("(:score . \"%g\") ", score);
    capi_printf("(:start . %d) ", start);
    capi_printf("(:end . %d) ", end);
    capi_printf("(:daughters . \"%s\") ", daughters.c_str());
    capi_printf("(:status . %d) ", status);
    capi_printf(")\n");
}

void
tsdb_rule_stat::capi_print()
{
    capi_printf("(");
    capi_printf("(:rule . \"%s\") ", escape_string(rule).c_str());
    capi_printf("(:actives . %d) ", actives);
    capi_printf("(:passives . %d) ", passives);
    capi_printf(")\n");
}

void
tsdb_parse::capi_print()
{
    if(!p_input.empty())
      capi_printf("(:p-input . \"%s\")", escape_string(p_input).c_str());

    if(!p_tokens.empty())
      capi_printf("(:p-tokens . \"%s\")", escape_string(p_tokens).c_str());

    if(!results.empty())
    {
        capi_printf("(:results . (\n");
        for(list<tsdb_result>::iterator it = results.begin();
            it != results.end(); ++it)
            it->capi_print();
        capi_printf("))\n");
    }

    if(!edges.empty())
    {
        capi_printf("(:chart . (\n");
        for(list<tsdb_edge>::iterator it = edges.begin();
            it != edges.end(); ++it)
            it->capi_print();
        capi_printf("))\n");
    }

    if(!rule_stats.empty())
    {
        capi_printf("(:statistics . (\n");
        for(list<tsdb_rule_stat>::iterator it = rule_stats.begin();
            it != rule_stats.end(); ++it)
            it->capi_print();
        capi_printf("))\n");
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
                "(:mtcpu . %d) "
                "(:clashes . %d) "
                "(:pruned . %d) "
                "(:subsumptions . %d) "
                "(:trees . %d) "
                "(:frozen . %d) "
                "(:equivalence . %d) "
                "(:proactive . %d) "
                "(:retroactive . %d) "
                "(:utcpu . %d) "
                "(:upedges . %d) "
                "(:failures . %d) "
                "(:hypotheses . %d) "
                "(:rtrees . %d) "
                "(:rreadings . %d) "
                "\")",
                nmeanings,
                mtcpu,
                clashes,
                pruned,
                subsumptions,
                trees,
                p_frozen,
                p_equivalent,
                p_proactive,
                p_retroactive,
                p_utcpu,
                p_upedges,
                p_failures,
                p_hypotheses,
                rtrees, rreadings);
}

#endif

static int tsdb_unique_id = 1;

void
tsdb_parse_collect_edges(tsdb_parse &T, tItem *root)
{
    list<tItem *> edges;
    root->collect_children(edges);

    for(list<tItem *>::iterator it = edges.begin(); it != edges.end(); ++it)
    {
        tsdb_edge e;
        e.id = (*it)->id();
        e.status = 0; // (passive edge)
        e.label = string((*it)->printname());
        e.start = (*it)->start();
        e.end = (*it)->end();
        e.score = 0.0;
        list<int> dtrs;
        ostringstream tmp;
        tmp << "(";
        (*it)->daughter_ids(dtrs);
        for(list<int>::iterator it_dtr = dtrs.begin();
            it_dtr != dtrs.end(); ++it_dtr)
            tmp << (it_dtr == dtrs.begin() ? "" : " ") << *it_dtr;
        tmp << ")";
        e.daughters = tmp.str();
        T.push_edge(e);
    }
}

string tsdb_derivation(tItem *it, int protocolversion) {
  ostringstream out;
  tTSDBDerivationPrinter tdp(out, protocolversion);
  tdp.print(it);
  return out.str();
}

void
cheap_tsdb_summarize_item(chart &Chart, int length,
                          int treal, int nderivations,
                          tsdb_parse &T)
{
    int tsdb_mode;
    get_opt("opt_tsdb", tsdb_mode);
    tsdb_mode &= 31;
    if(get_opt_bool("opt_derivation"))
    {
        int nres = 0;
        if(nderivations >= 0) // default case, report results
        {
            if(!nderivations) nderivations = Chart.readings().size();
            for(vector<tItem *>::iterator iter = Chart.readings().begin();
                nderivations && iter != Chart.readings().end();
                ++iter, --nderivations)
            {
                tsdb_result R;

                R.parse_id = tsdb_unique_id;
                R.result_id = nres;
                if(Grammar->sm())
                {
                    R.scored = true;
                    R.score = (*iter)->score();
                }
                if(tsdb_mode == 1)
                    R.derivation = tsdb_derivation(*iter, tsdb_mode);
                else
                {
                    R.edge_id = (*iter)->id();
                    tsdb_parse_collect_edges(T, *iter);
                }

                if(! get_opt_string("opt_mrs").empty()) {
                  if ((strcmp(get_opt_string("opt_mrs").c_str(), "new") == 0) ||
                      (strcmp(get_opt_string("opt_mrs").c_str(), "simple") == 0)) {
                    fs f = (*iter)->get_fs();
                    mrs::tPSOA* mrs = new mrs::tPSOA(f.dag());
                    if (mrs->valid()) {
                      mrs::tPSOA* mapped_mrs = vpm->map_mrs(mrs, true);
                      ostringstream out;
                      if (mapped_mrs->valid()) {
                        if (strcmp(get_opt_string("opt_mrs").c_str(), "new") == 0) {
                          MrxMRSPrinter ptr(out);
                          ptr.print(mapped_mrs);
                        } else if (strcmp(get_opt_string("opt_mrs").c_str(), "simple") == 0) {
                          SimpleMRSPrinter ptr(out);
                          ptr.print(mapped_mrs);
                        }
                      }
                      R.mrs = out.str();
                      delete mapped_mrs;
                    }
                    delete mrs;
                  }
#ifdef HAVE_MRS
                  else {
                    R.mrs = ecl_cpp_extract_mrs((*iter)->get_fs().dag(),
                                                get_opt_string("opt_mrs").c_str());
                  }
#endif
                }
                T.push_result(R);
                ++nres;
            }
        }
        else // report all passive edges
        {
            for(chart_iter it(Chart); it.valid(); ++it)
            {
                if(it.current()->passive())
                {
                    tsdb_result R;

                    R.result_id = nres;
                    R.derivation = tsdb_derivation(it.current(), tsdb_mode);

                    T.push_result(R);
                    ++nres;
                }
            }
        }
    }

    if(get_opt_bool("opt_rulestatistics"))
    {
      for(ruleiter rule = Grammar->rules().begin();
          rule != Grammar->rules().end(); ++rule)
        {
            tsdb_rule_stat S;
            grammar_rule *R = *rule;


            S.rule = R->printname();
            S.actives = R->actives;
            S.passives = R->passives;

            T.push_rule_stat(S);
        }
    }

    T.run_id = 1;
    T.parse_id = tsdb_unique_id;
    T.i_id = tsdb_unique_id;
    ++tsdb_unique_id;
    T.p_input = stats.p_input;
    T.p_tokens = stats.p_tokens;
    T.date = current_time();

    T.readings = stats.readings;
    T.words = stats.words;
    T.mtcpu = stats.mtcpu;
    T.first = stats.first;
    // This is the right way to do it, even if berthold complains.
    T.tcpu = stats.tcpu;
    T.total = stats.tcpu + stats.p_utcpu;
    T.tgc = 0;
    T.treal = treal;

    T.others = stats.stat_bytes + stats.p_stat_bytes;

    T.p_ftasks = stats.ftasks_fi + stats.ftasks_qc;
    T.p_etasks = stats.etasks;
    T.p_stasks = stats.stasks;
    T.aedges = stats.aedges;
    T.pedges = stats.pedges + stats.p_upedges;
    T.raedges = stats.raedges;
    T.rpedges = stats.rpedges;

    T.unifications = stats.unifications_succ + stats.unifications_fail;
    T.copies = stats.copies;

    T.nmeanings = 0;
    T.clashes = stats.unifications_fail;
    T.pruned = stats.words_pruned;

    T.subsumptions = stats.subsumptions_succ + stats.subsumptions_fail;
    T.trees = stats.trees;
    T.p_equivalent = stats.p_equivalent;
    T.p_proactive = stats.p_proactive;
    T.p_retroactive = stats.p_retroactive;
    T.p_frozen = stats.p_frozen;
    T.p_utcpu = stats.p_utcpu;
    T.p_upedges = stats.p_upedges;
    T.p_failures = stats.p_failures;
    T.p_hypotheses = stats.p_hypotheses;

    T.rtrees = stats.rtrees;
    T.rreadings = stats.rreadings;
}

void
cheap_tsdb_summarize_error(list<tError> &conditions, int treal, tsdb_parse &T)
{
    T.run_id = 1;
    T.parse_id = tsdb_unique_id;
    T.i_id = tsdb_unique_id;
    T.date = current_time();
    ++tsdb_unique_id;

    T.readings = -1;
    T.pedges = stats.pedges;
    T.words = stats.words;

    T.mtcpu = stats.mtcpu;
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

    T.subsumptions = stats.subsumptions_succ + stats.subsumptions_fail;
    T.trees = stats.trees;
    T.p_equivalent = stats.p_equivalent;
    T.p_proactive = stats.p_proactive;
    T.p_retroactive = stats.p_retroactive;
    T.p_frozen = stats.p_frozen;
    T.p_utcpu = stats.p_utcpu;
    T.p_upedges = stats.p_upedges;
    T.p_failures = stats.p_failures;
    T.p_hypotheses = stats.p_hypotheses;

    for(list<tError>::iterator it = conditions.begin(); it != conditions.end();
        ++it)
        T.err += string((it == conditions.begin() ? "" : " ")) + it->getMessage();
}

string
tsdb_escape_string(const string &s)
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

void
tsdb_result::file_print(FILE *f)
{
    fprintf(f, "%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@",
            parse_id, result_id, time,
            r_ctasks, r_ftasks, r_etasks, r_stasks,
            size, r_aedges, r_pedges);
    fprintf(f, "%s@%s@%s@%s@%s\n",
            tsdb_escape_string(derivation).c_str(),
            tsdb_escape_string(surface).c_str(),
            tsdb_escape_string(tree).c_str(),
            tsdb_escape_string(mrs).c_str(),
            tsdb_escape_string(flags).c_str()
            );
}

void
tsdb_parse::file_print(FILE *f_parse, FILE *f_result, FILE *f_item)
{
    if(!results.empty())
    {
        for(list<tsdb_result>::iterator it = results.begin();
            it != results.end(); ++it)
            it->file_print(f_result);
    }

    fprintf(f_parse,
            "%d@%d@%d@%s@%s@"
            "%d@%d@%d@%d@%d@%d@"
            "%d@%d@%d@%d@%d@%d@"
            "%d@%d@%d@%d@"
            "%d@%d@%d@%d@"
            "%ld@%d@%d@%d@",
            parse_id, run_id, i_id,
            p_input.c_str(), p_tokens.c_str(),
            readings, first, total, tcpu, tgc, treal,
            words, l_stasks, p_ctasks, p_ftasks, p_etasks, p_stasks,
            aedges, pedges, raedges, rpedges,
            unifications, copies, conses, symbols,
            others, gcs, i_load, a_load);

    fprintf(f_parse, "%s@%s@(:nmeanings . %d) "
            "(:clashes . %d) (:pruned . %d)\n",
            tsdb_escape_string(date).c_str(),
            tsdb_escape_string(err).c_str(),
            nmeanings, clashes, pruned);

    fprintf(f_item, "%d@unknown@unknown@unknown@1@unknown@%s@@@@1@%d@@yy@%s\n",
            parse_id, tsdb_escape_string(i_input).c_str(), i_length, current_time().c_str());
}


tTsdbDump::tTsdbDump(string directory)
  : _parse_file(NULL), _result_file(NULL), _item_file(NULL), _current(NULL) {
  if (directory.size() > 0) {

    if(directory[directory.size() - 1] != '/')
      directory += "/";

    static const string files_to_touch[] = {
      "analysis", "phenomenon", "parameter", "set", "item-phenomenon",
      "item-set", "rule", "output", "edge", "tree", "decision",
      "preference", "update", "fold", "score"
    };

    // returns true on error
    int len = sizeof(files_to_touch) / sizeof(string);
    if (print_relations(directory) || print_run(directory)) return;
    for (int i = 0; i < len; ++i) {
      string to_open = directory + files_to_touch[i];
      FILE *f = fopen(to_open.c_str(), "w");
      fclose(f);
    }

    _item_file = fopen((directory + "item").c_str(), "w");
    _result_file = fopen((directory + "result").c_str(), "w");
    _parse_file = fopen((directory + "parse").c_str(), "w");
    if (_item_file == NULL || _result_file == NULL || _parse_file == NULL) {
      if (_item_file != NULL) {
        fclose(_item_file); _item_file = NULL;
      }
      if (_result_file != NULL) {
        fclose(_result_file); _result_file = NULL;
      }
      if (_parse_file != NULL) {
        fclose(_parse_file); _parse_file = NULL;
      }
    }
  }
}

tTsdbDump::~tTsdbDump() {
  if (_current != NULL) delete _current;
  if (_item_file != NULL) fclose(_item_file);
  if (_result_file != NULL) fclose(_result_file);
  if (_parse_file != NULL) fclose(_parse_file);
}

void tTsdbDump::start() {
  if(_current != NULL) delete _current;
  if (active()) {
    _current = new tsdb_parse();
  }
}

void tTsdbDump::finish(chart *Chart, const string &input) {
  if (_current != NULL) {
    _current->set_input(input);
    _current->set_i_length(Chart->length()-1);
    cheap_tsdb_summarize_item(*Chart, Chart->rightmost(), -1, 0, *_current);
    dump_current();
  }
}

void tTsdbDump::error(class chart *Chart, const string &input,
                      const class tError & e){
  if (_current != NULL) {
    if(Chart) {
      _current->set_input(input);
      _current->set_i_length(Chart->length()-1);
    }
    list<tError> errors;
    errors.push_back(e);
    cheap_tsdb_summarize_error(errors, -1, *_current);
    dump_current();
  }
}

void tTsdbDump::dump_current() {
  if (active() && (_current != NULL)) {
    _current->file_print(_parse_file, _result_file, _item_file);
    delete _current;
    _current = 0;
  }
}

bool tTsdbDump::print_run(string directory) {
  FILE *run_file = fopen((directory + "run").c_str(), "w");
  if (run_file) {
    // run-id, run-comment, platform, tsdb version, application
    fprintf(run_file, "1@@%s@2.0@%s@", CHEAP_PLATFORM, CHEAP_VERSION);
    // environment
    map<string, string> properties = Grammar->properties();
    for(map<string, string>::iterator it = properties.begin();
        it != properties.end(); ++it) {
      fprintf(run_file, "(:%s . \\\"%s\\\") ",
              it->first.c_str(), it->second.c_str());
    }
    // grammar, avms, sorts, templates, lexicon, lrules, rules
    fprintf(run_file, "@%s@%d@%d@%s@%d@%d@%d@",
            Grammar->property("version").c_str(),
            nstatictypes,
            -1,
            (Grammar->property("ntemplates").empty()
             ? ""
             : Grammar->property("ntemplates").c_str()),
            Grammar->nstems(),
            -1,
            Grammar->rules().size()
            );
    // user, host, os, start, end, items, status
    const char *user = getenv("USER");
    char host[256];
    gethostname(host, 256);
    fprintf(run_file, "%s@%s@%s@%s@%s@%d@%s@",
            (user == NULL ? "anon" : user),
            host,
            "unknown",
            "01-01-1970 12:00:00",
            "01-01-1970 12:00:00",
            -1,
            "unknown");

    // capi_printf("(:leafs . %d) ", nstatictypes - first_leaftype); ??
    fclose(run_file);
    return false;
  }
  return true;
}

bool tTsdbDump::print_relations(string directory) {
  FILE *relations_file = fopen((directory + "relations").c_str(), "w");
  if (relations_file) {
    fprintf(relations_file, "%s\n", "\n\
item:\n\
  i-id :integer :key\n\
  i-origin :string\n\
  i-register :string\n\
  i-format :string\n\
  i-difficulty :integer\n\
  i-category :string\n\
  i-input :string\n\
  i-tokens :string\n\
  i-gloss :string\n\
  i-translation :string\n\
  i-wf :integer\n\
  i-length :integer\n\
  i-comment :string\n\
  i-author :string\n\
  i-date :date\n\
\n\
analysis:\n\
  i-id :integer :key\n\
  a-position :string\n\
  a-instance :string\n\
  a-category :string\n\
  a-function :string\n\
  a-domain :string\n\
  a-tag :string\n\
  a-comment :string\n\
\n\
phenomenon:\n\
  p-id :integer :key\n\
  p-name :string\n\
  p-supertypes :string\n\
  p-presupposition :string\n\
  p-interaction :string\n\
  p-purpose :string\n\
  p-restrictions :string\n\
  p-comment :string\n\
  p-author :string\n\
  p-date :date\n\
\n\
parameter:\n\
  ip-id :integer :key\n\
  position :string\n\
  attribute :string\n\
  value :string\n\
  instance :string\n\
  pa-comment :string\n\
\n\
set:\n\
  s-id :integer :key\n\
  p-id :integer :key :partial\n\
  s-author :string\n\
  s-date :date\n\
\n\
item-phenomenon:\n\
  ip-id :integer :key\n\
  i-id :integer :key\n\
  p-id :integer :key\n\
  ip-author :string\n\
  ip-date :date\n\
\n\
item-set:\n\
  i-id :integer :key :partial\n\
  s-id :integer :key\n\
  polarity :integer\n\
\n\
run:\n\
  run-id :integer :key                  # unique test run identifier\n\
  run-comment :string                   # descriptive narrative\n\
  platform :string                      # implementation platform (version)\n\
  tsdb :string                          # tsdb(1) (version) used\n\
  application :string                   # application (version) used\n\
  environment :string                   # application-specific information\n\
  grammar :string                       # grammar (version) used\n\
  avms :integer                         # number of avm types in image\n\
  sorts :integer                        # number of sort types in image\n\
  templates :integer                    # number of templates in image\n\
  lexicon :integer                      # number of lexical entries\n\
  lrules :integer                       # number of lexical rules\n\
  rules :integer                        # number of (non-lexical) rules\n\
  user :string                          # user who did the test run\n\
  host :string                          # machine used for this run\n\
  os :string                            # operating system (version)\n\
  start :date                           # start time of this test run\n\
  end :date                             # end time for this test run\n\
  items :integer                        # number of test items in this run\n\
  status :string                        # exit status (PVM only)\n\
\n\
parse:\n\
  parse-id :integer :key                # unique parse identifier\n\
  run-id :integer :key                  # test run for this parse\n\
  i-id :integer :key                    # item parsed\n\
  p-input :string                       # initial (pre-processed) parser input\n\
  p-tokens :string                      # internal parser input: lexical lookup\n\
  readings :integer                     # number of readings obtained\n\
  first :integer                        # time to find first reading (msec)\n\
  total :integer                        # total time for parsing (msec)\n\
  tcpu :integer                         # total (cpu) processing time (msec)\n\
  tgc :integer                          # gc time used (msec)\n\
  treal :integer                        # overall real time (msec)\n\
  words :integer                        # lexical entries retrieved\n\
  l-stasks :integer                     # successful lexical rule applications\n\
  p-ctasks :integer                     # parser contemplated tasks (LKB)\n\
  p-ftasks :integer                     # parser filtered tasks\n\
  p-etasks :integer                     # parser executed tasks\n\
  p-stasks :integer                     # parser succeeding tasks\n\
  aedges :integer                       # active items in chart (PAGE)\n\
  pedges :integer                       # passive items in chart\n\
  raedges :integer                      # active items contributing to result\n\
  rpedges :integer                      # passive items contributing to result\n\
  unifications :integer                 # number of (node) unifications\n\
  copies :integer                       # number of (node) copy operations\n\
  conses :integer                       # cons() cells allocated\n\
  symbols :integer                      # symbols allocated\n\
  others :integer                       # bytes of memory allocated\n\
  gcs :integer                          # number of garbage collections\n\
  i-load :integer                       # initial load (start of parse)\n\
  a-load :integer                       # average load\n\
  date :date                            # date and time of parse\n\
  error :string                         # error string (if applicable |:-)\n\
  comment :string                       # application-specific comment\n\
\n\
result:\n\
  parse-id :integer :key                # parse for this result\n\
  result-id :integer                    # unique result identifier\n\
  time :integer                         # time to find this result (msec)\n\
  r-ctasks :integer                     # parser contemplated tasks\n\
  r-ftasks :integer                     # parser filtered tasks\n\
  r-etasks :integer                     # parser executed tasks\n\
  r-stasks :integer                     # parser succeeding tasks\n\
  size :integer                         # size of feature structure\n\
  r-aedges :integer                     # active items for this result\n\
  r-pedges :integer                     # passive items in this result\n\
  derivation :string                    # derivation tree for this reading\n\
  surface :string                       # surface string (e.g. realization)\n\
  tree :string                          # phrase structure tree (CSLI labels)\n\
  mrs :string                           # mrs for this reading\n\
  flags :string                         # arbitrary annotation (e.g. BLEU)\n\
\n\
rule:\n\
  parse-id :integer :key                # parse for this rule summary\n\
  rule :string                          # rule name\n\
  filtered :integer                     # rule postulations filtered\n\
  executed :integer                     # rule postulations executed\n\
  successes :integer                    # successful rule postulations\n\
  actives :integer                      # active edges built from this rule\n\
  passives :integer                     # passive edges built from this rule\n\
\n\
output:\n\
  i-id :integer :key                    # item for this output specification\n\
  o-application :string                 # applicable application(s)\n\
  o-grammar :string                     # applicable grammar(s)\n\
  o-ignore :string                      # ignore this item flag\n\
  o-wf :integer                         # application-specific grammaticality\n\
  o-gc :integer                         # maximal number of garbage collections\n\
  o-derivation :string                  # expected derivation\n\
  o-surface :string                     # expected surface string\n\
  o-tree :string                        # expected phrase structure tree\n\
  o-mrs :string                         # expected mrs\n\
  o-edges :integer                      # maximal number of edges to build\n\
  o-user :string                        # author of this output specification\n\
  o-date :date                          # creation date\n\
\n\
edge:\n\
  e-id :integer :key                    # unique edge identifier\n\
  parse-id :integer :key                # parse for this edge\n\
  e-name :string                        # edge label (as in `derivation')\n\
  e-status :integer                     # 0 : passive; 1 : active\n\
  e-result :integer                     # 0 : nope; 1 : yup, result\n\
  e-start :integer                      # start vertex for this edge\n\
  e-end :integer                        # end vertex for this edge\n\
  e-daughters :string                   # (Common-Lisp) list of daughters\n\
  e-parents :string                     # (Common-Lisp) list of parents\n\
  e-alternates :string                  # alternates packed into this edge\n\
\n\
tree:\n\
  parse-id :integer :key\n\
  t-version :integer\n\
  t-active :integer :key\n\
  t-confidence :integer\n\
  t-author :string\n\
  t-start :date\n\
  t-end :date\n\
  t-comment :string\n\
\n\
decision:\n\
  parse-id :integer :key\n\
  t-version :integer\n\
  d-state :integer\n\
  d-type :integer\n\
  d-key :string\n\
  d-value :string\n\
  d-start :integer\n\
  d-end :integer\n\
  d-date :date\n\
\n\
preference:\n\
  parse-id :integer :key\n\
  t-version :integer\n\
  result-id :integer\n\
\n\
update:\n\
  parse-id :integer :key\n\
  t-version :integer\n\
  u-matches :integer\n\
  u-mismatches :integer\n\
  u-new :integer\n\
  u-gin :integer\n\
  u-gout :integer\n\
  u-pin :integer\n\
  u-pout :integer\n\
  u-in :integer\n\
  u-out :integer\n\
\n\
fold:\n\
  f-id :integer :key\n\
  f-train :integer\n\
  f-trains :string\n\
  f-test :integer\n\
  f-tests :string\n\
  f-events :integer\n\
  f-features :integer\n\
  f-environment :string\n\
  f-iterations :integer\n\
  f-etime :integer\n\
  f-estimation :string\n\
  f-accuracy :string\n\
  f-extras :string\n\
  f-user :string\n\
  f-host :string\n\
  f-start :date\n\
  f-end :date\n\
  f-comment :string\n\
\n\
score:\n\
  parse-id :integer :key\n\
  result-id :integer\n\
  score-start :integer\n\
  score-end :integer\n\
  score-id :integer\n\
  learner :string\n\
  rank :integer\n\
  score :string\n\
");
    fclose(relations_file);
    return false;
  }
  return true;
}

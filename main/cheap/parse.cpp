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

/* parser control */

#include "pet-system.h"

#include "cheap.h"
#include "parse.h"
#include "fs.h"
#include "item.h"
#include "grammar.h"
#include "chart.h"
#include "inputchart.h"
#include "tokenizer.h"
#include "agenda.h"
#include "tsdb++.h"

#ifdef YY
#include "yy.h"
#endif

//
// global variables for parsing
//

chart *Chart;
agenda *Agenda;

timer *ParseTime;
timer TotalParseTime(false);

//
// filtering
//

bool
filter_rule_task(grammar_rule *R, item *passive)
{

#ifdef DEBUG
    fprintf(ferr, "trying "); R->print(ferr);
    fprintf(ferr, " & passive "); passive->print(ferr);
    fprintf(ferr, " ==> ");
#endif

    if(opt_filter && !Grammar->filter_compatible(R, R->nextarg(),
                                                 passive->rule()))
    {
        stats.ftasks_fi++;

#ifdef DEBUG
        fprintf(ferr, "filtered (rf)\n");
#endif

        return false;
    }

    if(opt_nqc != 0 && !qc_compatible(R->qc_vector(R->nextarg()),
                                      passive->qc_vector()))
    {
        stats.ftasks_qc++;

#ifdef DEBUG
        fprintf(ferr, "filtered (qc)\n");
#endif

        return false;
    }

#ifdef DEBUG
    fprintf(ferr, "passed filters\n");
#endif

    return true;
}

bool
filter_combine_task(item *active, item *passive)
{
#ifdef DEBUG
    fprintf(ferr, "trying active "); active->print(ferr);
    fprintf(ferr, " & passive "); passive->print(ferr);
    fprintf(ferr, " ==> ");
#endif

    if(opt_filter && !Grammar->filter_compatible(active->rule(),
                                                 active->nextarg(),
                                                 passive->rule()))
    {
#ifdef DEBUG
        fprintf(ferr, "filtered (rf)\n");
#endif

        stats.ftasks_fi++;
        return false;
    }

    if(opt_nqc != 0 && !qc_compatible(active->qc_vector(),
                                      passive->qc_vector()))
    {
#ifdef DEBUG
        fprintf(ferr, "filtered (qc)\n");
#endif

        stats.ftasks_qc++;
        return false;
    }

#ifdef DEBUG
    fprintf(ferr, "passed filters\n");
#endif

    return true;
}

//
// parser control
//

void
postulate(item *passive)
{
    // iterate over all the rules in the grammar
    for(rule_iter rule(Grammar); rule.valid(); rule++)
    {
        grammar_rule *R = rule.current();

        if(passive->compatible(R, Chart->rightmost()))
            if(filter_rule_task(R, passive))
                Agenda->push(New rule_and_passive_task(Chart, Agenda, R,
                                                       passive));
    }
}

void
fundamental_for_passive(item *passive)
{
    // iterate over all active items adjacent to passive and try combination
    for(chart_iter_adj_active it(Chart, passive); it.valid(); it++)
    {
        item *active = it.current();
        if(active->adjacent(passive))
            if(passive->compatible(active, Chart->rightmost()))
                if(filter_combine_task(active, passive))
                    Agenda->push(New active_and_passive_task(Chart, Agenda,
                                                             active, passive));
    }
}

void
fundamental_for_active(phrasal_item *active)
{
  // iterate over all passive items adjacent to active and try combination

  for(chart_iter_adj_passive it(Chart, active); it.valid(); it++)
    if(opt_packing == 0 || !it.current()->blocked())
      if(it.current()->compatible(active, Chart->rightmost()))
        if(filter_combine_task(active, it.current()))
          Agenda->push(New
                       active_and_passive_task(Chart, Agenda,
                                               active, it.current()));
}

bool
packed_edge(item *newitem)
{
    if(newitem->trait() == INFL_TRAIT)
      return false;

    for(chart_iter_span_passive iter(Chart, newitem->start(), newitem->end());
        iter.valid(); iter++)
    {
        bool forward, backward;
        item *olditem = iter.current();

	if(olditem->trait() == INFL_TRAIT)
          continue;

        subsumes(olditem->get_fs(), newitem->get_fs(), forward, backward);

        if(forward && !olditem->blocked())
        {
            if((!backward && (opt_packing & PACKING_PRO))
               || (backward && (opt_packing & PACKING_EQUI)))
            {
                if(verbosity > 4)
                {
                    fprintf(ferr, "proactive (%s) packing:\n", backward
                            ? "equi" : "subs");
                    newitem->print(ferr);
                    fprintf(ferr, "\n --> \n");
                    olditem->print(ferr);
                    fprintf(ferr, "\n");
                }
                
                if(backward)
                    stats.p_equivalent++;
                else
                    stats.p_proactive++;
                
                olditem->packed.push_back(newitem);
                return true;
            }
        }
      
        if(backward && (opt_packing & PACKING_RETRO) && !olditem->frosted())
        {
            if(verbosity > 4)
            {
                fprintf(ferr, "retroactive packing:\n");
                newitem->print(ferr);
                fprintf(ferr, " <- ");
                olditem->print(ferr);
                fprintf(ferr, "\n");
            }

	    newitem->packed.splice(newitem->packed.begin(), olditem->packed);

            if(!olditem->blocked())
            {
                stats.p_retroactive++;
                newitem->packed.push_back(olditem);
            }

            olditem->frost();

            // delete (old, chart)
        }
    }
    return false;
}

bool
add_root(item *it)
    // deals with result item
    // return value: true -> stop parsing; false -> continue parsing
{
    Chart->trees().push_back(it);
    stats.trees++;
    if(stats.first == -1)
    {
        stats.first = ParseTime->convert2ms(ParseTime->elapsed());
        if(opt_nsolutions > 0 && stats.trees >= opt_nsolutions)
            return true;
    }
    return false;
}

void
add_item(item *it)
{
    assert(!(opt_packing && it->blocked()));

#ifdef DEBUG
    fprintf(ferr, "add_item ");
    it->print(ferr);
    fprintf(ferr, "\n");
#endif

    if(it->passive())
    {
        if(opt_packing && packed_edge(it))
            return;

        Chart->add(it);

        type_t rule;
        if(it->root(Grammar, Chart->rightmost(), rule))
        {
            it->set_result_root(rule);
            if(add_root(it))
                return;
        }

        postulate(it);
        fundamental_for_passive(it);
    }
    else
    {
        Chart->add(it);
        fundamental_for_active(dynamic_cast<phrasal_item *> (it));
    }
}

inline bool
resources_exhausted()
{
    return (pedgelimit > 0 && Chart->pedges() >= pedgelimit) || 
        (memlimit > 0 && t_alloc.max_usage() >= memlimit);
}

void
parse(chart &C, list<lex_item *> &initial, fs_alloc_state &FSAS, 
      list<error> &errors)
{
    if(initial.empty()) return;

    unify_wellformed = true;

    Chart = &C;
    Agenda = New agenda;

    TotalParseTime.start();
    ParseTime = New timer;

    for(list<lex_item *>::iterator lex_it = initial.begin();
        lex_it != initial.end(); ++lex_it)
    {
        Agenda->push(New item_task(Chart, Agenda, *lex_it));
        stats.words++;
    }

    while(!Agenda->empty() &&
          (opt_nsolutions == 0 || stats.trees < opt_nsolutions) &&
#ifdef YY
          (opt_nth_meaning == 0 || stats.nmeanings < opt_nth_meaning) &&
#endif
          !resources_exhausted())
    {
        basic_task *t; item *it;
	  
        t = Agenda->pop();
        if((it = t->execute()) != 0)
            add_item(it);
	  
        delete t;
    }

    ParseTime->stop();
    TotalParseTime.stop();

    stats.tcpu = ParseTime->convert2ms(ParseTime->elapsed());

    stats.dyn_bytes = FSAS.dynamic_usage();
    stats.stat_bytes = FSAS.static_usage();
    FSAS.clear_stats();

    get_unifier_stats();
    C.get_statistics();

    if(opt_shrink_mem)
    {
        FSAS.may_shrink();
        prune_glbcache();
    }

    if(verbosity > 8)
        C.print(fstatus);
  
    delete ParseTime;
    delete Agenda;

    if(resources_exhausted())
    {
        ostringstream s;

        if(pedgelimit == 0 || Chart->pedges() < pedgelimit)
            s << "memory limit exhausted (" << memlimit / (1024 * 1024) 
              << " MB)";
        else
            s << "edge limit exhausted (" << pedgelimit 
              << " pedges)";

        errors.push_back(s.str());
    }

    if(opt_packing && !(opt_packing & PACKING_NOUNPACK))
    {
        timer *UnpackTime = New timer();
	int nres = 0;
        for(vector<item *>::iterator tree = Chart->trees().begin();
            tree != Chart->trees().end(); ++tree)
        {
            if((*tree)->blocked())
                continue;

            stats.p_trees++;

            list<item *> results;
            int upedgelimit = pedgelimit ? pedgelimit - Chart->pedges() : 0;
            results = (*tree)->unpack(upedgelimit);
            
            for(list<item *>::iterator res = results.begin();
                res != results.end(); ++res)
            {
                type_t rule;
                if((*res)->root(Grammar, Chart->rightmost(), rule))
                {
                    Chart->readings().push_back(*res);
		    stats.readings++;
                    if(verbosity > 2)
                    {
                        fprintf(stderr, "unpacked[%d] (%.1f): ", nres++,
                                UnpackTime->convert2ms(UnpackTime->elapsed()) / 1000.);
                        (*res)->print_derivation(stderr, false);
                        fprintf(stderr, "\n");
                    }
                }
            }
            if(upedgelimit > 0 && stats.p_upedges > upedgelimit)
            {
                ostringstream s;

                s << "unpack edge limit exhausted (" << upedgelimit 
                  << " pedges)";

                errors.push_back(s.str());
                break;
            }
        }
        stats.p_utcpu = UnpackTime->convert2ms(UnpackTime->elapsed());
        stats.p_dyn_bytes = FSAS.dynamic_usage();
        stats.p_stat_bytes = FSAS.static_usage();
        FSAS.clear_stats();
        delete UnpackTime;
    }
    else
    {
        stats.readings = stats.trees;
        Chart->readings() = Chart->trees();
    }
    if(Grammar->sm())
    {
        sort(Chart->readings().begin(), Chart->readings().end(),
             greater_than_score(Grammar->sm()));
    }
}

void
analyze(input_chart &i_chart, string input, chart *&C,
        fs_alloc_state &FSAS, list<error> &errors, int id)
{
    FSAS.clear_stats();
    stats.reset();
    stats.id = id;

    auto_ptr<item_owner> owner(New item_owner);
    item::default_owner(owner.get());

#ifdef YY
    if(opt_yy)
        i_chart.populate(New yy_tokenizer(input));
    else
#endif
        i_chart.populate(New lingo_tokenizer(input));

    list<lex_item *> lex_items;
    int max_pos = i_chart.expand_all(lex_items);

    dependency_filter(lex_items,
                      cheap_settings->lookup("chart-dependencies"),
                      cheap_settings->lookup(
                          "unidirectional-chart-dependencies") != 0);
        
    if(opt_default_les)
        i_chart.add_generics(lex_items);

    // Discount priorities of lexical items that are covered by a larger
    // multiword lexical item.
    i_chart.discount_covered_items(lex_items);

    // Perform final adjustment of lexical item priorities for
    // special postags (like SpellCorrected)
    for(list<lex_item *>::iterator lex_it = lex_items.begin();
        lex_it != lex_items.end(); ++lex_it)
    {
        (*lex_it)->adjust_priority("posdiscount");
    }

    if(verbosity > 9)
        i_chart.print(ferr);

    string missing = i_chart.uncovered(i_chart.gaps(max_pos, lex_items));
    if (!missing.empty()) 
        throw error("no lexicon entries for " + missing) ;

    C = Chart = New chart(max_pos, owner);

    parse(*Chart, lex_items, FSAS, errors);
}

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
#include "k2y.h"
#endif

//#define DEBUG_DEFER

//
// global variables for parsing
//

chart *Chart;
agenda *Agenda;

timer *Clock;
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

    // when making changes here, don't forget to change the excursion for
    // hyperactive parsing in task.cpp accordingly

    if(opt_hyper)
    {
        // avoid processing tasks already done in the `excursion'
        for(chart_iter_adj_passive it(Chart, active); it.valid(); it++)
            if(it.current()->stamp() > active->done())
#ifdef PACKING
                if(it.current()->frozen() == 0)
#endif
                    if(it.current()->compatible(active, Chart->rightmost()))
                        if(filter_combine_task(active, it.current()))
                            Agenda->push(New
                                active_and_passive_task(Chart, Agenda,
                                                        active, it.current()));
    }
    else
    {
        for(chart_iter_adj_passive it(Chart, active); it.valid(); it++)
#ifdef PACKING
            if(it.current()->frozen() == 0)
#endif
                if(it.current()->compatible(active, Chart->rightmost()))
                    if(filter_combine_task(active, it.current()))
                        Agenda->push(New
                            active_and_passive_task(Chart, Agenda,
                                                    active, it.current()));
    }
}

#ifdef PACKING

void
block(item *it, int mark)
{
    fprintf(ferr, "%sing ", mark == 1 ? "frost" : "freez");
    it->print(ferr);
    fprintf(ferr, "\n");

    if(it->passive() && (it->frozen() == 0 || mark == 2))
    {
        it->freeze(mark);
        if(it->frozen() == 0)
            Chart->pedges()--;
    }  

    item *parent;
    forall(parent, it->parents)
        block(parent, 2);
}

bool
packed_edge(item *newitem)
{
    for(chart_iter_span iter(Chart, newitem->start(), newitem->end());
        iter.valid(); iter++)
    {
        bool forward, backward;
        item *olditem = iter.current();

        subsumes(olditem->get_fs(), newitem->get_fs(), forward, backward);

        if(forward && olditem->frozen() == 0)
        {
            fprintf(ferr, "proactive (%s) packing:\n", backward
                    ? "equi" : "subs");
            newitem->print(ferr);
            fprintf(ferr, " -> ");
            olditem->print(ferr);
            fprintf(ferr, "\n");

            olditem->packed.push(newitem);
            return true;
        }
      
        if(backward)
        {
            fprintf(ferr, "retroactive packing:\n");
            newitem->print(ferr);
            fprintf(ferr, " <- ");
            olditem->print(ferr);
            fprintf(ferr, "\n");
	  
            newitem->packed.conc(olditem->packed);
            olditem->packed.clear();

            if(olditem->frozen() == 0)
                newitem->packed.push(olditem);

            block(olditem, 1);

            // delete (old, chart)
        }
    }
    return false;
}

#endif

bool
add_root(item *it)
    // deals with result item
    // return value: true -> stop parsing; false -> continue parsing
{
    Chart->Roots().push_back(it);
    stats.readings++;
    if(stats.first == -1)
    {
        stats.first = Clock->convert2ms(Clock->elapsed());
        if(opt_nsolutions > 0 && stats.readings >= opt_nsolutions)
            return true;
    }
#ifdef YY
    if(opt_k2y && opt_nth_meaning != 0)
    {
        int n = construct_k2y(0, it, true, 0);
        if(n >= 0)
        {
            stats.nmeanings++;
            if(stats.nmeanings >= opt_nth_meaning)
                return true;
        }
    }
#endif
    return false;
}

void
add_item(item *it)
{
#ifdef PACKING
    if(it->frozen())
    {
        fprintf(ferr, "ignoring ");
        it->print(ferr);
        fprintf(ferr, "\n");
        return;
    }
#endif

#ifdef DEBUG
    fprintf(ferr, "add_item ");
    it->print(ferr);
    fprintf(ferr, "\n");
#endif

    if(it->in_chart())
    {
        // item is already in chart -> this is a deferred root node
#ifdef DEBUG_DEFER
        fprintf(ferr, " -> deferred root item\n");
#endif
        add_root(it);
        return;
    }

    if(it->passive())
    {
#ifdef PACKING
        if(packed_edge(it))
            return;
#endif
        Chart->add(it);

        type_t rule;
        int maxp; 
        if(it->root(Grammar, Chart->rightmost(), rule, maxp))
        {
            it->rriority(maxp);
            it->set_result_root(rule);
            // we found a root item - it might be too early
            if(maxp != 0 && it->priority() > maxp)
            {
#ifdef DEBUG_DEFER
                fprintf(ferr, " -> root, but it's too early\n");
#endif
                Agenda->push(New item_task(Chart, Agenda, it, maxp));
            }
            else
            {
#ifdef DEBUG_DEFER
                fprintf(ferr, " -> root on time\n");
#endif
                if(add_root(it))
                    return;
            }
        }

        postulate(it);
        fundamental_for_passive(it);
    }
    else
    {
        Chart->add(it);
#ifndef CRASHES_ON_DYNAMIC_CASTS
        fundamental_for_active(dynamic_cast<phrasal_item *> (it));
#else
        fundamental_for_active((phrasal_item *) it);
#endif
    }
#ifdef DEBUG_DEFER
    fprintf(ferr, "\n");
#endif
}

inline bool
ressources_exhausted()
{
    return (pedgelimit > 0 && Chart->pedges() >= pedgelimit) || 
        (memlimit > 0 && t_alloc.max_usage() >= memlimit);
}

void
get_statistics(chart &C, timer *Clock, fs_alloc_state &FSAS);

void
parse(chart &C, list<lex_item *> &initial, fs_alloc_state &FSAS)
{
    if(initial.empty()) return;

    unify_wellformed = true;

    Chart = &C;
    Agenda = New agenda;

    TotalParseTime.start();
    Clock = New timer;

    for(list<lex_item *>::iterator lex_it = initial.begin();
        lex_it != initial.end(); ++lex_it)
    {
        Agenda->push(New item_task(Chart, Agenda, *lex_it));
        stats.words++;
    }

    while(!Agenda->empty() &&
          (opt_nsolutions == 0 || stats.readings < opt_nsolutions) &&
#ifdef YY
          (opt_nth_meaning == 0 || stats.nmeanings < opt_nth_meaning) &&
#endif
          !ressources_exhausted())
    {
        basic_task *t; item *it;
	  
        t = Agenda->pop();
        if((it = t->execute()) != 0)
            add_item(it);
	  
        delete t;
    }

    TotalParseTime.stop();
    get_statistics(C, Clock, FSAS);

    if(opt_shrink_mem)
    {
        FSAS.may_shrink();
        prune_glbcache();
    }

    if(verbosity > 8)
        C.print(fstatus);
  
    delete Clock;
    delete Agenda;

    if(Chart->Roots().empty() && ressources_exhausted())
    {
        if(pedgelimit == 0 || Chart->pedges() < pedgelimit)
            throw error_ressource_limit("memory (MB)",
                                        memlimit / (1024 * 1024));
        else
            throw error_ressource_limit("pedges", pedgelimit);
    }
}

void
analyze(input_chart &i_chart, string input, chart *&C,
        fs_alloc_state &FSAS, int id)
{
    FSAS.clear_stats();
    stats.reset();
    stats.id = id;

#ifdef YY
    if(opt_k2y)
        mrs_reset_mappings();
#endif

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

    parse(*Chart, lex_items, FSAS);
}

void
get_statistics(chart &C, timer *Clock, fs_alloc_state &FSAS)
{
    stats.tcpu = Clock->convert2ms(Clock->elapsed());
    stats.dyn_bytes = FSAS.dynamic_usage();
    stats.stat_bytes = FSAS.static_usage();

    get_unifier_stats();

    C.get_statistics();
}
/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* parser control */

#include <string.h>
#include <string>
using namespace std;
#include <vector>
#include <queue>
#include "cheap.h"
#include "parse.h"
#include "fs.h"
#include "item.h"
#include "grammar.h"
#include "chart.h"
#include "agenda.h"
#include "tsdb++.h"

#ifdef YY
#  include "k2y.h"
#endif

#ifdef __BORLANDC__
#define strcasecmp strcmpi
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

bool filter_rule_task(grammar_rule *R, item *passive)
{

#ifdef DEBUG
  fprintf(ferr, "trying "); R->print(ferr);
  fprintf(ferr, " & passive "); passive->print(ferr);
  fprintf(ferr, " ==> ");
#endif

  if(opt_filter && !Grammar->filter_compatible(R, R->nextarg(), passive->rule()))
    {
      stats.ftasks_fi++;

#ifdef DEBUG
      fprintf(ferr, "filtered (rf)\n");
#endif

      return false;
    }

  if(opt_nqc != 0 && !qc_compatible(R->qc_vector(R->nextarg()), passive->qc_vector()))
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

bool filter_combine_task(item *active, item *passive)
{
#ifdef DEBUG
  fprintf(ferr, "trying active "); active->print(ferr);
  fprintf(ferr, " & passive "); passive->print(ferr);
  fprintf(ferr, " ==> ");
#endif

  if(opt_filter && !Grammar->filter_compatible(active->rule(), active->nextarg(), passive->rule()))
    {
#ifdef DEBUG
      fprintf(ferr, "filtered (rf)\n");
#endif

      stats.ftasks_fi++;
      return false;
    }

  if(opt_nqc != 0 && !qc_compatible(active->qc_vector(), passive->qc_vector()))
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

void postulate(item *passive, tokenlist *Input)
{
  // iterate over all the rules in the grammar
  for(rule_iter rule(Grammar); rule.valid(); rule++)
    {
      grammar_rule *R = rule.current();

      if(!opt_shaping || passive->shape_fits(R, Input->size()))
	if(filter_rule_task(R, passive))
	  Agenda->push(new rule_and_passive_task(R, passive));
    }
}

void fundamental_for_passive(item *passive)
{
  // iterate over all active items adjacent to passive and try combination
  for(chart_iter_adj_active it(Chart, passive); it.valid(); it++)
    {
      item *active = it.current();
      if(active->adjacent(passive))
	if(filter_combine_task(active, passive))
	  Agenda->push(new active_and_passive_task(active, passive));
    }
}

void fundamental_for_active(phrasal_item *active)
{
  // iterate over all passive items adjacent to active and try combination
  if(opt_hyper)
    {
      // avoid processing tasks already done in the `excursion'
      for(chart_iter_adj_passive it(Chart, active); it.valid(); it++)
        if(it.current()->stamp() > active->done())
#ifdef PACKING
	if(it.current()->frozen() == 0)
#endif
          if(filter_combine_task(active, it.current()))
            Agenda->push(new active_and_passive_task(active, it.current()));
    }
  else
    {
      for(chart_iter_adj_passive it(Chart, active); it.valid(); it++)
#ifdef PACKING
	if(it.current()->frozen() == 0)
#endif
        if(filter_combine_task(active, it.current()))
          Agenda->push(new active_and_passive_task(active, it.current()));
    }
}

#ifdef PACKING

void block(item *it, int mark)
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

bool packed_edge(item *newitem)
{
  for(chart_iter_span iter(Chart, newitem->start(), newitem->end());
      iter.valid(); iter++)
    {
      bool forward, backward;
      item *olditem = iter.current();

      subsumes(olditem->get_fs(), newitem->get_fs(), forward, backward);

      if(forward && olditem->frozen() == 0)
	{
	  fprintf(ferr, "proactive (%s) packing:\n", backward ? "equi" : "subs");
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

bool add_root(item *it, tokenlist *Input)
// deals with result item
// return value: true -> stop parsing; false -> continue parsing
{
  stats.readings++;
  it->set_result(true);
  if(stats.first == -1)
    {
      stats.first = Clock->convert2ms(Clock->elapsed());
      if(opt_one_solution) 
	return true;
    }
#ifdef YY
  if(opt_k2y && opt_one_meaning)
    {
      int n = construct_k2y(0, it, true, 0);
      if(n >= 0)
	{
	  stats.nmeanings++;
	  return true;
	}
    }
#endif
  return false;
}

void add_item(item *it, tokenlist *Input)
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
      add_root(it, Input);
      return;
    }

  if(it->passive())
    {
#ifdef PACKING
      if(packed_edge(it))
	return;
#endif
      Chart->add(it);

      int maxp; 
      if(it->root(Grammar, Input, maxp))
	{
	  // we found a root item - it might be too early
	  if(maxp != 0 && it->priority() > maxp)
	    {
#ifdef DEBUG_DEFER
	      fprintf(ferr, " -> root, but it's too early\n");
#endif
	      Agenda->push(new item_task(it, maxp));
	    }
	  else
	    {
#ifdef DEBUG_DEFER
	      fprintf(ferr, " -> root on time\n");
#endif
	      if(add_root(it, Input))
		return;
	    }
	}

      postulate(it, Input);
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

void add_generics(setting *generics, tokenlist *Input, int i, int offset, postags onlyfor = postags())
// Add generic lexical entries for token at position `i'. If `onlyfor' is 
// non-empty, only those generic entries corresponding to one of those
// POS tags are postulated. The correspondence is defined in posmapping.
{
  if(!generics)
    return;

#ifdef DEBUG
  fprintf(ferr, 
	  "using generics for `%s' @ %d:\n", 
	  (*Input)[i].c_str(), i);
#endif
  for(int j = 0; j < generics->n; j++)
    {
      char *suffix = cheap_settings->assoc("generic-le-suffixes",
					   generics->values[j]);
      if(suffix)
	if((*Input)[i].length() <= strlen(suffix) ||
	   strcasecmp(suffix,
		      (*Input)[i].c_str()+(*Input)[i].length()-strlen(suffix)) != 0)
	  continue;

      int inst = lookup_type(generics->values[j]);
      if(inst == -1)
	{
	  fprintf(ferr, "couldn't lookup generic `%s'\n",
		  generics->values[j]);
	  throw error("undefined generic");
	}
	  
      if(!onlyfor.empty())
	{
	  setting *set = cheap_settings->lookup("posmapping");
	  if(set)
	    {
	      bool good = false;
	      for(int i = 0; i < set->n; i+=3)
		{
		  if(i+3 > set->n)
		    {
		      fprintf(ferr, "warning: incomplete last entry in POS mapping - ignored\n");
		      break;
		    }
      
		  char *lhs = set->values[i], *rhs = set->values[i+2];

		  int type = lookup_type(rhs);
		  if(type == -1)
		    {
		      fprintf(ferr, "warning: unknown type `%s' in POS mapping\n",
			      rhs);
		    }
		  else
		    {
		      if(subtype(inst, type))
			{
			  if(onlyfor.contains(lhs))
			    {
			      good = true;
			      break;
			    }
			}
		    }
		}
	      if(!good)
		continue;
	    }
	}

#ifdef DEBUG
      fprintf(ferr, "  ==> %s [%s]\n", generics->values[j], 
	      suffix == 0 ? "*" : suffix);
#endif
	  
      generic_le_item *lex =
	new generic_le_item(inst, i, 
			    (*Input)[i].c_str(), Input->POS(i), 750);
      
      if(lex)
	{
	  lex->skew(offset);
	  Chart->own(lex);
	  Agenda->push(new item_task(lex));
	  stats.words++;
	}
    }
}

int initialize_chart(tokenlist *Input)
{
  vector<bool> in_lexicon(Input->length(), false);
  vector<bool> punctuation(Input->length(), false);
  int offset = 0;

  vector<postags> missingpos(Input->length(), postags());

  // for chart manipulation
  struct setting *deps = cheap_settings->lookup("chart-dependencies"); 
  vector< set<int> > satisfied;
  if(deps)
    satisfied.resize(deps->n);
  map<lex_item *, pair<int, int> > requires;

  // propose lexical items
  list<lex_item *> proposed_les;
  for(int i = 0; i < Input->length(); i++)
    {
      if(verbosity > 3 && !Input->POS(i).empty())
        {
          fprintf(ferr, "POS tags for `%s':", (*Input)[i].c_str());
          Input->POS(i).print(ferr);
   	  fprintf(ferr, "\n");
	}

      // start out assuming everything is missing
      missingpos[i] = Input->POS(i);

#ifdef YY
      if(opt_yy && ((*Input)[i].empty() || Input->punctuationp(i))) {
        punctuation[i] = true;
        offset++;
        continue;
      } /* if */
#endif

      for(le_iter it(Grammar, i, Input); it.valid(); it++)
	{
	  fs f = it.current()->instantiate();
	  if(f.valid())
	    {

	      lex_item *lex = 
                new lex_item(it.current(), i, f, Input->POS(i), offset);


	      if(!lex)
		{
		  fprintf(ferr, "couldn't construct lex_item for %s\n",
			  it.current()->description().c_str());
		  continue;
		}

#ifdef DEBUG
	      fprintf(ferr, "found le for `%s' at %d:\n",
		      (*Input)[i].c_str(), i);
	      lex->print(ferr);
	      fprintf(ferr, "\n");
#endif

	      if(opt_chart_man && deps)
		{
#ifdef DEBUG_DEP
		  fprintf(ferr, "dependency information for %s(`%s'):\n",
			  it.current()->description().c_str(),
			  (*Input)[i].c_str());
#endif

		  for(int j = 0; j < deps->n; j++)
		    {
		      fs v = f.get_path_value(deps->values[j]);
		      if(v.valid())
			{
#ifdef DEBUG_DEP
			  fprintf(ferr, "  %s : %s\n",
				  deps->values[j], v.name());
#endif
			  satisfied[j].insert(v.type());
			  requires[lex].first = (j % 2 == 0) ? j + 1 : j - 1;
			  requires[lex].second  = v.type();
			}
		    }
		}
	      proposed_les.push_back(lex);
	    }
	}
    }

  for(list<lex_item *>::iterator iter = proposed_les.begin(); iter != proposed_les.end(); ++iter)
    {
      lex_item *lex = *iter;
     
      if(requires.find(lex) != requires.end())
	{
	  pair<int, int> req = requires[lex];
	  if(verbosity > 2)
	    fprintf(ferr, "`%s' requires %s at %d -> ",
		    lex->description().c_str(),
		    typenames[req.second], req.first);
	  
	  if(satisfied[req.first].find(req.second) == satisfied[req.first].end())
	    {
	      delete lex;
	      lex = 0;
	      stats.words_pruned++;
	    }

	  if(verbosity > 2)
	    fprintf(ferr, "%s satisfied\n", lex == 0 ? "not" : "");
	}
	 
      if(lex)
	{
          stats.words++;
          for(int j = lex->start(); j < lex->end(); j++)
	    {
	      in_lexicon[j] = true;
	      if(!lex->postag().empty())
		missingpos[j].remove(lex->postag());
	    }
          //
          // _hack_
          // while the language server sends punctuation and empty
          // tokens down our pipe, we end up with a skewed view of the
          // universe: input positions do not correspond to positions
          // in the chart :-{.                   (29-jan-01  -  oe@yy)
          //
          lex->skew(lex->offset());
	  Chart->own(lex);
          Agenda->push(new item_task(lex));
	}
    }
  

  //
  // record total number of (punctuation) tokens that were ignored
  //
  Input->skew(offset);

  // check for input tokens not covered by appropriate lexical entries
  string no_lex;
  setting *generics 
    = (opt_default_les ? cheap_settings->lookup("generic-les") : NULL);
  
  offset = 0;
  for(int i = 0; i < Input->length(); i++)
    {
      if(punctuation[i])
	{
	  offset++;
	  continue;
	}
      if(generics)
	{
	  // is there any lexical entry for this position? 
	  if(!in_lexicon[i]) 
	    {
	      add_generics(generics, Input, i, offset);
	    }
	  // even if there is one (or more) lexical entries, we
	  // still might want to postulate generic entries, for those
          // categories proposed by the tagger, but not represented by any
          // of the lexical entries that we found
	  else
	    {
	      if(!missingpos[i].empty())
		{
		  if(verbosity > 4)
		    {
		      fprintf(fstatus, 
			      "unsatisfied POS tag(s) for `%s':", 
			      (*Input)[i].c_str());
		      missingpos[i].print(fstatus);
		      fprintf(fstatus, "\n");
		    }
#ifdef EXPERIMENTAL
		  add_generics(generics, Input, i, offset, missingpos[i]);
#endif
		}
	    }
	}
      else
	{
	  if(!in_lexicon[i])
	  {
	    if(!no_lex.empty())
	      no_lex += ", ";
	    no_lex += "<" + string((*Input)[i]) + ">";
	  }
	}
    }

  if(!no_lex.empty())
    throw error("no lexicon entries for " + no_lex);

  return stats.words;
}

inline bool ressources_exhausted()
{
  return (pedgelimit > 0 && Chart->pedges() >= pedgelimit) || 
    (memlimit > 0 && t_alloc.max_usage() >= memlimit);
}

void get_statistics(chart &C, timer *Clock, fs_alloc_state &FSAS);

void parse(chart &C, tokenlist *Input, int id)
{
  C.FSAS.clear_stats();
  stats.reset();
  stats.id = id;

  unify_wellformed = true;

  Chart = &C;
  Agenda = new agenda;
  TotalParseTime.start();
  Clock = new timer;

  if(initialize_chart(Input) > 0)
    {
      while(!Agenda->empty() &&
	    (opt_one_solution == false || stats.first == -1) &&
#ifdef YY
	    (opt_one_meaning == false || !stats.nmeanings) &&
#endif
	    !ressources_exhausted())
	{
	  basic_task *t; item *it;
	  
	  t = Agenda->pop();
	  if((it = t->execute()) != 0)
	    add_item(it, Input);
	  
	  delete t;
	}

      TotalParseTime.stop();
      get_statistics(C, Clock, C.FSAS);

      if(opt_shrink_mem)
	{
	  C.FSAS.may_shrink();
	  prune_glbcache();
	}

      if(verbosity > 8)
	C.print(fstatus);
    }

  delete Clock;
  delete Agenda;

  if(ressources_exhausted())
    {
      if(pedgelimit == 0 || stats.pedges < pedgelimit)
	throw error_ressource_limit("memory (MB)", memlimit / (1024 * 1024));
      else
	throw error_ressource_limit("pedges", pedgelimit);
    }
}

void get_statistics(chart &C, timer *Clock, fs_alloc_state &FSAS)
{
  stats.tcpu = Clock->convert2ms(Clock->elapsed());
  stats.dyn_bytes = FSAS.dynamic_usage();
  stats.stat_bytes = FSAS.static_usage();

  get_unifier_stats();

  C.get_statistics();
}

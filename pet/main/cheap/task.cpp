/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* tasks */

#include "task.h"
#include "tsdb++.h"
#include "options.h"
#include "parse.h"

#define HYPERACTIVE_EXKURS

int basic_task::next_id = 0;

item *build_combined_item(item *active, item *passive);

item *build_rule_item(grammar_rule *R, item *passive)
{
  fs_alloc_state FSAS(false);

  stats.etasks++;

  fs res;

  bool temporary = false;

  fs rule = R->instantiate();
  fs arg = R->nextarg(rule);

  if(!arg.valid())
    {
      fprintf(ferr, "trouble getting arg of rule\n");
      return NULL;
    }

  if(!opt_hyper || R->hyperactive() == false)
    {
      res = unify_restrict(rule,
			   passive->get_fs(),
			   arg,
                           R->arity() == 1 ? Grammar->deleted_daughters() : 0);
    }
  else
    {
      if(R->arity() > 1)
        {
          res = unify_np(rule, passive->get_fs(), arg);
          temporary = true;
        }
      else
        {
          res = unify_restrict(rule, passive->get_fs(), arg, Grammar->deleted_daughters());
        }
    }

  if(!res.valid())
    {
      FSAS.release();
      return NULL;
    }
  else
    {
      stats.stasks++;

      phrasal_item *it;

      if(temporary)
	{
	  temporary_generation save(res.temp());
	  it = new phrasal_item(R, passive, res);
	  Chart->own(it);
#ifdef HYPERACTIVE_EXKURS
	  // try to find a passive edge to complete this right away
          //
          // in best-first mode, it is more important to stick to the ordering
          // of tasks, as governed by the scoring mechanism. (9-apr-01  -  oe)
          //
          if(!opt_one_solution && !opt_one_meaning) {
            bool success = false;
            for(chart_iter_adj_passive iter(Chart, it); iter.valid(); iter++)
              {
                it->set_done(iter.current()->stamp());
                if(filter_combine_task(it, iter.current()))
                  {
                    item *it2 = build_combined_item(it, iter.current());
                    if(it2)
                      {
                        Agenda->push(new item_task(it2));
                        success = true;
                      }
                    
                    break;
                  }
              }
            if(!success) FSAS.release();
          } /* if */
#else
	    FSAS.release();
#endif
	}
      else
	{
	  it = new phrasal_item(R, passive, res);
	  Chart->own(it);
	}

      return it;
    }
}

item *build_combined_item(item *active, item *passive)
{
  fs_alloc_state FSAS(false);

  stats.etasks++;

  fs res;

  bool temporary = false;

  fs combined = active->get_fs();

  if(opt_hyper && combined.temp())
    unify_generation = combined.temp(); // XXX this might need to be reset

  fs arg = active->nextarg(combined);

  if(!arg.valid())
    {
      fprintf(ferr, "trouble getting arg of active item\n");
      return NULL;
    }

  if(!opt_hyper || active->rule()->hyperactive() == false)
    {
      res = unify_restrict(combined,
			   passive->get_fs(),
			   arg,
                           active->arity() == 1 ? Grammar->deleted_daughters() : 0); 
    }
  else
    {
      if(active->arity() > 1)
        {
          res = unify_np(combined, passive->get_fs(), arg);
          temporary = true;
        }
      else
        {
          res = unify_restrict(combined, passive->get_fs(), arg, Grammar->deleted_daughters());
        }
    }

  if(!res.valid())
    {
      FSAS.release();
      return NULL;
    }
  else
    {
      stats.stasks++;

      phrasal_item *it;

      if(temporary)
	{
	  temporary_generation save(res.temp());
#ifndef CRASHES_ON_DYNAMIC_CASTS
	  it = new phrasal_item(dynamic_cast<phrasal_item *>(active), passive, res);
#else
	  it = new phrasal_item((phrasal_item *)(active), passive, res);
#endif
	  Chart->own(it);
	  FSAS.release();
	}
      else
	{
#ifndef CRASHES_ON_DYNAMIC_CASTS
	  it = new phrasal_item(dynamic_cast<phrasal_item *>(active), passive, res);
#else
	  it = new phrasal_item((phrasal_item *)(active), passive, res);
#endif
	  Chart->own(it);
	}

      return it;
    }
}

item *item_task::execute()
{
#ifdef PACKING
  if(_item->frozen())
    return 0;
#endif
  return _item;
}

item *rule_and_passive_task::execute()
{
#ifdef PACKING
  if(_passive->frozen())
    return 0;
#endif
  return build_rule_item(_R, _passive);
}

item *active_and_passive_task::execute()
{
#ifdef PACKING
  if(_passive->frozen() || _active->frozen())
    return 0;
#endif
  return build_combined_item(_active, _passive);
}

void basic_task::print(FILE *f)
{
  fprintf(f, "task #%d (%d [%d %d %d %d])", _id, _p, _q, _r, _s, _t);
}

void rule_and_passive_task::print(FILE *f)
{
  fprintf(f,
          "task #%d {%s + %d} (%d [%d %d %d %d])",
          _id,
          _R->name(), _passive->id(),
          _p, _q, _r, _s, _t);
}

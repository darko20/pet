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

/* tasks */

#include "pet-system.h"
#include "task.h"
#include "parse.h"
#include "chart.h"
#include "agenda.h"
#include "options.h"
#include "tsdb++.h"

// #define HYPERACTIVE_EXKURS

int basic_task::next_id = 0;

item *build_combined_item(chart *C, item *active, item *passive);

item *build_rule_item(chart *C, agenda *A, grammar_rule *R, item *passive)
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
	  it = New phrasal_item(R, passive, res);
#ifdef HYPERACTIVE_EXKURS
	  // try to find a passive edge to complete this right away
          //
          // in best-first mode, it is more important to stick to the ordering
          // of tasks, as governed by the scoring mechanism. (9-apr-01  -  oe)
          //
          if(opt_nsolutions == 0
#ifdef YY
             && (opt_nth_meaning == 0)
#endif
             ) {
            bool success = false;
            for(chart_iter_adj_passive iter(C, it); iter.valid(); iter++)
              {
		if(!iter.current()->compatible(it, C->rightmost()))
		  continue;
                it->set_done(iter.current()->stamp());
                if(filter_combine_task(it, iter.current()))
                  {
                    item *it2 = build_combined_item(C, it, iter.current());
                    if(it2)
                      {
                        A->push(New item_task(C, A, it2));
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
	  it = New phrasal_item(R, passive, res);
	}

      return it;
    }
}

item *build_combined_item(chart *C, item *active, item *passive)
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
	  it = New phrasal_item(dynamic_cast<phrasal_item *>(active), passive, res);
	  FSAS.release();
	}
      else
	{
	  it = New phrasal_item(dynamic_cast<phrasal_item *>(active), passive, res);
	}

      return it;
    }
}

item *item_task::execute()
{
    // _fixme_ this will go anyway, and may be wrong
  if(opt_packing && _item->blocked())
    return 0;

  return _item;
}

item *rule_and_passive_task::execute()
{
  if(opt_packing && _passive->frozen())
    return 0;

  return build_rule_item(_C, _A, _R, _passive);
}

item *active_and_passive_task::execute()
{
  if(opt_packing && (_passive->frozen() || _active->frozen()))
    return 0;

  return build_combined_item(_C, _active, _passive);
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
          _R->printname(), _passive->id(),
          _p, _q, _r, _s, _t);
}

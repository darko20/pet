/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* i/o operations for simple typed dags */

#include "pet-system.h"
#include "dumper.h"
#include "dag.h"
#include "types.h"

//
// readable output of dag in OSF format
//

void dag_mark_coreferences(struct dag_node *dag)
// recursively set `visit' field of dag nodes to number of occurences
// in this dag - any nodes with more than one occurence are `coreferenced'
{
  dag = dag_deref(dag);

  if(dag_set_visit(dag, dag_get_visit(dag) + 1) == 1)
    { // not yet visited
      struct dag_arc *arc;

      arc = dag->arcs;
      while(arc)
	{
	  dag_mark_coreferences(arc->val);
	  arc = arc->next;
	}
    }
}

int coref_nr;

void dag_print_rec(FILE *f, struct dag_node *dag, int indent)
// recursively print dag. requires `visit' field to be set up by
// mark_coreferences. negative value in `visit' field means that node
// is coreferenced, and already printed
{
  int coref;
  struct dag_arc *arc;

  dag = dag_deref(dag);

  coref = dag_get_visit(dag) - 1;
  if(coref < 0) // dag is coreferenced, already printed
    {
      fprintf(f, "#%d", -(coref+1));
      return;
    }
  else if(coref > 0) // dag is coreferenced, not printed yet
    {
      coref = -dag_set_visit(dag, -(coref_nr++));
      
      indent += fprintf(f, "#%d:", coref);
    }

  fprintf(f, "%s (%x)", typenames[dag->type], (int) dag);
 
  if((arc = dag->arcs) != 0)
    {
      struct dag_node **print_attrs = (struct dag_node **)
	malloc(sizeof(struct dag_node *) * nattrs);

      int maxlen = 0, i, maxatt = 0;

      for(i = 0; i < nattrs; i++)
	print_attrs[i] = 0;

      while(arc)
	{
          i = attrnamelen[arc->attr];
	  maxlen = maxlen > i ? maxlen : i;
	  print_attrs[arc->attr]=arc->val;
	  maxatt = arc->attr > maxatt ? arc->attr : maxatt;
	  arc=arc->next;
	}

      fprintf(f, "\n%*s[ ", indent, "");
      
      bool first = true;
      for(int j = 0; j <= maxatt; j++) if(print_attrs[j])
	{
          i = attrnamelen[j];
	  if(!first)
	    fprintf(f, ",\n%*s",indent + 2,"");
	  else
	    first = false;

	  fprintf(f, "%s%*s", attrname[j], maxlen-i+1, "");
	  dag_print_rec(f, print_attrs[j], indent + 2 + maxlen + 1);
	}

      fprintf(f, " ]");
      free(print_attrs);
    }
}

void dag_print(FILE *f, struct dag_node *dag)
{
  if(dag == 0)
    {
      fprintf(f, "%s", typenames[0]);
      return;
    }
  if(dag == FAIL)
    {
      fprintf(f, "fail");
      return;
    }
  
  dag_mark_coreferences(dag);
  
  coref_nr = 1;
  dag_print_rec(f, dag, 0);
  dag_invalidate_visited();
}

//
// compact binary form output (dumping)
//

int dag_dump_grand_total_nodes = 0;
int dag_dump_grand_total_atomic = 0;
int dag_dump_grand_total_arcs = 0;

int dag_dump_total_nodes, dag_dump_total_arcs;

void dag_mark_dump_nodes(struct dag_node *dag)
{
  dag = dag_deref(dag);

  if(dag_get_visit(dag) == 0)
    {
      struct dag_arc *arc;
      dag_set_visit(dag, -1);

      if(!dag->arcs)
        dag_dump_grand_total_atomic++;

      arc = dag->arcs;
      while(arc)
	{
	  dag_dump_total_arcs++;
	  dag_mark_dump_nodes(arc->val);
	  arc = arc->next;
	}

      dag_set_visit(dag, dag_dump_total_nodes++);
    }
}

#ifdef FLOP
extern int *leaftype_order;
#endif

int dump_index = 0;

void dag_dump_rec(dumper *f, struct dag_node *dag)
{
  int visit;

  static struct dag_node_dump dump_n;
  static struct dag_arc_dump dump_a;

  dag = dag_deref(dag);

  visit = dag_get_visit(dag);

  if(visit > 0) // first time
    {
      struct dag_arc *arc;
      int nr;
      int type;

      dag_set_visit(dag, -visit);

      arc = dag->arcs;
      while(arc)
	{
	  dag_dump_rec(f, arc->val);
	  arc = arc->next;
	}

      arc = dag->arcs; nr = 0;
      while(arc) nr++, arc = arc->next;

      type = dag->type;

#ifdef FLOP
      type = leaftype_order[type];
#endif

      dump_n.type = type;

      dump_n.nattrs = (short int) nr;
      dump_node(f, &dump_n);
      dump_index++;

      arc = dag->arcs;
      while(arc)
	{
	  dump_a.attr = (short int) arc->attr;
	  dump_a.val = (short int) (abs(dag_get_visit(dag_deref(arc->val))) - 1);
	  assert(dump_a.val < dump_index);
	  dump_arc(f, &dump_a);
	  arc = arc->next;
	}
    }
}

bool dag_dump(dumper *f, struct dag_node *dag)
{
  if(dag == 0 || dag == FAIL)
    {
      return false;
    }
  
  dump_index = 0;

  dag_dump_total_nodes = 1; dag_dump_total_arcs = 0;
  dag_mark_dump_nodes(dag); 
  dag_dump_total_nodes--;
  
  f->dump_int(dag_dump_total_nodes);
  f->dump_int(dag_dump_total_arcs);

  dag_dump_rec(f, dag);
  dag_invalidate_visited();

  dag_dump_grand_total_nodes += dag_dump_total_nodes;
  dag_dump_grand_total_arcs += dag_dump_total_arcs;

  return true;
}

struct dag_node *dag_undump(dumper *f)
{
  struct dag_node *undumped_nodes;
  struct dag_arc *undumped_arcs = NULL;

  struct dag_node_dump dump_n;
  struct dag_arc_dump dump_a;

  dag_dump_total_nodes = f->undump_int();
  dag_dump_total_arcs = f->undump_int();

  if((undumped_nodes = (struct dag_node *) dag_alloc_p_nodes(dag_dump_total_nodes)) == 0)
    return FAIL;

  if(dag_dump_total_arcs > 0)
    if((undumped_arcs = (struct dag_arc *) dag_alloc_p_arcs(dag_dump_total_arcs)) == 0)
      return FAIL;

  int current_arc = 0;
  
  for(int i = 0; i < dag_dump_total_nodes; i++)
    {
      undump_node(f, &dump_n);

      if(dump_n.type < 0)
	dump_n.type = -dump_n.type; // node is not expanded

      dag_init(undumped_nodes+i, dump_n.type);

      if(dump_n.nattrs > 0)
	undumped_nodes[i].arcs = undumped_arcs+current_arc;

      for(int j = 0; j < dump_n.nattrs; j++)
	{
	  undump_arc(f, &dump_a);
	  undumped_arcs[current_arc].attr = dump_a.attr;
	  undumped_arcs[current_arc].val = undumped_nodes + dump_a.val;
	  undumped_arcs[current_arc].next =
	    (j == dump_n.nattrs - 1) ? 0 : undumped_arcs+current_arc+1;

	  current_arc++;
	}
    }

  return undumped_nodes + dag_dump_total_nodes - 1;
}

//
// The entire contents of this file, in printed or electronic form, are
// (c) Copyright YY Software Corporation, La Honda, CA 2000, 2001.
// Unpublished work -- all rights reserved -- proprietary, trade secret
//

// K2Y semantics representation (dan, uc, oe)

#include "k2y.h"
#include "types.h"
#include "item.h"
#include "tsdb++.h"

// #define JAPANESE

// macro to ease conversion from L E D A to STL - can be used like the
// forall macro from L E D A, but only on list<int> containers

#define forallint(V,L) V=L.front();\
 for(list<int>::iterator _iterator = L.begin(); _iterator != L.end();\
 ++_iterator,V=*_iterator)

//
// kludge to capture printed output in string, so we can return it to the fine
// [incr tsdb()] system.  this file may need some reorganization and cleaning
// sometime soon :-).                                  (27-mar-00  -  oe)
//

static struct MFILE *mstream;

//
// for the moment we just print the k2y representation, rather than
// really constructing it
//

// if this is true, don't really construct semantics, just count objects
bool evaluate;
static int nrelations;
static set<int> raw_atoms;

char *k2y_name[K2Y_XXX] = { "sentence", "mainverb",
                            "subject", "dobject", "iobject",
                            "vcomp", "modifier", "intarg",
                            "quantifier", "conj", "nncmpnd", "particle",
                            "subord" };

#define MAXIMUM_NUMBER_OF_RELS 222

void new_k2y_object(mrs_rel &r, k2y_role role, int clause, 
                    fs index = 0, bool closep = false)
{
  fs f;
  nrelations++;

  if(nrelations > MAXIMUM_NUMBER_OF_RELS) {
    char foo[128];
    sprintf(foo, "apparently circular K2Y (%d relation(s))", nrelations);
    throw error(foo);
  } /* if */

  if(!r.label().empty()) {
    for(list<int>::iterator i = r.label().begin(); 
        i != r.label().end();
        i++) {
      raw_atoms.insert(*i);
    } /* for */
  } /* if */

  if(evaluate) return;

  mprintf(mstream, "  %s[", k2y_name[role]);
  mprintf(mstream, "ID x%d", r.id());
  if(r.cvalue() >= 0) {
    mprintf(mstream, ", REL const_rel, CVALUE %d", r.cvalue());
  } /* if */
  else {
    mprintf(mstream, ", REL %s", r.name());
  } /* else */
  mprintf(mstream, ", CLAUSE x%d", clause);
  if(!r.label().empty()) {
    mprintf(mstream, ", RA < ");
    for(list<int>::iterator i = r.label().begin(); 
        i != r.label().end();
        i++) {
      mprintf(mstream, "%d ", *i * 100);
    } /* for */
    mprintf(mstream, ">");
  } /* if */

  if(role == K2Y_MAINVERB)
  {
    if(index != 0 && index.valid()) {
      f = index.get_path_value("E.TENSE");
      if(f.valid()) mprintf(mstream, ", TENSE %s", f.name());
      f = index.get_path_value("E.ASPECT");
      if(f.valid()) mprintf(mstream, ", ASPECT %s", f.name());
      f = index.get_path_value("E.MOOD");
      if(f.valid()) mprintf(mstream, ", MOOD %s", f.name());
    } /* if */

  }

  if(role == K2Y_SUBJECT || role == K2Y_DOBJECT || role == K2Y_IOBJECT)
  {
    f = r.get_fs().get_path_value("INST.PNG.PN");
    if(f.valid()) mprintf(mstream, ", PN %s", f.name());

    f = r.get_fs().get_path_value("INST.PNG.GEN");
    if(f.valid()) mprintf(mstream, ", GENDER %s", f.name());
  }
  if(closep) mprintf(mstream, "]\n");
}

void new_k2y_modifier(mrs_rel &r, k2y_role role, int clause, int arg)
{
  new_k2y_object(r, role, clause);
  if(evaluate) return;
  if(arg != 0) mprintf(mstream, ", ARG x%d", arg);
  mprintf(mstream, "]\n");
}

void new_k2y_intarg(mrs_rel &r, k2y_role role, int clause, mrs_rel argof)
{
  new_k2y_object(r, role, clause);
  if(evaluate) return;
  if(argof.valid()) 
    {
      if(subtype(argof.type(), lookup_type(k2y_pred_name("k2y_prep_rel"))) ||
         subtype(argof.type(), lookup_type(k2y_pred_name("k2y_nn_rel"))))
        mprintf(mstream, ", ARGOF x%d", argof.id());
      else
        mprintf(mstream, ", OBJOF x%d", argof.id());
    }
  mprintf(mstream, "]\n");
}

void new_k2y_quantifier(mrs_rel &r, k2y_role role, int clause, int var)
{
  new_k2y_object(r, role, clause);
  if(evaluate) return;
  if(var != 0) mprintf(mstream, ", VAR x%d", var);
  mprintf(mstream, "]\n");
}

void new_k2y_conj(mrs_rel &r, k2y_role role, int clause, list<int> conjs)
{
  new_k2y_object(r, role, clause);
  if(evaluate) return;
  if(!conjs.empty())
  {
    mprintf(mstream, ", CONJUNCTS < ");
    int conj;
    forallint(conj, conjs)
    {
      mprintf(mstream, "x%d ", conj);
    }
    mprintf(mstream, ">");
  }
  if(r.get_fs().valid()) {
    fs index = r.get_fs().get_path_value("C-ARG");
    if(index.valid()) {
      fs f;
      f = index.get_path_value("E.TENSE");
      if(f.valid()) mprintf(mstream, ", TENSE %s", f.name());
      f = index.get_path_value("E.ASPECT");
      if(f.valid()) mprintf(mstream, ", ASPECT %s", f.name());
      f = index.get_path_value("E.MOOD");
      if(f.valid()) mprintf(mstream, ", MOOD %s", f.name());
    } /* if */
  } /* if */

  mprintf(mstream, "]\n");
}

//
// mrs -> k2y
//

int k2y_clause(mrs &m, int clause, int id, int mv_id = 0);
int k2y_message(mrs &m, int id);
list<int> k2y_conjunction(mrs &m, int clause, int conj_id);
list<int> k2y_conjuncts(mrs &m, int clause, char *path, list<int> conjs, bool mvsearch = false);

void k2y_verb(mrs &m, int clause, int id);
bool k2y_nom(mrs &m, int clause, k2y_role role, int id, int argof = -1);

// look for a verb_particle of .id (verb's handle).
void k2y_particle(mrs &m, int clause, int id, int governor = -1)
{

  list<int> particles = m.rels(k2y_role_name("k2y_hndl"), id, 
                               lookup_type(k2y_pred_name("k2y_select_rel")));

  int i;
  forallint(i, particles) {
    mrs_rel particle = m.rel(i);
    if(particle.valid()) {
      new_k2y_object(particle, K2Y_PARTICLE, clause, 0, false);
      if(!evaluate) {
        if(governor >= 0) mprintf(mstream, ", ARGOF x%d]\n", governor);
        else mprintf(mstream, "]\n");
      } /* if */
      mrs_rel nom = m.rel(k2y_role_name("k2y_inst"), 
                              particle.value(k2y_role_name("k2y_arg3")));
      if(nom.valid())
        new_k2y_intarg(nom, K2Y_INTARG, clause, m.rel(particle.id()));
    } /* if */
    
  } /* forallint */

}


// look for a vcomp of .id.
void k2y_vcomp(mrs &m, int clause, int id)
{
  mrs_rel vcomp;
  vcomp = m.rel(k2y_role_name("k2y_hndl"), id, 
                lookup_type(k2y_type_name("k2y_message")));
  if(vcomp.valid())
    {
      int message_id = k2y_message(m, id);
      mrs_rel message = m.rel(message_id);
      new_k2y_object(message, K2Y_VCOMP, clause, 0, true);
    }
  else
    {
    vcomp = m.rel(k2y_role_name("k2y_hndl"), m.hcons(id), 
                  lookup_type(k2y_pred_name("k2y_verb_rel")));
    if(!vcomp.valid())
      vcomp = m.rel(k2y_role_name("k2y_hndl"), m.hcons(id), 
                    lookup_type(k2y_pred_name("k2y_adv_rel")));
    if(!vcomp.valid())
      vcomp = m.rel(k2y_role_name("k2y_hndl"), m.hcons(id), 
                    lookup_type(k2y_pred_name("k2y_excl_rel")));
    if(!vcomp.valid())
      vcomp = m.rel(k2y_role_name("k2y_hndl"), m.hcons(id), 
                    lookup_type(k2y_pred_name("k2y_event_rel")));
    if(vcomp.valid())
      {
      int message_id = k2y_message(m, vcomp.value(k2y_role_name("k2y_hndl")));
      mrs_rel message = m.rel(message_id);
      new_k2y_object(message, K2Y_VCOMP, clause, 0, true);
      }
    }
}

// look for modifiers of .id.
void k2y_mod(mrs &m, int clause, int id, int arg)
{
  list<int> l = m.rels(k2y_role_name("k2y_arg"), id);
  mrs_rel argrel = m.rel(arg);
  int i;
  forallint(i, l)
    {
      mrs_rel mod = m.rel(i);
      fs afs;
      if(!m.used(mod.id()) &&
         !subtype(mod.type(), lookup_type(k2y_pred_name("k2y_select_rel"))) &&
         (!subtype(mod.type(), lookup_type(k2y_pred_name("k2y_event_rel"))) ||
          ((afs = mod.get_fs().get_path_value("EVENT.E.TENSE")).valid()
           && subtype(afs.type(), lookup_type(k2y_type_name("k2y_no_tense"))))))
        {
          fs afs;

          if(!mod.value(k2y_role_name("k2y_arg3")) || 
             ((afs = mod.get_fs().get_path_value(k2y_role_name("k2y_arg3"))).valid()
              && !subtype(afs.type(), 
                          lookup_type(k2y_type_name("k2y_nonexpl_ind")))) ||
             subtype(argrel.type(), lookup_type(k2y_pred_name("k2y_verb_rel"))))
            {
              new_k2y_modifier(mod, K2Y_MODIFIER, clause, arg);
              k2y_nom(m, clause, K2Y_INTARG, mod.value(k2y_role_name("k2y_arg3")), mod.id());
              k2y_vcomp(m, clause, mod.value(k2y_role_name("k2y_arg4")));
              m.use_rel(mod.id());
            }
        }
    }
}

// relative clauses (a special kind of modifier)
void k2y_rc(mrs &m, int clause, int id, int arg)
{
  mrs_rel prop = m.rel(k2y_role_name("k2y_hndl"), id, 
                       lookup_type(k2y_pred_name("k2y_prpstn_rel")));
  if(prop.valid() && !m.used(prop.id()))
  {
    m.use_rel(prop.id());
    new_k2y_modifier(prop, K2Y_MODIFIER, clause, arg);
    k2y_clause(m, prop.id(), prop.value(k2y_role_name("k2y_soa")));
  }
}

// reduced relative clauses (a special kind of modifier)
void k2y_rrc(mrs &m, int clause, int id, int arg, int hid)
{
  list<int> redprops = m.rels(k2y_role_name("k2y_hndl"), id, 
                              lookup_type(k2y_pred_name("k2y_verb_rel")));
  int i;
  mrs_rel redprop;
  list<int> redpropcands = m.rels(k2y_role_name("k2y_hndl"), id, 
                                  lookup_type(k2y_pred_name("k2y_arg_rel")));
  forallint(i,redpropcands)
    {
      redprop = m.rel(i);
      fs afs;
      if(subtype(redprop.type(), 
                 lookup_type(k2y_pred_name("k2y_nonevent_rel"))) ||
         ((afs = redprop.get_fs().get_path_value(k2y_role_name("k2y_arg3"))).valid()
          && subtype(afs.type(), lookup_type(k2y_type_name("k2y_nonexpl_ind")))
          && !subtype(redprop.type(), 
                      lookup_type(k2y_pred_name("k2y_proposs_rel")))))
        redprops.push_front(i);
    }
  forallint(i, redprops)
   {
      redprop = m.rel(i);
      fs afs;
      if(redprop.valid() && !m.used(redprop.id()) &&
         redprop.id() != hid &&
         (subtype(redprop.type(), 
                  lookup_type(k2y_pred_name("k2y_nonevent_rel"))) ||
          ((afs = redprop.get_fs().get_path_value("EVENT.E.TENSE")).valid()
           && subtype(afs.type(), lookup_type(k2y_type_name("k2y_no_tense")))
           && !subtype(redprop.type(), 
                       lookup_type(k2y_pred_name("k2y_select_rel")))
           && (!(afs = redprop.get_fs().get_path_value("ARG")).valid()
               || !subtype(afs.type(), lookup_type(k2y_type_name("k2y_event")))))))
        {
          // construct a pseudo proposition
          mrs_rel pseudo = mrs_rel(&m, lookup_type(k2y_pred_name("k2y_prpstn_rel")));
          m.push_rel(pseudo);
          new_k2y_modifier(pseudo, K2Y_MODIFIER, clause, arg);
          m.use_rel(redprop.id());
          k2y_clause(m, pseudo.id(), 0, redprop.id());
        }
    }
}

// look for a quantifier for .id.
void k2y_quantifier(mrs &m, int clause, int id, int var)
{
  mrs_rel quant = m.rel(k2y_role_name("k2y_bv"), id);
  if(quant.valid()) new_k2y_quantifier(quant, K2Y_QUANTIFIER, clause, var);
}

// noun noun compounds(a special kind of modifier)
void k2y_nn(mrs &m, int clause, int id, int arg)
{
  list<int> nnrels = m.rels(k2y_role_name("k2y_nn_head"), id, 
                            lookup_type(k2y_pred_name("k2y_nn_rel")));
  int i;
  forallint(i, nnrels)
    {
      mrs_rel nnrel = m.rel(i);
      if(nnrel.valid())
        {
          new_k2y_modifier(nnrel, K2Y_NNCMPND, clause, arg);
          mrs_rel nom = m.rel(k2y_role_name("k2y_inst"),
                              nnrel.value(k2y_role_name("k2y_nn_nonhead")));
          if(nom.valid())
            {
              new_k2y_intarg(nom, K2Y_INTARG, clause, nnrel);
              k2y_mod(m, clause, nom.value(k2y_role_name("k2y_inst")), nom.id());
              k2y_quantifier(m, clause, nom.value(k2y_role_name("k2y_inst")), 
                             nom.id());
              k2y_nn(m, clause, nom.value(k2y_role_name("k2y_inst")), nom.id());
            }
        }
    }
}

// look for subject, dobject or iobject, respectively
bool k2y_nom(mrs &m, int clause, k2y_role role, int id, int argof)
{
  mrs_rel nom = m.rel(k2y_role_name("k2y_inst"), id);
  if(m.used(id, clause))
  {
    return true;
  }
  else if(nom.valid() &&
          !subtype(nom.type(), 
                   lookup_type(k2y_pred_name("k2y_nomger_rel"))))
    {
      m.use_rel(id, clause);
      if(argof >= 0)
        new_k2y_intarg(nom, role, clause, m.rel(argof));
      else
        new_k2y_object(nom, role, clause, 0, true);
      if(m.used(id, 1)) return true;
      m.use_rel(id, 1);
      k2y_mod(m, clause, nom.value(k2y_role_name("k2y_inst")), nom.id());
      if(subtype(nom.type(), lookup_type(k2y_pred_name("k2y_nom_rel"))))
        {
          k2y_nn(m, clause, nom.value(k2y_role_name("k2y_inst")), nom.id());
          k2y_rc(m, clause, nom.value(k2y_role_name("k2y_hndl")), nom.id());
          k2y_rrc(m, clause, nom.value(k2y_role_name("k2y_hndl")), nom.id(), 
                  id);
          k2y_quantifier(m, clause, nom.value(k2y_role_name("k2y_inst")), 
                         nom.id());
          k2y_vcomp(m, clause, nom.value(k2y_role_name("k2y_arg4")));
        }
      return true;
    }
  else if(nom.valid())
    {
      m.use_rel(id, clause);
      mrs_rel mv = m.rel(k2y_role_name("k2y_hndl"), 
                         nom.value(k2y_role_name("k2y_hndl")), 
                         lookup_type(k2y_pred_name("k2y_verb_rel")));
      mrs_rel pseudo = mrs_rel(&m, lookup_type(k2y_pred_name("k2y_hypo_rel")));
      m.push_rel(pseudo);
      if(argof >= 0)
        new_k2y_intarg(nom, role, clause, m.rel(argof));
      else
        new_k2y_object(nom, role, clause, 0, true);
      new_k2y_intarg(pseudo, K2Y_VCOMP, clause, nom);
      k2y_clause(m,pseudo.id(),nom.value(k2y_role_name("k2y_hndl")));
      return pseudo.id();
    }
  else
    {
      mrs_rel r = m.rel(k2y_role_name("k2y_c_arg"), id);
      if(r.valid())
	{
	  list<int> conjs = k2y_conjunction(m, clause, r.id());
	  int i;
	  forallint(i, conjs)
	    {
	      mrs_rel conj = m.rel(i);
	      k2y_nom(m, clause, role, conj.value(k2y_role_name("k2y_inst")), 
                      argof);
	    }
	  return true;
	}
      else
	return false;
    }
}

void k2y_verb(mrs &m, int clause, int id)
{
  mrs_rel verb = m.rel(id);

  if(verb.valid())
  {
    k2y_mod(m, clause, verb.value(k2y_role_name("k2y_event")), id);
    int subjid;
    if(subtype(verb.type(), lookup_type(k2y_pred_name("k2y_arg3_rel"))))
      subjid = verb.value(k2y_role_name("k2y_arg3"));
    else
      {
	if(subtype(verb.type(), lookup_type(k2y_pred_name("k2y_arg1_rel"))))
          subjid = verb.value(k2y_role_name("k2y_arg1"));
        else
          subjid = verb.value(k2y_role_name("k2y_arg"));
      }
    
    mrs_rel subjrel = m.rel(k2y_role_name("k2y_inst"), subjid);
    if(subjrel.valid())
      k2y_rrc(m, clause, verb.value(k2y_role_name("k2y_hndl")), 
              subjrel.id(), id);

    k2y_nom(m, clause, K2Y_SUBJECT, subjid);
    k2y_nom(m, clause, K2Y_DOBJECT, verb.value(k2y_role_name("k2y_arg3")));
    k2y_nom(m, clause, K2Y_IOBJECT, verb.value(k2y_role_name("k2y_arg2")));
    k2y_particle(m, clause, verb.value(k2y_role_name("k2y_hndl")), verb.id());
    k2y_vcomp(m, clause, verb.value(k2y_role_name("k2y_arg4")));
  }
}

int k2y_clause_conjunction(mrs &m, int clause, int id, int mv_id)
{
   mrs_rel conj = mrs_rel();
   list<int> ids;

   if(mv_id == 0)
     conj = m.rel(k2y_role_name("k2y_hndl"), m.hcons(id), 
                  lookup_type(k2y_pred_name("k2y_conj_rel")));

   if(conj.valid() && subtype(conj.type(), lookup_type(k2y_pred_name("k2y_conj_rel"))))
     {
       char *indices[] = { "L-INDEX", "R-INDEX", NULL };
       list<int> conjs = conj.id_list_by_paths(indices);

      if(!conjs.empty())
        ids = k2y_conjuncts(m, clause, k2y_role_name("k2y_inst"), conjs);
     }
   if(conj.valid() && subtype(conj.type(), lookup_type(k2y_pred_name("k2y_conj_rel"))) && 
      ids.empty())
   {
     list<int> conjs = k2y_conjunction(m, clause, conj.id());
     fs foo;
     if((foo = conj.get_fs().get_path_value("R-HANDEL")).valid()
        && subtype(foo.type(), lookup_type(k2y_type_name("k2y_handle"))))
       { 
	 int i;
         forallint(i, conjs)
           {
             mrs_rel conj = m.rel(i);
             k2y_clause(m, clause, conj.value(k2y_role_name("k2y_hndl")), conj.id());
           }
         return conj.id();
       }
     else
       return dummy_id;
   }
   else
     return 0;
} 

int k2y_clause_subord(mrs &m, int clause, int id, int mv_id)
{
  mrs_rel subord = mrs_rel();
  if(mv_id == 0)
    subord = m.rel(k2y_role_name("k2y_hndl"), m.hcons(id), 
                   lookup_type(k2y_pred_name("k2y_subord_rel")));
  if(subord.valid())
    {
      char *indices[] = { "MAIN", "SUBORD", NULL };
      list<int> subs = subord.id_list_by_paths(indices);
      if(!subs.empty())
        {
          int i;
          list<int> subids;
          list<int> ids;
          forallint(i, subs)
            {
              mrs_rel sub = m.rel(k2y_role_name("k2y_hndl"), m.hcons(i), 
                                  lookup_type(k2y_type_name("k2y_message")));
              if(sub.valid())
                {
                  subids.push_front(m.hcons(i));
                  int message_id = k2y_message(m, sub.value(k2y_role_name("k2y_hndl")));
                  mrs_rel message = m.rel(message_id);
                  new_k2y_object(message, K2Y_VCOMP, subord.id(), 0, true);
                }
            }
          ids = k2y_conjuncts(m, clause, k2y_role_name("k2y_hndl"), subids);
          new_k2y_conj(subord, K2Y_SUBORD, clause, ids);
          return subord.id();
        }
    }
  return 0;
}

int k2y_clause_exclam(mrs &m, int clause, int id, int mv_id)
{
  mrs_rel exclrel = mrs_rel();
  if(mv_id == 0)
    exclrel = m.rel(k2y_role_name("k2y_hndl"), id, 
                    lookup_type(k2y_pred_name("k2y_excl_rel")));
  if(exclrel.valid())
    {
      new_k2y_object(exclrel, K2Y_MAINVERB, clause, 0, true);
      return exclrel.id();
    }
  return 0;
}

int k2y_clause(mrs &m, int clause, int id, int mv_id)
{
  if(!k2y_clause_conjunction(m, clause, id, mv_id))
   if(!k2y_clause_subord(m, clause, id, mv_id))
    if(!k2y_clause_exclam(m, clause, id, mv_id))
     {
       mrs_rel mv;
       if(mv_id)
         // Main verb ID already provided
         mv = m.rel(mv_id);
       else 
         {
           // Find the main verb, ignoring any VP modifiers of that verb
           list<int> mvs = m.rels(k2y_role_name("k2y_hndl"), m.hcons(id), 
                                  lookup_type(k2y_pred_name("k2y_verb_rel")));
           // If there are VP modifiers of VP, find the (single) tensed one
           if(mvs.size() > 1)
             {
               int i;
               forallint(i, mvs)                 
                 {
                   mrs_rel mvcand;
                   mvcand = m.rel(i);
                   fs afs;
                   if((afs = mvcand.get_fs().get_path_value("EVENT.E.TENSE")).valid() &&
                      !subtype(afs.type(), lookup_type(k2y_type_name("k2y_no_tense")))
                      && mvcand.valid())
                     mv = mvcand;
                 }
             }
           else if(!mvs.empty()) 
             // There is a unique main verb
             mv = m.rel(mvs.front());
           }
       if(mv.valid() 
          && (!mv_id || subtype(mv.type(), 
                                lookup_type(k2y_pred_name("k2y_verb_rel")))))
         {
           fs index;
           index = mv.get_fs().get_path_value(k2y_role_name("k2y_event"));
           if(subtype(mv.type(), lookup_type(k2y_pred_name("k2y_appos_rel"))))
             {
               mrs_rel newmv = mrs_rel(&m, lookup_type(k2y_pred_name("k2y_cop_rel")));
               m.push_rel(newmv);
               m.use_rel(newmv.id());
               new_k2y_object(newmv, K2Y_MAINVERB, clause, index, true);
             }
           else
             new_k2y_object(mv, K2Y_MAINVERB, clause, index, true);
           k2y_verb(m, clause, mv.id());
           return mv.id();
         }
       else
         {
           if(!mv.valid())
             mv = m.rel(k2y_role_name("k2y_hndl"), m.hcons(id), 
                        lookup_type(k2y_pred_name("k2y_adj_rel")));
           if(!mv.valid() || (!mv_id && m.used(mv.id())))
             {
               list<int> pprels = m.rels(k2y_role_name("k2y_hndl"), m.hcons(id), 
                                         lookup_type(k2y_pred_name("k2y_prep_rel")));
               int i;
               fs afs;
               forallint(i,pprels)
                 { 
                   mrs_rel pprel = m.rel(i);
                   if((afs = 
                       pprel.get_fs().get_path_value(k2y_role_name("k2y_arg"))).valid()
                      && !subtype(afs.type(), lookup_type(k2y_type_name("k2y_event")))
                      && !subtype(pprel.type(), 
                                  lookup_type(k2y_pred_name("k2y_select_rel"))))
                     mv = pprel;
                 }
             }
           if(mv.valid() && subtype(mv.type(), 
                                    lookup_type(k2y_pred_name("k2y_event_rel"))) &&
              !subtype(mv.type(), lookup_type(k2y_pred_name("k2y_select_rel"))) &&
              (mv_id || !m.used(mv.id())))
             {
               fs index;
               mrs_rel newmv = mv;
               if(!subtype(mv.type(), lookup_type(k2y_pred_name("k2y_adj_rel"))))
                 {
                   newmv = mrs_rel(&m, lookup_type(k2y_pred_name("k2y_cop_rel")));
                   m.push_rel(newmv);
                 }
               index = mv.get_fs().get_path_value(k2y_role_name("k2y_event"));
               new_k2y_object(newmv, K2Y_MAINVERB, clause, index, true);
               m.use_rel(newmv.id());
               mrs_rel argrel = m.rel(k2y_role_name("k2y_inst"), 
                                      mv.value(k2y_role_name("k2y_arg")));
               if(subtype(mv.type(), lookup_type(k2y_pred_name("k2y_prep_rel"))))
                 {
                   if(!subtype(mv.type(), lookup_type(k2y_pred_name("k2y_select_rel"))))
                     {
                       new_k2y_modifier(mv, K2Y_MODIFIER, clause, newmv.id());
                       k2y_nom(m, clause, K2Y_INTARG, 
                               mv.value(k2y_role_name("k2y_arg3")), mv.id());
                     }
                 }
               else 
                 {
                   if(!subtype(mv.type(), 
                               lookup_type(k2y_pred_name("k2y_adj_rel"))))
                     {
                       new_k2y_modifier(mv, K2Y_MODIFIER, clause, argrel.id());
                       k2y_nom(m, clause, K2Y_INTARG, 
                               mv.value(k2y_role_name("k2y_arg3")), mv.id());
                     }
                   else 
                     {
                       k2y_nom(m, clause, K2Y_DOBJECT, 
                               mv.value(k2y_role_name("k2y_arg3")), mv.id());
                       k2y_particle(m, clause, 
                                mv.value(k2y_role_name("k2y_hndl")), mv.id());
                     }
                 }
               m.use_rel(mv.id());
               k2y_nom(m, clause, K2Y_SUBJECT, mv.value(k2y_role_name("k2y_arg")));
               k2y_vcomp(m, clause, mv.value(k2y_role_name("k2y_arg4")));
               k2y_mod(m, clause, mv.value(k2y_role_name("k2y_event")), 
                       newmv.id());
               list<int> pprels = m.rels(k2y_role_name("k2y_hndl"), m.hcons(id), 
                                         lookup_type(k2y_pred_name("k2y_prep_rel")));
               int i;
               forallint(i,pprels)
                 { 
                   mrs_rel pprel = m.rel(i);
                   if(!(pprel.id() == mv.id()) &&
                      !subtype(pprel.type(), 
                               lookup_type(k2y_pred_name("k2y_select_rel"))))
                     {
                       new_k2y_modifier(pprel, K2Y_MODIFIER, clause, newmv.id());
                       k2y_nom(m, clause, K2Y_INTARG, 
                               pprel.value(k2y_role_name("k2y_arg3")), pprel.id());
                       m.use_rel(pprel.id());
                     }
                 }
               return newmv.id();
             }
           else
             {
               if(mv_id)
                 {
                   mrs_rel adv = m.rel(mv_id);
                   if(adv.valid() &&
                      subtype(adv.type(), lookup_type(k2y_pred_name("k2y_adv_rel"))))
                     {
                       int argid = adv.value(k2y_role_name("k2y_arg"));
                       mrs_rel argrel = m.rel(k2y_role_name("k2y_hndl"), 
                                             m.hcons(argid), 
                                             lookup_type(k2y_type_name("k2y_message")));
                       if(argrel.valid())
                         mv_id = k2y_message(m, argid);
                       else
                         mv_id = k2y_clause(m, clause, argid);
                       new_k2y_modifier(adv, K2Y_MODIFIER, clause, mv_id);
                       return adv.id();
                     }
                 }
               else
                 {
                   list<int> adv_cands = m.rels(k2y_role_name("k2y_hndl"), m.hcons(id), 
                                            lookup_type(k2y_pred_name("k2y_adv_rel")));
                   int i;
                   forallint(i, adv_cands)
                     {
                       mrs_rel adv = m.rel(i);
                       fs arg = adv.get_fs().get_attr_value(k2y_role_name("k2y_arg"));
                       if(arg.valid() && 
                          subtype(arg.type(), lookup_type(k2y_type_name("k2y_handle"))))
                         {
                           int argid = m.hcons(adv.value(k2y_role_name("k2y_arg")));
                           mrs_rel argrel = m.rel(k2y_role_name("k2y_hndl"), argid, 
                                             lookup_type(k2y_type_name("k2y_message")));
                           if(argrel.valid())
                             {
                               mv_id = k2y_message(m, argid);
                               mrs_rel message = m.rel(mv_id);
                               new_k2y_object(message, K2Y_VCOMP, clause, 0, true);
                             }
                           else
                             mv_id = k2y_clause(m, clause, argid);
                           new_k2y_modifier(adv, K2Y_MODIFIER, clause, mv_id);
                           return adv.id();
                         }
                     }
                 }
             }
         }
     }  
  return 0;
}

int k2y_message(mrs &m, int id)
{
  mrs_rel rel = m.rel(k2y_role_name("k2y_hndl"), id, 
                      lookup_type(k2y_type_name("k2y_message")));
  if(rel.valid())
    {
      mrs_rel doublemsg = m.rel(k2y_role_name("k2y_hndl"), 
                                m.hcons(rel.value(k2y_role_name("k2y_soa"))),
                                lookup_type(k2y_type_name("k2y_message")));
      if (doublemsg.valid())
        k2y_clause(m, rel.id(), doublemsg.value(k2y_role_name("k2y_soa")));
      else
        k2y_clause(m, rel.id(), rel.value(k2y_role_name("k2y_soa")));
      return rel.id();
    }
  else
  {
     // construct a pseudo id
     mrs_rel pseudo = mrs_rel(&m, lookup_type(k2y_type_name("k2y_message")));
     m.push_rel(pseudo);

     rel = m.rel(k2y_role_name("k2y_hndl"), id, 
                 lookup_type(k2y_pred_name("k2y_conj_rel"))) ;
     if(rel.valid())
     {
       list<int> conjs = k2y_conjunction(m, pseudo.id(), rel.id());
       int i;
       forallint(i, conjs)
       {
         mrs_rel conj = m.rel(i);
         k2y_vcomp(m, pseudo.id(), conj.value(k2y_role_name("k2y_hndl")));
       }
     }
     else
     {
       rel = m.rel(k2y_role_name("k2y_inst"), m.index());
       if(rel.valid())
         k2y_nom(m, pseudo.id(), K2Y_DOBJECT, m.index());
       else
         {
           if(m.rel(id).valid())
             k2y_clause(m, pseudo.id(), 0, id);
           else
             k2y_clause(m, pseudo.id(), id);
         }
     }
     return pseudo.id();
  }
}

list<int> k2y_conjuncts(mrs &m, int clause, char *path, list<int> conjs, bool mvsearch)
{
  list<int> ids;
  int i;
  
  forallint(i, conjs)
  {
    mrs_rel r;
    if(mvsearch)
      r = m.rel(path, i, lookup_type(k2y_pred_name("k2y_verb_rel")));
    if(!mvsearch || !r.valid())
      r = m.rel(path, i);

    if(r.valid()) ids.push_front(r.id());
  }
  return ids;
}

list<int> k2y_conjunction(mrs &m, int clause, int conj_id)
{
  list<int> ids;
  mrs_rel conj = m.rel(conj_id);
  char *paths[] = { "L-HANDEL", "R-HANDEL", NULL };
  list<int> conjs = conj.id_list_by_paths(paths);

  if(conj.valid() && !m.used(conj.id()))
  {
    fs foo;
    mrs_rel bar;
    if((foo = (conj.get_fs().get_path_value("R-HANDEL"))).valid()
       && subtype(foo.type(), lookup_type(k2y_type_name("k2y_handle")))
       && (bar = m.rel(k2y_role_name("k2y_hndl"), conjs.front())).valid()
       && subtype(bar.type(), lookup_type(k2y_type_name("k2y_message"))))
      {
        ids = k2y_conjuncts(m, clause, k2y_role_name("k2y_hndl"), conjs);
        new_k2y_conj(conj, K2Y_CONJ, clause, ids);
        m.use_rel(conj.id());
      }
    else
    {
      if(conjs.empty())
        {
        char *paths[] = { "L-INDEX", "R-INDEX", NULL };
        conjs = conj.id_list_by_paths(paths);
        if(!conjs.empty())
          {
            ids = k2y_conjuncts(m, clause, k2y_role_name("k2y_inst"), conjs);
            if(!ids.empty())
              {
                new_k2y_conj(conj, K2Y_CONJ, clause, ids);
                m.use_rel(conj.id());
              }
          }
        }
      else
        {
           ids = k2y_conjuncts(m, clause, k2y_role_name("k2y_hndl"), conjs, true);
           int i;
           list<int> newconjs;
           forallint(i, ids)
             {
               mrs_rel conjnct = m.rel(i);
               if(conjnct.valid())
                 {
                   int message_id =  k2y_message(m, conjnct.id());
                   mrs_rel message = m.rel(message_id);
                   newconjs.push_front(message.id());
                   new_k2y_object(message, K2Y_VCOMP, clause, 0, true);
                   // DPF 11-Feb-01 - Next line doesn't work, but should for
                   // 'kim is null and void for sandy'
                   //k2y_mod(m, message_id, 
                   //     conj.value(k2y_role_name"k2y_c_arg")), conjnct.id());
                 }
             }
           new_k2y_conj(conj, K2Y_CONJ, clause, newconjs);
           ids.clear();
        }
    }
  }
  else
    fprintf(ferr, "k2yconjunction(): unexpected empty conj %d\n", conj_id);
  return ids;
}

void k2y_sentence(mrs &m)
{
  int message_id = k2y_message(m, m.top());
  mrs_rel message = m.rel(message_id);

  if(message.valid()) 
    new_k2y_object(message, K2Y_SENTENCE, message.id(), 0, true);
}

//
// utility functions
//

//#define DEBUG_MRS

int construct_k2y(int id, item *root, bool eval, struct MFILE *stream)
{

  evaluate = eval;
  if(!eval && stream == NULL) return(-1);
  mstream = (stream != NULL ? mopen() : (MFILE *)NULL);

  try {

    int status = 0;

    mrs m(root->get_fs());

#ifdef DEBUG_MRS
    if(!eval) {
      m.print(ferr);
      fprintf(ferr, "\n");
    } /* if */
#endif

    nrelations = 0; 
    raw_atoms.clear();

    k2y_sentence(m);

    if((int)raw_atoms.size() < root->end() * (float)opt_k2y / 100) {
      if(!eval && verbosity > 1) {
        fprintf(ferr, 
                "construct_k2y(): [%d of %d] "
                "sparse K2Y (%d raw atoms(s)).\n",
                id, stats.readings, raw_atoms.size());
      } /* if */
      if(stream != NULL) {
        mprintf(stream, 
                "([%d--%d] %d of %d: "
                "sparse K2Y (%d raw atoms(s)))",
                root->start(), root->end(), 
                id, stats.readings, raw_atoms.size());
      } /* if */
      status = -1;
    } /* if */
    else {
      if(stream != NULL) {
        mprintf(stream, 
                "{%d--%d; %d of %d; [%d relation%s %d raw atom%s];\n%s}", 
                root->start(), root->end(), 
                id, stats.readings,
                nrelations, (nrelations > 1 ? "s" : ""),
                raw_atoms.size(), (raw_atoms.size() > 1 ? "s" : ""),
                mstring(mstream));
      } /* if */
      status = nrelations;
    } /* else */
    if(mstream != NULL) mclose(mstream);
    mstream = ((MFILE *)NULL);
    return status;

  } /* try */

  catch(error &condition) {
    if(!eval && verbosity > 1) {
      fprintf(ferr, 
              "construct_k2y(): [%d of %d] error: `",
              id, stats.readings);
      condition.print(ferr);
      fprintf(ferr, "'.\n");
    } /* if */
    if(stream != NULL) {
      mprintf(stream, 
              "([%d--%d] %d of %d: %s)",
              root->start(), root->end(), 
              id, stats.readings,
              condition.msg().c_str());
    } /* if */
    if(mstream != NULL) mclose(mstream);
    mstream = ((MFILE *)NULL);
    return -42;
  } /* catch */
}



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

// K2Y semantics representation (dan, uc, oe)

#include "pet-system.h"
#include "k2y.h"
#include "types.h"
#include "item.h"
#include "tsdb++.h"

// Prototypes
void k2y_deg(mrs &m, int clause, int modid, int argid);

// #define JAPANESE

// macro to ease conversion from L E D A to STL - can be used like the
// forall macro from L E D A, but only on list<int> containers

#define forallint(V,L) V=L.front();\
 for(list<int>::iterator _iterator = L.begin(); _iterator != L.end();\
 ++_iterator,V=*_iterator)

//
// kluge to capture printed output in string, so we can return it to the fine
// [incr tsdb()] system.  this file may need some reorganization and cleaning
// sometime soon :-).                                  (27-mar-00  -  oe)
//

static struct MFILE *mstream;

//
// for the moment we just print the k2y representation, rather than
// really constructing it
//

// safeguard against endless recursion
#define K2Y_MAX_DEPTH 256


class K2YSafeguard
{
public:
  K2YSafeguard()
  {
    ++_depth;
    if(_depth > K2Y_MAX_DEPTH)
    {
        ostringstream desc;
        desc << "apparently circular K2Y (depth " << _depth << ")";
	throw error(desc.str());
    }
  }
  ~K2YSafeguard()
  {
    --_depth;
  }
private:
  static int _depth;
};
int K2YSafeguard::_depth = 0;

// if this is true, don't really construct semantics, just count objects
bool evaluate;
static int nrelations;
static set<int> input_ids;

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

  if(nrelations > MAXIMUM_NUMBER_OF_RELS)
  {
      ostringstream desc;
      desc << "apparently circular K2Y (" << nrelations << " relation(s))";
      throw error(desc.str());
    }

  if(!r.labels().empty())
    {
      for(list<int>::iterator i = r.labels().begin(); 
	  i != r.labels().end();
	  i++)
	input_ids.insert(*i);
    }

  if(evaluate) return;

  mprintf(mstream, "  %s[", k2y_name[role]);
  mprintf(mstream, "ID x%d", r.id());
  if(r.cvalue() >= 0) {
    mprintf(mstream, ", REL const_rel, CVALUE %d", r.cvalue());
  } /* if */
  else if(subtype(r.type(), lookup_type(k2y_pred_name("k2y_nomger_rel")))) {
    mprintf(mstream, ", REL nominalize_rel");
  } // else
  else {
    mprintf(mstream, ", REL %s", r.name());
    if(strcmp(r.name(), r.pred())) mprintf(mstream, ", PRED %s", r.pred());
  } /* else */
  mprintf(mstream, ", CLAUSE x%d", clause);

  if(!r.labels().empty())
    {
      mprintf(mstream, ", IDS < ");
      for(list<int>::iterator i = r.labels().begin(); 
	  i != r.labels().end();
	  i++)
	mprintf(mstream, "%d ", *i);

      mprintf(mstream, ">");
    }

  if(!r.senses().empty())
    {
      mprintf(mstream, ", SENSES < ");
      for(list<string>::iterator s = r.senses().begin(); 
	  s != r.senses().end();
	  s++)
	mprintf(mstream, "%s ", s->c_str());

      mprintf(mstream, ">");
    }

  if(role == K2Y_MAINVERB)
  {
    if(index != 0 && index.valid())
      {
	f = index.get_path_value("E.TENSE");
	if(f.valid()) mprintf(mstream, ", TENSE %s", f.name());
	f = index.get_path_value("E.ASPECT");
	if(f.valid()) mprintf(mstream, ", ASPECT %s", f.name());
	f = index.get_path_value("E.MOOD");
	if(f.valid()) mprintf(mstream, ", MOOD %s", f.name());
      }
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
         subtype(argof.type(), lookup_type(k2y_pred_name("k2y_nn_rel"))) ||
         subtype(argof.type(), lookup_type(k2y_pred_name("k2y_app_rel"))))   
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
  K2YSafeguard safeguard;

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
int k2y_vcomp(mrs &m, int clause, int id)
{
  K2YSafeguard safeguard;
  mrs_rel vcomp;
  vcomp = m.rel(k2y_role_name("k2y_hndl"), m.hcons(id), 
                lookup_type(k2y_type_name("k2y_message")));
  if(!vcomp.valid())
    vcomp = m.rel(k2y_role_name("k2y_hndl"), m.hcons(id), 
                  lookup_type(k2y_pred_name("k2y_conj_rel")));
  if(vcomp.valid())
    {
      int message_id = k2y_message(m, id);
      mrs_rel message = m.rel(message_id);
      new_k2y_object(message, K2Y_VCOMP, clause, 0, true);
      return message_id;
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
                    lookup_type(k2y_pred_name("k2y_nom_rel")));
    if(!vcomp.valid())
      vcomp = m.rel(k2y_role_name("k2y_hndl"), m.hcons(id), 
                    lookup_type(k2y_pred_name("k2y_event_rel")));
    if(vcomp.valid())
      {
      int message_id = k2y_message(m, vcomp.value(k2y_role_name("k2y_hndl")));
      mrs_rel message = m.rel(message_id);
      new_k2y_object(message, K2Y_VCOMP, clause, 0, true);
      return message_id;
      }
    else
      return 0;
    }
}

// relative clauses (a special kind of modifier)
void k2y_rc(mrs &m, int clause, int id, int arg)
{
  K2YSafeguard safeguard;
  list<int> props = m.rels(k2y_role_name("k2y_hndl"), id, 
                           lookup_type(k2y_type_name("k2y_message")));
  int i;
  mrs_rel prop;

  forallint(i, props)
    {
      prop = m.rel(i);
      if(prop.valid() && !m.used(prop.id()))
        {
          m.use_rel(prop.id());
          new_k2y_modifier(prop, K2Y_MODIFIER, clause, arg);
          k2y_clause(m, prop.id(), prop.value(k2y_role_name("k2y_soa")));
        }
    }
}

// reduced relative clauses (a special kind of modifier)
void k2y_rrc(mrs &m, int clause, int id, int arg)
{
  K2YSafeguard safeguard;
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
      if((subtype(redprop.type(), 
                  lookup_type(k2y_pred_name("k2y_nonevent_rel"))) &&
          !subtype(redprop.type(), 
                   lookup_type(k2y_pred_name("k2y_adv_rel")))) ||
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
         redprop.id() != arg &&
         (subtype(redprop.type(), 
                  lookup_type(k2y_pred_name("k2y_nonevent_rel"))) ||
          ((afs = redprop.get_fs().get_path_value("EVENT.E.TENSE")).valid()
           && subtype(afs.type(), lookup_type(k2y_type_name("k2y_no_tense")))
           // The check for no_aspect is needed to prevent base-VP complements
           // of modals from acting as reduced relatives
           && (afs = redprop.get_fs().get_path_value("EVENT.E.ASPECT")).valid()
           && (!subtype(redprop.type(), 
                      lookup_type(k2y_pred_name("k2y_verb_rel"))) ||
             !subtype(lookup_type(k2y_type_name("k2y_no_aspect")), 
                      afs.type()))
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

//void k2y_deg(mrs &m, int clause, int argid)
//{
//  mrs_rel arg = m.rel(argid);
//  list<int> degrels = 
//    m.rels(k2y_role_name("k2y_hndl"),
//           arg.value(k2y_role_name("k2y_hndl")),
//           lookup_type(k2y_pred_name("k2y_deg_rel")));
//  int i;
//  fs afs;
//  forallint(i,degrels)
//    {
//      mrs_rel deg = m.rel(i);
//      if(deg.valid() &&
//         (arg.value(k2y_role_name("k2y_dim")) == 
//          deg.value(k2y_role_name("k2y_darg"))))
//        {
//          mrs_rel degdim = 
//            m.rel(k2y_role_name("k2y_inst"),
//                  deg.value(k2y_role_name("k2y_dim")));
//          if(!degdim.valid())
//            new_k2y_modifier(deg, K2Y_MODIFIER, clause, arg.id());
//          else
//            k2y_nom(m, clause, K2Y_IOBJECT, 
//                    deg.value(k2y_role_name("k2y_dim")));
//        }
//    }
//}

void k2y_deg(mrs &m, int clause, int modid, int argid)
{
  mrs_rel mod = m.rel(modid);
  list<int> degrels = 
    m.rels(k2y_role_name("k2y_hndl"),
           mod.value(k2y_role_name("k2y_hndl")),
           lookup_type(k2y_pred_name("k2y_deg_rel")));
  int i;
  fs afs;
  forallint(i,degrels)
    {
      mrs_rel deg = m.rel(i);
      if(deg.valid() &&
         (mod.value(k2y_role_name("k2y_dim")) == 
          deg.value(k2y_role_name("k2y_darg"))))
        {
          mrs_rel degdim = 
            m.rel(k2y_role_name("k2y_inst"),
                  deg.value(k2y_role_name("k2y_dim")));
          if(!degdim.valid())
            new_k2y_modifier(deg, K2Y_MODIFIER, clause, mod.id());
          else
            {
              if(argid)
                {
                  if(opt_k2y_segregation)
                    {
                      mrs_rel pseudo = mrs_rel(&m, lookup_type(k2y_pred_name("k2y_prpstn_rel")));
                      m.push_rel(pseudo);
                      new_k2y_modifier(pseudo, K2Y_MODIFIER, clause, argid),
                        k2y_clause(m, pseudo.id(), 0, mod.id());
                    }
                }
              else
                k2y_nom(m, clause, K2Y_IOBJECT, 
                        deg.value(k2y_role_name("k2y_dim")));
            }
        }
    }
}




// look for modifiers of .id.
void k2y_mod(mrs &m, int clause, int id, int arg)
{
  K2YSafeguard safeguard;
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
              mrs_rel dimrel = m.rel(k2y_role_name("k2y_darg"),
                                     mod.value(k2y_role_name("k2y_dim")));
              if(subtype(argrel.type(), 
                         lookup_type(k2y_pred_name("k2y_verb_rel"))) ||
                 // 10-Feb-02 DPF now treat numeral-adjs as reduced rels, to
                 // allow degree modifiers as in "more than eighty percent"
                 // subtype(mod.type(), 
                 //         lookup_type(k2y_pred_name("k2y_const_rel"))) ||
                 subtype(mod.type(), 
                         lookup_type(k2y_pred_name("k2y_adv_rel"))) ||
                 !opt_k2y_segregation)
                {
                  new_k2y_modifier(mod, K2Y_MODIFIER, clause, arg);
                  // hack for degree phrases without role table segregation
                  if(dimrel.valid() && !opt_k2y_segregation)
                    {
                      mrs_rel nrel = m.rel(k2y_role_name("k2y_inst"),
                                       dimrel.value(k2y_role_name("k2y_dim")));
                      if(nrel.valid())
                        {
                          new_k2y_modifier(nrel, K2Y_MODIFIER, clause, arg);
                          mrs_rel nmrel = m.rel(k2y_role_name("k2y_arg"),
                                        nrel.value(k2y_role_name("k2y_inst")));
                          if(nmrel.valid())
                            new_k2y_modifier(nmrel, K2Y_MODIFIER, clause, arg);
                        }
                    }

                  k2y_deg(m, clause, mod.id(), arg);
                  k2y_nom(m, clause, K2Y_INTARG, mod.value(k2y_role_name("k2y_arg3")), mod.id());
                  k2y_vcomp(m, clause, mod.value(k2y_role_name("k2y_arg4")));
                }
              else
                // treat like reduced relative
                {
                  mrs_rel pseudo = mrs_rel(&m, lookup_type(k2y_pred_name("k2y_prpstn_rel")));
                  m.push_rel(pseudo);
                  new_k2y_modifier(pseudo, K2Y_MODIFIER, clause, 
                                   m.rel(k2y_role_name("k2y_inst"), id).id());
                  k2y_clause(m, pseudo.id(), 0, mod.id());
                }
              m.use_rel(mod.id());
            }
        }
    }
}

// look for a quantifier for .id.
void k2y_quantifier(mrs &m, int clause, int id, int var)
{
  K2YSafeguard safeguard;
  mrs_rel quant = m.rel(k2y_role_name("k2y_bv"), id);
  if(quant.valid()) new_k2y_quantifier(quant, K2Y_QUANTIFIER, clause, var);
}

// noun noun compounds(a special kind of modifier)
void k2y_nn(mrs &m, int clause, int id, int arg)
{
  K2YSafeguard safeguard;
  list<int> nnrels = m.rels(k2y_role_name("k2y_nn_head"), id, 
                            lookup_type(k2y_pred_name("k2y_nn_rel")));
  int i;
  forallint(i, nnrels)
    {
      mrs_rel nnrel = m.rel(i);
      if(nnrel.valid())
        {
          mrs_rel nom = m.rel(k2y_role_name("k2y_inst"),
                              nnrel.value(k2y_role_name("k2y_nn_nonhead")));
          int arg4val = nom.value(k2y_role_name("k2y_arg4"));
          if(arg4val)
            {
              // This is a nominal gerund as in 'the routing list' so do hack:
              mrs_rel hdn = m.rel(k2y_role_name("k2y_inst"),
                                  nnrel.value(k2y_role_name("k2y_nn_head")));
              if(hdn.valid())
                {
                  k2y_rc(m, clause, arg4val, hdn.id());
                  // Add head of N-N compound as "subject" of gerund clause
                  mrs_rel gr = m.rel(k2y_role_name("k2y_hndl"), arg4val,
                                     lookup_type(k2y_pred_name("k2y_prpstn_rel")));
                  if(gr.valid())
                    k2y_nom(m, gr.id(), K2Y_SUBJECT, 
                            nnrel.value(k2y_role_name("k2y_nn_head")));
                }
            }
          else
            {
              if(subtype(nnrel.type(), 
                         lookup_type(k2y_pred_name("k2y_nn_rel"))) &&
                 !subtype(nom.type(), 
                          lookup_type(k2y_pred_name("k2y_num_rel"))) &&
                 opt_k2y_segregation)
                {
                  if(!m.used(nnrel.id()))
                    {
                      mrs_rel pseudo = mrs_rel(&m, lookup_type(k2y_pred_name("k2y_prpstn_rel")));
                      m.push_rel(pseudo);
                      new_k2y_modifier(pseudo, K2Y_MODIFIER, clause, 
                                       m.rel(k2y_role_name("k2y_inst"), id).id());
                      k2y_clause(m, pseudo.id(), 0, nnrel.id());
                    }
                }
              else
                {
                  new_k2y_modifier(nnrel, K2Y_NNCMPND, clause, arg);
                  if(nom.valid() &&
                     !subtype(nom.type(), 
                              lookup_type(k2y_pred_name("k2y_num_rel"))))
                    {
                      new_k2y_intarg(nom, K2Y_INTARG, clause, nnrel);
                      k2y_mod(m, clause, nom.value(k2y_role_name("k2y_inst")), 
                              nom.id());
                      k2y_quantifier(m, clause, 
                                     nom.value(k2y_role_name("k2y_inst")), 
                                     nom.id());
                      k2y_nn(m, clause, 
                             nom.value(k2y_role_name("k2y_inst")), nom.id());
                    }
                }
            }
        }
    }
}

// For narrow appositions like "number thirty", treated like compounds
void k2y_appos(mrs &m, int clause, int id, int arg)
{
  K2YSafeguard safeguard;
  list<int> apprels = m.rels(k2y_role_name("k2y_arg3"), id,
                             lookup_type(k2y_pred_name("k2y_app_rel")));
  int i;
  forallint(i, apprels)
    {
      mrs_rel apprel = m.rel(i);
      if(apprel.valid())
        {
          m.use_rel(apprel.id());
          new_k2y_modifier(apprel, K2Y_NNCMPND, clause, arg);
          mrs_rel nom = m.rel(k2y_role_name("k2y_inst"),
                              apprel.value(k2y_role_name("k2y_arg1")));
          if(nom.valid())
            {
              new_k2y_intarg(nom, K2Y_INTARG, clause, apprel);
              k2y_mod(m, clause, nom.value(k2y_role_name("k2y_inst")), nom.id());
              k2y_quantifier(m, clause, nom.value(k2y_role_name("k2y_inst")),
                             nom.id());
              k2y_nn(m, clause, nom.value(k2y_role_name("k2y_inst")), nom.id());
              k2y_appos(m, clause, nom.value(k2y_role_name("k2y_inst")), nom.id());
            }
        }
    }
}

// look for subject, dobject or iobject, respectively
// DPF 5-Sep-01 - Rethink how "id" is used here, since it is the INST value
// but we say m.used(id, clause) when we should probably say m.used(nom.id()...
bool k2y_nom(mrs &m, int clause, k2y_role role, int id, int argof)
{
  K2YSafeguard safeguard;
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
          k2y_appos(m, clause, nom.value(k2y_role_name("k2y_inst")), nom.id());
          k2y_rc(m, clause, nom.value(k2y_role_name("k2y_hndl")), nom.id());
          k2y_rrc(m, clause, nom.value(k2y_role_name("k2y_hndl")), nom.id());
          k2y_quantifier(m, clause, nom.value(k2y_role_name("k2y_inst")), 
                         nom.id());
          k2y_vcomp(m, clause, nom.value(k2y_role_name("k2y_arg4")));
        }
      return true;
    }
  else if(nom.valid())
    {
      k2y_mod(m, clause, nom.value(k2y_role_name("k2y_inst")), nom.id());
      mrs_rel pseudo;
      if(!nom.value(k2y_role_name("k2y_arg4")) &&
         !m.used(nom.id()))
        {
          // was k2y_hypo_rel
          pseudo = mrs_rel(&m, lookup_type(k2y_pred_name("k2y_prpstn_rel")));
          m.push_rel(pseudo);
        }
      else pseudo = m.rel(k2y_role_name("k2y_hndl"), 
                          nom.value(k2y_role_name("k2y_arg4")));
      m.use_rel(nom.id(), clause);
      if(argof >= 0)
        new_k2y_intarg(nom, role, clause, m.rel(argof));
      else
        new_k2y_object(nom, role, clause, 0, true);
      if(!m.used(pseudo.id()))
        {
          new_k2y_intarg(pseudo, K2Y_VCOMP, clause, nom);
          if(nom.value(k2y_role_name("k2y_arg4")))
            k2y_message(m, nom.value(k2y_role_name("k2y_arg4")));
          else
            k2y_clause(m, pseudo.id(), nom.value(k2y_role_name("k2y_hndl")));
          m.use_rel(pseudo.id());
        }
      k2y_rrc(m, clause, nom.value(k2y_role_name("k2y_hndl")), nom.id());
      return pseudo.id();
    }
  else
    {
      mrs_rel r = m.rel(k2y_role_name("k2y_c_arg"), id);
      if(r.valid() && !m.used(r.id()))
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
  K2YSafeguard safeguard;
  mrs_rel verb = m.rel(id);

  if(verb.valid())
  {
    k2y_mod(m, clause, verb.value(k2y_role_name("k2y_event")), id);
    int subjid;
    if(subtype(verb.type(), lookup_type(k2y_pred_name("k2y_arg3_rel"))))
      subjid = verb.value(k2y_role_name("k2y_arg3"));
    else if(subtype(verb.type(), lookup_type(k2y_pred_name("k2y_ellips_rel"))))
      subjid = verb.value(k2y_role_name("k2y_role"));
    else
      {
	if(subtype(verb.type(), lookup_type(k2y_pred_name("k2y_arg1_rel"))))
          subjid = verb.value(k2y_role_name("k2y_arg1"));
        else
          subjid = verb.value(k2y_role_name("k2y_arg"));
      }
    
    // The following is needed for post-VP verbal modifiers, as in
    // "Kim arrived laughing"
    mrs_rel subjrel = m.rel(k2y_role_name("k2y_inst"), subjid);
    if(subjrel.valid())
      k2y_rrc(m, clause, verb.value(k2y_role_name("k2y_hndl")), 
             subjrel.id());

    k2y_vcomp(m, clause, verb.value(k2y_role_name("k2y_arg4")));
    k2y_nom(m, clause, K2Y_SUBJECT, subjid);
    k2y_nom(m, clause, K2Y_DOBJECT, verb.value(k2y_role_name("k2y_arg3")));
    k2y_nom(m, clause, K2Y_IOBJECT, verb.value(k2y_role_name("k2y_arg2")));
    k2y_particle(m, clause, verb.value(k2y_role_name("k2y_hndl")), verb.id());
    fs foo;
    if((foo = verb.get_fs().get_path_value(k2y_role_name("k2y_argh"))).valid())
      {
        if(subtype(foo.type(), lookup_type(k2y_type_name("k2y_handle"))))
          k2y_vcomp(m, clause, verb.value(k2y_role_name("k2y_argh")));
        else
          {
            int message_id = k2y_message(m, verb.value(k2y_role_name("k2y_argh")));
            mrs_rel message = m.rel(message_id);
            new_k2y_object(message, K2Y_VCOMP, clause, 0, true);
          }
      }
    //
    // special case for modifiers that (syntactically) attach to a nominalized
    // projection of this verb, e.g. `atarashii yokin'
    //
    mrs_rel wa = m.rel(k2y_role_name("k2y_arg3"), 
                       verb.value(k2y_role_name("k2y_event")));
    if(wa.valid()) {
      mrs_rel mod = m.rel(k2y_role_name("k2y_event"),
                          wa.value(k2y_role_name("k2y_arg")));
      if(mod.valid()) {
        new_k2y_modifier(mod, K2Y_MODIFIER, clause, id);
        k2y_nom(m, clause, K2Y_INTARG, 
                mod.value(k2y_role_name("k2y_arg3")), mod.id());
        k2y_vcomp(m, clause, mod.value(k2y_role_name("k2y_arg4")));
      } // if 
    } //if 
  }
}

int k2y_clause_conjunction(mrs &m, int clause, int id, int mv_id)
{
  K2YSafeguard safeguard;
   mrs_rel conj = mrs_rel();
   list<int> ids;

   if(mv_id == 0)
     conj = m.rel(k2y_role_name("k2y_hndl"), m.hcons(id), 
                  lookup_type(k2y_pred_name("k2y_conj_rel")));

   if(conj.valid() && subtype(conj.type(), lookup_type(k2y_pred_name("k2y_conj_rel"))))
     {
       char *indices[] = { "k2y_l_index", "k2y_r_index", NULL };
       list<int> conjs = conj.id_list_by_roles(indices);

      if(!conjs.empty())
        {
          ids = k2y_conjuncts(m, clause, k2y_role_name("k2y_inst"), conjs);
          if(ids.empty() &&
             m.rel(k2y_role_name("k2y_event"), conjs.front(),
                   lookup_type(k2y_pred_name("k2y_prep_rel"))).valid())
            ids = k2y_conjuncts(m, clause, k2y_role_name("k2y_event"), conjs);
        }
     }
   if(conj.valid() && 
      subtype(conj.type(), lookup_type(k2y_pred_name("k2y_conj_rel"))) && 
      ids.empty())
   {
     list<int> conjs = k2y_conjunction(m, clause, conj.id());
     fs foo;
     if((foo = conj.get_fs().get_path_value(k2y_role_name("k2y_r_handel"))).valid()
        && subtype(foo.type(), lookup_type(k2y_type_name("k2y_handle"))))
       { 
	 int i;
         forallint(i, conjs)
           {
             mrs_rel cnjnct = m.rel(i);
             //if(!m.used(i))
             //  k2y_vcomp(m, clause, cnjnct.value(k2y_role_name("k2y_hndl")));
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
  K2YSafeguard safeguard;
  mrs_rel subord = mrs_rel();
  if(mv_id == 0)
    subord = m.rel(k2y_role_name("k2y_hndl"), m.hcons(id), 
                   lookup_type(k2y_pred_name("k2y_subord_rel")));
  if(subord.valid())
    {
      char *indices[] = { "k2y_main", "k2y_subord", NULL };
      list<int> subs = subord.id_list_by_roles(indices);
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
  K2YSafeguard safeguard;
  mrs_rel exclrel = mrs_rel();
  if(mv_id == 0)
      exclrel = m.rel(k2y_role_name("k2y_hndl"), id, 
                      lookup_type(k2y_pred_name("k2y_excl_rel")));
  else if(!id == 0)
    exclrel = m.rel(mv_id);
  if(exclrel.valid())
    {
      new_k2y_object(exclrel, K2Y_MAINVERB, clause, 0, true);
      //return exclrel.id();
      return clause;
    }
  return 0;
}

int k2y_clause(mrs &m, int clause, int id, int mv_id)
{
  K2YSafeguard safeguard;
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
           new_k2y_object(mv, K2Y_MAINVERB, clause, index, true);
           m.use_rel(mv.id());
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
               if(!subtype(mv.type(), lookup_type(k2y_pred_name("k2y_adj_rel"))) &&
                  !subtype(mv.type(), lookup_type(k2y_pred_name("k2y_const_rel"))))
                 {
                   newmv = mrs_rel(&m, lookup_type(k2y_pred_name("k2y_cop_rel")));
                   m.push_rel(newmv);
                 }
               index = mv.get_fs().get_path_value(k2y_role_name("k2y_event"));
               new_k2y_object(newmv, K2Y_MAINVERB, clause, index, true);
               m.use_rel(newmv.id());
               mrs_rel argrel = m.rel(k2y_role_name("k2y_inst"), 
                                      mv.value(k2y_role_name("k2y_arg")));
               if(subtype(mv.type(), lookup_type(k2y_pred_name("k2y_nn_rel")))
                  && opt_k2y_segregation)
                 {
                   new_k2y_modifier(mv, K2Y_MODIFIER, clause, newmv.id());
                   k2y_nom(m, clause, K2Y_INTARG, 
                           mv.value(k2y_role_name("k2y_nn_nonhead")), mv.id());
                 }
               else 
                 {
                   if(subtype(mv.type(), 
                              lookup_type(k2y_pred_name("k2y_prep_rel"))))
                     {
                       if(!subtype(mv.type(), 
                                lookup_type(k2y_pred_name("k2y_select_rel"))))
                         {
                           new_k2y_modifier(mv, K2Y_MODIFIER, clause, 
                                            newmv.id());
                           k2y_deg(m, clause, mv.id(), 0);
                           k2y_nom(m, clause, K2Y_INTARG, 
                                   mv.value(k2y_role_name("k2y_arg3")), 
                                   mv.id());
                         }
                     }
                   else 
                     {
                       if(!subtype(mv.type(), lookup_type(k2y_pred_name("k2y_adj_rel"))) &&
                          !subtype(mv.type(), lookup_type(k2y_pred_name("k2y_const_rel"))))
                         {
                           new_k2y_modifier(mv, K2Y_MODIFIER, clause, 
                                            argrel.id());
                           k2y_nom(m, clause, K2Y_INTARG, 
                                 mv.value(k2y_role_name("k2y_arg3")), mv.id());
                         }
                       else 
                         {
                           if(mv.value(k2y_role_name("k2y_dim")));
                           if(opt_k2y_segregation)
                             k2y_deg(m, clause, mv.id(), 0);
                           k2y_nom(m, clause, K2Y_DOBJECT, 
                                   mv.value(k2y_role_name("k2y_arg3")), 
                                   mv.id());
                           k2y_particle(m, clause, 
                                        mv.value(k2y_role_name("k2y_hndl")), 
                                        mv.id());
                           k2y_mod(m, clause, mv.value(k2y_role_name("k2y_event")), mv.id());
                         }
                     }
                 }
               m.use_rel(mv.id());
               k2y_nom(m, clause, K2Y_SUBJECT, mv.value(k2y_role_name("k2y_arg")));
               k2y_nom(m, clause, K2Y_SUBJECT, mv.value(k2y_role_name("k2y_nn_head")));
               k2y_vcomp(m, clause, mv.value(k2y_role_name("k2y_arg4")));
               k2y_mod(m, clause, mv.value(k2y_role_name("k2y_event")), 
                       newmv.id());
               list<int> pprels = m.rels(k2y_role_name("k2y_arg"), 
                                         mv.value(k2y_role_name("k2y_event")),
                                         lookup_type(k2y_pred_name("k2y_prep_rel")));
               int i;
               forallint(i,pprels)
                 { 
                   mrs_rel pprel = m.rel(i);
                   if(!(pprel.id() == mv.id()) &&
                      !subtype(pprel.type(), 
                               lookup_type(k2y_pred_name("k2y_select_rel"))) &&
                      !m.used(pprel.id()))
                     {
                       new_k2y_modifier(pprel, K2Y_MODIFIER, clause, newmv.id());
                       k2y_deg(m, clause, pprel.id(), newmv.id());
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
  K2YSafeguard safeguard;
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
         if(!subtype(conj.type(),
                     lookup_type(k2y_pred_name("k2y_excl_rel")))
            && !m.used(conj.id()))
           k2y_vcomp(m, pseudo.id(), conj.value(k2y_role_name("k2y_hndl")));
       }
     }
     else
     {
       if(!k2y_clause_exclam(m, pseudo.id(), id, 0))
         {

           rel = m.rel(k2y_role_name("k2y_hndl"), id, 
                       lookup_type(k2y_pred_name("k2y_nom_rel"))) ;
           if(rel.valid())
             k2y_nom(m, pseudo.id(), K2Y_DOBJECT, 
                     rel.value(k2y_role_name("k2y_inst")));
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
         }
     }
     return pseudo.id();
  }
}

list<int> k2y_conjuncts(mrs &m, int clause, char *path, list<int> conjs, bool mvsearch)
{
  K2YSafeguard safeguard;
  list<int> ids;
  int i;
  
  forallint(i, conjs)
  {
    mrs_rel r;
    if(mvsearch)
      r = m.rel(path, i, lookup_type(k2y_pred_name("k2y_verb_rel")));
    if(!mvsearch || !r.valid())
      {
        int rc;
        list<int> rcands = m.rels(path,i);
        forallint(rc, rcands)
          if(m.rel(rc).valid()
             && !subtype(m.rel(rc).type(), 
                         lookup_type(k2y_pred_name("k2y_deg_rel"))))
            {
              if(subtype(m.rel(rc).type(),
                         lookup_type(k2y_pred_name("k2y_excl_rel"))) ||
                 subtype(m.rel(rc).type(),
                         lookup_type(k2y_pred_name("k2y_conj_rel"))))
                {
                  int vcid = k2y_vcomp(m, clause, 
                                       m.rel(rc).value(k2y_role_name("k2y_hndl")));
                  m.use_rel(vcid);
                  r = m.rel(vcid);
                }
              else
                r = m.rel(rc);
            }
      }
    if(r.valid()) 
      ids.push_front(r.id());
    else 
      {
        mrs_rel conjrel = m.rel(k2y_role_name("k2y_c_arg"), i);
        list<int> newids;
        if(conjrel.valid())
          {
            fs foo;
            if((foo = (conjrel.get_fs().get_path_value("R-HANDEL"))).valid())
              {
                char *paths[] = { "k2y_l_handel", "k2y_r_handel", NULL };
                list<int> conjs = conjrel.id_list_by_roles(paths);
                newids = k2y_conjuncts(m, clause, k2y_role_name("k2y_hndl"), 
                                       conjs);
              }
            else
              {
                char *paths[] = { "k2y_l_index", "k2y_r_index", NULL };
                list<int> conjs = conjrel.id_list_by_roles(paths);
                newids = k2y_conjuncts(m, clause, k2y_role_name("k2y_inst"), 
                                       conjs);
              }
            if(!newids.empty())
              {
                int j;
                forallint(j, newids)              
                  ids.push_front(j);                
              }
          }
      }
  }
  return ids;
}

list<int> k2y_conjunction(mrs &m, int clause, int conj_id)
{
  K2YSafeguard safeguard;
  list<int> ids;
  mrs_rel conj = m.rel(conj_id);
  char *paths[] = { "k2y_l_handel", "k2y_r_handel", NULL };
  list<int> conjs = conj.id_list_by_roles(paths);

  if(conj.valid() && !m.used(conj.id()))
  {
    fs foo;
    mrs_rel bar;
    if((foo = (conj.get_fs().get_path_value("R-HANDEL"))).valid()
       && subtype(foo.type(), lookup_type(k2y_type_name("k2y_handle")))
       && (bar = m.rel(k2y_role_name("k2y_hndl"), conjs.front())).valid()
       && (subtype(bar.type(), lookup_type(k2y_type_name("k2y_message"))) ||
           // DPF 17-Dec-01 - Commented out following line, which was
           // blocking one conjunct for "Kim arrives and laughs and falls"
           //subtype(bar.type(), lookup_type(k2y_pred_name("k2y_conj_rel"))) ||
           subtype(bar.type(), lookup_type(k2y_pred_name("k2y_excl_rel")))))

      {
        ids = k2y_conjuncts(m, clause, k2y_role_name("k2y_hndl"), conjs);
        new_k2y_conj(conj, K2Y_CONJ, clause, ids);
        m.use_rel(conj.id());
      }
    else
    {
      if(conjs.empty() ||
         ((bar = m.rel(k2y_role_name("k2y_hndl"), conjs.front())).valid() &&
          subtype(bar.type(), lookup_type(k2y_pred_name("k2y_prep_rel")))))
        {
        char *paths[] = { "k2y_l_index", "k2y_r_index", NULL };
        conjs = conj.id_list_by_roles(paths);
        if(!conjs.empty())
          {
            ids = k2y_conjuncts(m, clause, k2y_role_name("k2y_inst"), conjs);
            if(ids.empty())
             ids = k2y_conjuncts(m, clause, k2y_role_name("k2y_event"), conjs);
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
                 if(!m.used(conjnct.id()))
                   {
                     int message_id = 
                      k2y_message(m, conjnct.value(k2y_role_name("k2y_hndl")));
                     mrs_rel message = m.rel(message_id);
                     newconjs.push_front(message.id());
                     new_k2y_object(message, K2Y_VCOMP, clause, 0, true);
                     // DPF 11-Feb-01 - Next line doesn't work, but should for
                     // 'kim is null and void for sandy'
                     //k2y_mod(m, message_id, 
                     //     conj.value(k2y_role_name"k2y_c_arg")), conjnct.id());
                   }
                 else
                   newconjs.push_front(conjnct.id());
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
  K2YSafeguard safeguard;
  int message_id = k2y_message(m, m.top());
  mrs_rel message = m.rel(message_id);

  if(message.valid()) 
    new_k2y_object(message, K2Y_SENTENCE, message.id(), 0, true);
}

//
// utility functions
//


int construct_k2y(int id, item *root, bool eval, struct MFILE *stream)
{

  evaluate = eval;
  if(!eval && stream == NULL) return(-1);
  mstream = (stream != NULL ? mopen() : (MFILE *)NULL);

  try {

    int status = 0;

    mrs m(root->get_fs());

    if(!eval && cheap_settings->lookup("debug-mrs")) {
      m.print(ferr);
      fprintf(ferr, "\n");
    } /* if */

    nrelations = 0; 
    input_ids.clear();

    k2y_sentence(m);

    if(!cheap_settings->lookup("debug-mrs")
       && (int)input_ids.size() < root->end() * (float)opt_k2y / 100) {
      if(!eval && verbosity > 1) {
        fprintf(ferr, 
                "construct_k2y(): [%d of %d] "
                "sparse K2Y (%d input id(s)).\n",
                id, stats.readings, input_ids.size());
      } /* if */
      if(stream != NULL) {
        mprintf(stream, 
                "([%d--%d] %d of %d: "
                "sparse K2Y (%d input id(s)))",
                root->start(), root->end(), 
                id, stats.readings, input_ids.size());
      } /* if */
      status = -1;
    } /* if */
    else {
      if(stream != NULL) {
        mprintf(stream, 
                "{%d--%d; %d of %d; [%d relation%s %d input id%s];\n%s}", 
                root->start(), root->end(), 
                id, stats.readings,
                nrelations, (nrelations > 1 ? "s" : ""),
                input_ids.size(), (input_ids.size() > 1 ? "s" : ""),
                mstring(mstream));
      } /* if */
      status = nrelations;
    } /* else */
    if(mstream != NULL) mclose(mstream);
    mstream = ((MFILE *)NULL);
    return status;

  } /* try */

  catch(error &condition) {
    if(!eval) {
      fprintf(ferr, 
              "L2 error: construct_k2y(): [%d of %d] ",
              id, stats.readings);
      condition.print(ferr);
      fprintf(ferr, "\n");
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

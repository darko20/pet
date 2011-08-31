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

/* unpacking chart items */

#include "pet-config.h"

#include "resources.h"
#include "item.h"
#include "item-printer.h"
#include "logging.h"
#include "tsdb++.h"
#include "configs.h"  // might go when Resources are in place
#include "cheap.h"
#include "sm.h"

#include <iomanip>
#include <sstream>

using namespace std;

// ****************************************************************************
// Unpacking: methods of tItem and subclasses
// ****************************************************************************

// for printing debugging output
int unpacking_level;

// 2006/10/01 Yi Zhang <yzhang@coli.uni-sb.de>:
// option for grand-parenting level in MEM-based parse selection
// \todo Not nice, but it's a global option that may be considered a
// system-global const
unsigned int opt_gplevel;

extern bool characterize(fs &thefs, int from, int to);

list<tItem *>
tItem::unpack(Resources &resources)
{
    list<tItem *> res;

    unpacking_level++;
    LOG(logUnpack, DEBUG, std::setw(unpacking_level * 2) << ""
        << "> unpack [" << id() << "]" );

    // Ignore frozen items.
    if(frozen())
    {
        LOG(logUnpack, DEBUG, std::setw(unpacking_level * 2) << ""
            << "> unpack [" << id() << "] ( )" );
        unpacking_level--;
        return res;
    }

    if(_unpack_cache)
    {
        // This item has already been unpacked
        unpacking_level--;
        return *_unpack_cache;
    }


    // Check if we reached a resource limit. Caller is responsible for
    // checking this to verify completeness of results.
    if(resources.exhausted())
        return res;

    // Recursively unpack items that are packed into this item.
    for(item_iter p = packed.begin(); p != packed.end(); ++p)
    {
        // Append result of unpack_item on packed item.
        item_list tmp = (*p)->unpack(resources);
        res.splice(res.begin(), tmp);
    }

    // Unpack children.
    list<tItem *> tmp = unpack1(resources);
    res.splice(res.begin(), tmp);

    if(LOG_ENABLED(logUnpack, DEBUG))
    {
        ostringstream updeb;
        updeb << std::setw(unpacking_level * 2) << ""
              << "> unpack [" << id() << "] (";
        for(item_citer i = res.begin(); i != res.end(); ++i)
          updeb << (*i)->id() << " ";
        updeb << ")";
        LOG(logUnpack, DEBUG, updeb.str());
    }

    _unpack_cache = new list<tItem *>(res);

    unpacking_level--;
    return res;
}

/** \brief Since tInputItems do not have a feature structure, they need not be
 *  unpacked. Unpacking does not proceed past tLexItem
 */
list<tItem *>
tInputItem::unpack1(Resources &resources)
{
    list<tItem *> res;
    res.push_back(this);
    return res;
}

item_list
tLexItem::unpack1(Resources &resources) {
  // reconstruct the full feature structure and do characterization
  item_list res;
  res.push_back(this);
  return res;
}

item_list
tPhrasalItem::unpack1(Resources &resources)
{
    // Collect expansions for each daughter.
    vector<list<tItem *> > dtrs;
    for(item_iter dtr = _daughters.begin();
        dtr != _daughters.end(); ++dtr)
    {
        dtrs.push_back((*dtr)->unpack(resources));
    }

    // Consider all possible combinations of daughter structures
    // and collect the ones that combine.
    vector<tItem *> config(rule()->arity());
    item_list res;
    unpack_cross(dtrs, 0, config, res, resources);

    return res;
}

void
debug_unpack(tItem *combined, int motherid, vector<tItem *> &config) {
  ostringstream cdeb;
  cdeb << setw(unpacking_level * 2) << "" ;
  if (combined != NULL) {
    cdeb << "created edge " << combined->id() << " from " ;
  } else {
    cdeb << "failure instantiating ";
  }
  cdeb << motherid << "[";
  vector<tItem *>::iterator it = config.begin();
  if (it != config.end()) { cdeb << (*it)->id(); ++it; }
  for(; it != config.end(); ++it)
    cdeb << " " << (*it)->id();
  cdeb << "]" << endl;
  if (combined != NULL) {
    cdeb << *combined << endl;
    if(LOG_ENABLED(logUnpack, DEBUG))
      cdeb << combined->get_fs().dag();
  }
  LOG(logUnpack, INFO, cdeb.str());
}

// Recursively compute all configurations of dtrs, and accumulate valid
// instantiations (wrt mother) in res.
void
tPhrasalItem::unpack_cross(vector<list<tItem *> > &dtrs,
                           int index, vector<tItem *> &config,
                           list<tItem *> &res, Resources &resources) {
  if(index >= rule()->arity()) {
    tItem *combined = unpack_combine(config);
    if (LOG_ENABLED(logUnpack, INFO)) debug_unpack(combined, id(), config);
    if(combined) {
      ++ resources.pedges;
      res.push_back(combined);
    }
    else {
      stats.p_failures++;
    }
    if (resources.exhausted()) {
      throw tExhaustedError(resources.exhaustion_message());
    }
    return;
  }

  for(item_iter i = dtrs[index].begin(); i != dtrs[index].end(); ++i) {
    config[index] = *i;
    unpack_cross(dtrs, index + 1, config, res, resources);
  }
}

// Try to instantiate mother with a particular configuration of daughters.
// _fix_me_
// This is quite similar to functionality in task.cpp - common functionality
// should be factored out.
tItem *
tPhrasalItem::unpack_combine(vector<tItem *> &daughters) {
  fs_alloc_state FSAS(false);

  fs res = rule()->instantiate(true);

  list_int *tofill = rule()->allargs();

  while (res.valid() && tofill) {
    fs arg = res.nth_arg(first(tofill));
    if(res.temp())
      unify_generation = res.temp();
    if(rest(tofill)) {
      res = unify_np(res, daughters[first(tofill)-1]->get_fs(true), arg);
    }
    else {
      // _fix_me_
      // the whole _np architecture is rather messy
      res = unify_restrict(res, daughters[first(tofill)-1]->get_fs(true),
                           arg,
                           Grammar->deleted_daughters());
    }
    tofill = rest(tofill);
  }

  if(!res.valid()) {
    FSAS.release();
    return 0;
  }

  if(passive()) {
    characterize(res, _startposition, _endposition);
  }

  tPhrasalItem *result = new tPhrasalItem(this, daughters, res);
  if(result && Grammar->sm()) {
    result->score(Grammar->sm()->scoreLocalTree(result->rule(), daughters));
  } // if

  return result;
}

// **********************************************************************
// EXHAUSTIVE UNPACKING ENDS HERE ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//
// SELECTIVE UNPACKING STARTS HERE vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// **********************************************************************

/** Advance the indices vector.
 * e.g. <0 2 1> -> {<1 2 1> <0 3 1> <0 2 2>}
 */
static list<vector<int> > advance_indices(vector<int> indices) {
  list<vector<int> > results;
  for (unsigned int i = 0; i < indices.size(); ++i) {
    vector<int> new_indices = indices;
    new_indices[i]++;
    results.push_back(new_indices);
  }
  return results;
}

/** Insert hypothesis into agenda. Agenda is sorted descendingly
 *
 * \todo REPLACE THIS WITH A PROPER PRIORITY QUEUE! THIS IS LINEAR SEARCH!
 */
static void hagenda_insert(list<tHypothesis*> &agenda, tHypothesis* hypo,
                           list<tItem*> path) {
  if (agenda.empty()) {
    agenda.push_back(hypo);
    return;
  }
  int flag = 0;
  for (list<tHypothesis*>::iterator it = agenda.begin();
       it != agenda.end(); ++it) {
    if (hypo->scores[path] > (*it)->scores[path]) {
      agenda.insert(it, hypo);
      flag = 1;
      break;
    }
  }
  if (!flag)
    agenda.push_back(hypo);
}

/** Decompose edge and return the number of decompositions
 * All the decompositions are recorded in this->decompositions.
 * \todo bk: I don't see why it's necessary to store the decompositions,
 * because this function is only called once in hypothesize edge and could
 * as well return the computed decompositions, which are only used there
 * locally. So why store them in the item?
 * Deletion of the created decompositions gets much easier, though.
 */
int
tPhrasalItem::decompose_edge(std::list<tDecomposition*> &decompositions)
{  /** A list of decompositions */

  int dnum = 1;
  if (_daughters.size() == 0) {
    return 0;
  }
  vector<vector<tItem*> > dtr_items;
  int i = 0;
  for (list<tItem*>::const_iterator it = _daughters.begin();
       it != _daughters.end(); ++it, ++i) {
    dtr_items.resize(i+1);
    dtr_items[i].push_back(*it);
    for (list<tItem*>::const_iterator pi = (*it)->packed.begin();
         pi != (*it)->packed.end(); ++pi) {
      if (!(*pi)->frozen()) {
        dtr_items[i].push_back(*pi);
      }
    }
    dnum *= dtr_items[i].size();
  }

  for (int i = 0; i < dnum; ++i) {
    list<tItem*> rhs;
    int j = i;
    int k = 0;
    for (list<tItem*>::const_iterator it = _daughters.begin();
         it != _daughters.end(); ++it, ++k) {
      int psize = dtr_items[k].size();
      int mod = j % psize;
      rhs.push_back(dtr_items[k][mod]);
      j /= psize;
    }
    decompositions.push_back(new tDecomposition(rhs));
  }
  return dnum;
}


tHypothesis *
tInputItem::hypothesize_edge(item_list path, unsigned int i,
                             Resources &resources) {
  return NULL;
}

tHypothesis *
tLexItem::hypothesize_edge(item_list path, unsigned int i,
                           Resources &resources) {
  if (i == 0) {
    if (_hypo == NULL) {
      _hypo = new tHypothesis(this);
      stats.p_hypotheses ++;
    }
    Grammar->sm()->score_hypothesis(_hypo, path, opt_gplevel);

    return _hypo;
  } else
    return NULL;
}

tHypothesis *
tPhrasalItem::hypothesize_edge(item_list path, unsigned int i,
                               Resources &resources) {
  tHypothesis *hypo = NULL;

  // check whether timeout has passed.
  if (resources.exhausted()) {
    return hypo;
  }

  // Only check the path length no longer than the opt_gplevel
  while (path.size() > opt_gplevel)
    path.pop_front();

  if (_hypo_agendas.find(path) == _hypo_agendas.end()) {
    // This is a new path:
    // * initialize the agenda
    // * score the hypotheses
    // * create the hypothese cache
    _hypo_agendas[path].clear();
    for (vector<tHypothesis*>::iterator h = _hypotheses.begin();
         h != _hypotheses.end(); ++h) {
      Grammar->sm()->score_hypothesis(*h, path, opt_gplevel);
      hagenda_insert(_hypo_agendas[path], *h, path);
    }
    // \todo could somebody please explain this (yi?)
    _hypo_path_max[path] = UINT_MAX;
  }

  // Check cached hypotheses
  if (i < _hypotheses_path[path].size() && _hypotheses_path[path][i])
    return _hypotheses_path[path][i];

  // Quick return for failed hypothesis
  if (i >= _hypo_path_max[path])
    return NULL;

  // Create new path for daughters
  list<tItem*> newpath = path;
  newpath.push_back(this);
  if (newpath.size() > opt_gplevel)
    newpath.pop_front();

  // Initialize the set of decompositions and pushing initial
  // hypotheses onto the local agenda when called on an edge for the
  // first time.  This initialization should only be done for the
  // first hypothesis of the first path, as for the following path(s).
  if (i == 0 && _hypotheses_path.size() == 1) {
    // TODO
    // it is still unclear who will destroy the created decompositions
    list <tDecomposition*> decompositions;
    decompose_edge(decompositions);
    for (list<tDecomposition*>::iterator decomposition = decompositions.begin();
         decomposition != decompositions.end(); ++decomposition) {
      list<tHypothesis*> dtrs;
      vector<int> indices;
      for (list<tItem*>::const_iterator edge = (*decomposition)->rhs.begin();
           edge != (*decomposition)->rhs.end(); ++edge) {
        tHypothesis* dtr = (*edge)->hypothesize_edge(newpath, 0, resources);
        if (!dtr) {
          dtrs.clear();
          break;
        }
        dtrs.push_back(dtr);
        indices.push_back(0);
      }
      if (dtrs.size() != 0) {
        new_hypothesis(*decomposition, dtrs, indices);
        (*decomposition)->indices.insert(indices);
      }
    }
  }

  while (!_hypo_agendas[path].empty() && i >= _hypotheses_path[path].size()) {
    hypo = _hypo_agendas[path].front();
    _hypo_agendas[path].pop_front();
    list<vector<int> > indices_adv = advance_indices(hypo->indices);

    while (!indices_adv.empty()) {
      vector<int> indices = indices_adv.front();
      indices_adv.pop_front();
      // skip seen configurations
      if (! hypo->decomposition->indices.insert(indices).second) // seen before
        continue;

      list<tHypothesis*> dtrs;
      list<int> fdtr_idx;
      int idx = 0;
      for (item_iter edge = hypo->decomposition->rhs.begin();
           edge != hypo->decomposition->rhs.end(); ++edge, ++idx) {
        tHypothesis* dtr =
          (*edge)->hypothesize_edge(newpath, indices[idx], resources);
        if (!dtr) {
          dtrs.clear();
          break;
        }
        if (dtr->inst_failed) // record the failed positions
          fdtr_idx.push_back(idx);

        dtrs.push_back(dtr);
      }
      if (dtrs.size() == 0) // at least one of the daughter hypotheses
                            // does not exist
        continue;
      else if (fdtr_idx.size() > 0) { // at least one of the daughter
                                       // hypotheses failed to
                                       // instantiate
        // skip creating the hypothesis that determined not to instantiate,
        // but still put in the advanced indices
        vector<int> new_indices = indices;
        for (list<int>::iterator fidx = fdtr_idx.begin();
             fidx != fdtr_idx.end(); ++fidx) {
          new_indices[*fidx] ++;
        }
        indices_adv.push_back(new_indices);
      } else // every thing fine, create the new hypothesis
        new_hypothesis(hypo->decomposition, dtrs, indices);
    }
    //    if (!hypo->inst_failed) // this will cause missing readings when used with grandparenting
    _hypotheses_path[path].push_back(hypo);
  }
  if (i < _hypotheses_path[path].size()){
    if (_hypo_agendas[path].size() == 0)
      _hypo_path_max[path] = _hypotheses_path[path].size();
    return _hypotheses_path[path][i];
  }
  else {
    _hypo_path_max[path] = _hypotheses_path[path].size();
    return NULL;
  }
  //  return hypo;
}


void
tPhrasalItem::new_hypothesis(tDecomposition* decomposition,
                             list<tHypothesis *> dtrs,
                             vector<int> indices)
{
  tHypothesis *hypo = new tHypothesis(this, decomposition, dtrs, indices);
  stats.p_hypotheses ++;
  _hypotheses.push_back(hypo);
  for (map<list<tItem*>, list<tHypothesis*> >::iterator iter =
         _hypo_agendas.begin();
       iter != _hypo_agendas.end(); ++iter) {
    Grammar->sm()->score_hypothesis(hypo, (*iter).first, opt_gplevel);
    hagenda_insert(_hypo_agendas[(*iter).first], hypo, (*iter).first);
  }
}


tItem*
tInputItem::instantiate_hypothesis(item_list path, tHypothesis * hypo,
                                   Resources &resources) {
  score(hypo->scores[path]);
  return this;
}

tItem *
tLexItem::instantiate_hypothesis(item_list path, tHypothesis * hypo,
                                 Resources &resources) {
  score(hypo->scores[path]);
  return this;
}

tItem *
tPhrasalItem::instantiate_hypothesis(item_list path, tHypothesis * hypo,
                                     Resources &resources) {

  // Check if we reached the unpack edge limit. Caller is responsible for
  // checking this to verify completeness of results.
  if(resources.exhausted())
    return this;

  // If the hypothesis has been instantiated before, don't do it again
  if (hypo->inst_edge != NULL)
    return hypo->inst_edge;

  // If the instantiation failed before, don't bother trying again
#ifdef INSTFC
  if (hypo->inst_failed)
    return NULL;
#endif

  vector<tItem*> daughters;

  // Only check the path length no longer than the opt_gplevel
  while (path.size() > opt_gplevel)
    path.pop_front();

  list<tItem*> newpath = path;
  newpath.push_back(this);
  if (newpath.size() > opt_gplevel)
    newpath.pop_front();

  // Instantiate all the sub hypotheses.
  for (list<tHypothesis*>::iterator subhypo = hypo->hypo_dtrs.begin();
       subhypo != hypo->hypo_dtrs.end(); ++subhypo) {
    tItem* dtr = (*subhypo)->edge->instantiate_hypothesis(newpath, *subhypo,
                                                          resources);
    if (dtr)
      daughters.push_back(dtr);
    else {
      //hypo->inst_failed = true;//
      return NULL;
    }
  }

  tPhrasalItem *result;
  if (trait() != PCFG_TRAIT) {
    // Replay the unification.
    fs res = rule()->instantiate(true);
    list_int *tofill = rule()->allargs();
    while (res.valid() && tofill) {
      fs arg = res.nth_arg(first(tofill));
      if (res.temp())
        unify_generation = res.temp();
      if (rest(tofill)) {
        res = unify_np(res, daughters[first(tofill)-1]->get_fs(true), arg);
      } else {
        res = unify_restrict(res, daughters[first(tofill)-1]->get_fs(true),
                             arg,
                             Grammar->deleted_daughters());
      }
      tofill = rest(tofill);
      // I don't think it's necessary to check resources here, this is only a
      // single rule instantiation, comparable to a task execution, that will
      // be caught in the next step? Most likely so. TODO: check this
      if (resources.exhausted()) {
        throw tExhaustedError(resources.exhaustion_message());
      }
    }

    if (!res.valid()) {
      //    FSAS.release();
      //hypo->inst_failed = true;//
#ifdef INSTFC
      propagate_failure(hypo);
#endif
      stats.p_failures ++;
      return NULL;
    }
    if (passive()) {
      characterize(res, _startposition, _endposition);
    }

    result = new tPhrasalItem(this, daughters, res);
  } else {
    result = new tPhrasalItem(this, daughters);
  }

  ++ resources.pedges;
  result->score(hypo->scores[path]);
  hypo->inst_edge = result;
  return result;
}

#ifdef INSTFC
void propagate_failure(tHypothesis *hypo) {
  hypo->inst_failed = true;

  for (list<tHypothesis *>::iterator p_hypo = hypo->hypo_parents.begin();
       p_hypo != hypo->hypo_parents.end(); ++p_hypo) {
    propagate_failure(*p_hypo);
  }
}
#endif



template<typename T> class element_releaser {
private:
  list<T> &_list;

public:
  element_releaser(list<T> &l) : _list(l) {}

  ~element_releaser() {
    // release allocated elements
    for (typename list<T>::iterator it = _list.begin();
         it != _list.end();
         ++it)
      delete *it;
    _list.clear();
  }
};


/** Unpack at most \a n trees from the packed parse forest given by \a roots.
 *
 * This is the top-level function in tItem which is not called recursively.
 *
 * \param roots       a set of packed trees
 * \param n           the maximal number of trees to unpack
 * \param end         the rightmost position of the chart
 * \param resources   an object taking care of all resource limits like
 *                    unpacked edges, used memory and time
 *
 * \return a list of the best, at most \a n unpacked trees
 */
void
tItem::selectively_unpack(list<tItem*> roots, int nsolutions, int end,
                          Resources &resources, item_list &results) {
  if (nsolutions <= 0)
    return;

  list<tHypothesis*> ragenda;
  // will release memory upon method exit properly, even in case of uncaught
  // exception
  element_releaser<tHypothesis *> releaser(ragenda);

  tHypothesis* aitem;

  tHypothesis* hypo;
  list<tItem*> path;
  path.push_back(NULL); // root path
  while (path.size() > opt_gplevel)
    path.pop_front();

    for (item_iter it = roots.begin(); it != roots.end(); ++it) {
      tPhrasalItem* root = (tPhrasalItem*)(*it);
      hypo = (*it)->hypothesize_edge(path, 0, resources);

      if (hypo) {
        // Grammar->sm()->score_hypothesis(hypo);
        aitem = new tHypothesis(root, hypo, 0);
        stats.p_hypotheses ++;
        hagenda_insert(ragenda, aitem, path);
      }
      for (item_iter edge = root->packed.begin();
           edge != root->packed.end(); ++edge) {
        // ignore frozen edges
        if (! (*edge)->frozen()) {
          hypo = (*edge)->hypothesize_edge(path, 0, resources);
          if (hypo) {
            //Grammar->sm()->score_hypothesis(hypo);
            aitem = new tHypothesis(*edge, hypo, 0);
            stats.p_hypotheses ++;
            hagenda_insert(ragenda, aitem, path);
          }
        }
      }
    }

    while (! ragenda.empty() && nsolutions > 0) {
      aitem = ragenda.front();
      ragenda.pop_front();
      tItem *result =
        aitem-> edge->instantiate_hypothesis(path, aitem->hypo_dtrs.front(),
                                             resources);
      if (resources.exhausted()) {
        return;
      }
      type_t rule;
      if (result &&
          (result->trait() == PCFG_TRAIT || result->root(Grammar, end, rule))) {
        result->set_result_root(rule);
        results.push_back(result);
        --nsolutions;
        if (nsolutions == 0) {
          delete aitem;
          break;
        }
      }

      hypo = aitem->edge->hypothesize_edge(path, aitem->indices[0]+1,
                                           resources);
      if (hypo) {
        //Grammar->sm()->score_hypothesis(hypo);
        tHypothesis* naitem = new tHypothesis(aitem->edge, hypo,
                                              aitem->indices[0]+1);
        stats.p_hypotheses ++;
        hagenda_insert(ragenda, naitem, path);
      }
      delete aitem;
    }
}


// ****************************************************************************
// Unpacking: Top-level methods for parsing
// ****************************************************************************

/**
 * Unpacks the parse forest selectively (using the max-ent model).
 * This is the top-level method that is not called recursively
 * \return number of unpacked trees
 */
int unpack_selectively(std::vector<tItem*> &trees, int nsolutions,
                       int chart_length, Resources &resources,
                       vector<tItem *> &readings, list<tError> &errors) {
  int nres = 0;

  // selectively unpacking
  list<tItem*> uroots;
  for (vector<tItem*>::iterator tree = trees.begin(); tree != trees.end();
       ++tree) {
    if (! (*tree)->blocked()) {
      stats.trees ++;
      uroots.push_back(*tree);
    }
  }
  list<tItem*> results;
  try {
    tItem::selectively_unpack(uroots, nsolutions, chart_length,
                              resources, results);
  }
  catch (tExhaustedError e) {
    // fail gracefully, that way, all generated full results can be used.
    errors.push_back(e);
  }

  static tCompactDerivationPrinter cdp;
  for (list<tItem*>::iterator res = results.begin();
       res != results.end(); ++res) {
    readings.push_back(*res);
    LOG(logParse, DEBUG,
        "unpacked[" << nres << "] (" << setprecision(1)
        << (float) resources.get_stage_time_ms()
        << "): " << endl
        << (std::pair<tAbstractItemPrinter *, const tItem *>(&cdp, *res))
        << endl);
    nres++;
  }
  return nres;
}


/**
 * Unpacks the parse forest exhaustively.
 * \return number of unpacked trees
 * \todo why is opt_nsolutions not honored here
 */
int unpack_exhaustively(vector<tItem*> &trees, int chart_length,
                        Resources &resources, vector<tItem *> &readings) {
  int nres = 0;

  for(vector<tItem *>::iterator tree = trees.begin();
      tree != trees.end() && ! resources.exhausted();
      ++tree) {
    if(! (*tree)->blocked()) {

      stats.trees++;

      list<tItem *> results;

      results = (*tree)->unpack(resources);

      static tCompactDerivationPrinter cdp;
      for(list<tItem *>::iterator res = results.begin();
          res != results.end(); ++res) {
        type_t rule;
        if((*res)->root(Grammar, chart_length, rule)) {
          (*res)->set_result_root(rule);
          readings.push_back(*res);
          LOG(logParse, DEBUG,
              "unpacked[" << nres << "] (" << setprecision(1)
              << (float) resources.get_stage_time_ms()
              << "): " << endl
              << (std::pair<tAbstractItemPrinter *, const tItem *>(&cdp, *res))
              << endl);
          nres++;
        }
      }
    }
  }
  return nres;
}


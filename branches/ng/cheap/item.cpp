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

/* chart items */

#include "pet-config.h"
#include <sys/param.h>

#include "cheap.h"
#include "fs-chart.h"
#include "item.h"
#include "item-printer.h"
#include "parse.h"
#include "sm.h"
#include "tsdb++.h"
#include "types.h"
#include "utility.h"
#include "dagprinter.h"
#include "settings.h"
#include "configs.h"
#include "logging.h"

#include <sstream>
#include <iostream>
#include <sys/times.h>

using namespace std;

//#define PETDEBUG
//#define PETDEBUGPOS

// 2006/10/01 Yi Zhang <yzhang@coli.uni-sb.de>:
// option for grand-parenting level in MEM-based parse selection
extern unsigned int opt_gplevel;

// defined in parse.cpp
extern int opt_packing;

bool tItem::init_item() {
  tItem::opt_shaping = true;
  reference_opt("opt_shaping",
                "Filter items that would reach beyond the chart",
                tItem::opt_shaping);
  /** word lattice parsing (permissible token paths, cf tPath class) */
  tItem::opt_lattice = false;
  reference_opt("opt_lattice",
                "use the lattice structure specified in the input "
                "to restrict the search space in parsing",
                tItem::opt_lattice);
  opt_gplevel = 0;
  reference_opt("opt_gplevel",
                "determine the level of grandparenting "
                "used in the models for selective unpacking",
                opt_gplevel);
  return opt_lattice;
}

// options managed by the configuration system
bool tItem::opt_shaping, tItem::opt_lattice = tItem::init_item();

// this is a hoax to get the cfrom and cto values into the mrs
#ifdef DYNAMIC_SYMBOLS
struct charz_t {
  list_int *path;
  attr_t attribute;

  charz_t() {
    path = NULL;
    attribute = 0;
  }

  ~charz_t() {
    free_list(path);
  }

  void set(const char *string_path) {
    free_list(path);
    list_int *lpath = path_to_lpath(string_path);
    list_int *last = lpath;
    while(last->next->next != NULL) last = last->next;
    this->path = lpath;
    this->attribute = last->next->val;
    free_list(last->next);
    last->next = NULL;
  }
};

static charz_t cfrom, cto;
static bool charz_init = false;
static bool charz_use = false;

/** Set characterization paths and modlist. */
void init_characterization() {
  const char *cfrom_path = cheap_settings->value("mrs-cfrom-path");
  const char *cto_path = cheap_settings->value("mrs-cto-path");
  if ((cfrom_path != NULL) && (cto_path != NULL)) {
    cfrom.set(cfrom_path);
    cto.set(cto_path);
    charz_init = true;
    charz_use = ! get_opt_string("opt_mrs").empty();
  }
}

void finalize_characterization() {
  // nothing to do
}

bool characterize(fs &thefs, int from, int to) {
  if(charz_use) {
    assert(from >= 0 && to >= 0);
    return thefs.characterize(cfrom.path, cfrom.attribute
                              , retrieve_string_instance(from))
           && thefs.characterize(cto.path, cto.attribute
                              , retrieve_string_instance(to));
  }
  return true;
}
#else
bool characterize(fs, start, end) { return true; }
#endif

item_owner *tItem::_default_owner = NULL;
int tItem::_next_id = 1;

tItem::tItem(int start, int end, const tPaths &paths,
             const fs &f, const char *printname)
    : _id(_next_id++),
      _start(start), _end(end),
      _start_vertex(NULL), _end_vertex(NULL),
      _startposition(-1), _endposition(-1),
      _spanningonly(false), _paths(paths),
      _fs(f), _tofill(0), _nfilled(0), _inflrs_todo(0),
      _result_root(-1), _result_contrib(false),
      _qc_vector_unif(0), _qc_vector_subs(0),
      _score(0.0), _gmscore(0.0), _printname(printname),
      _blocked(0), _unpack_cache(0), parents(), packed(), _chart(0)
{
    if(_default_owner) _default_owner->add(this);
}

tItem::tItem(int start, int end, const tPaths &paths,
             const char *printname)
    : _id(_next_id++),
      _start(start), _end(end),
      _start_vertex(NULL), _end_vertex(NULL),
      _spanningonly(false), _paths(paths),
      _fs(), _tofill(0), _nfilled(0), _inflrs_todo(0),
      _result_root(-1), _result_contrib(false),
      _qc_vector_unif(0), _qc_vector_subs(0),
      _score(0.0), _gmscore(0.0), _printname(printname),
      _blocked(0), _unpack_cache(0), parents(), packed(), _chart(0)
{
    if(_default_owner) _default_owner->add(this);
}

tItem::~tItem()
{
    delete[] _qc_vector_unif;
    delete[] _qc_vector_subs;
    // free_list(_inflrs_todo); // This is now only done in tLexItem
    delete _unpack_cache;
}

void
tItem::set_result_root(type_t rule) {
  set_result_contrib();
  _result_root = rule;
}



/*****************************************************************************
 INPUT ITEM
 *****************************************************************************/

// constructor with start/end nodes and external start/end positions
tInputItem::tInputItem(string id
                       , int start, int end, int startposition, int endposition
                       , string surface, string stem
                       , const tPaths &paths
                       , int token_class
                       , const std::list<int> &infl_rules
                       , const postags &pos
                       , modlist fsmods
                       , const fs &token_fs)
  : tItem(start, end, paths, token_fs, surface.c_str())
    , _input_id(id), _class(token_class), _surface(surface), _stem(stem)
    , _fsmods(fsmods), _postags(pos)
{
  _startposition = startposition;
  _endposition = endposition;
  _trait = INPUT_TRAIT;
  _inflrs_todo = copy_list(infl_rules);
  if (!token_fs.valid()) // i.e. if no token fs has been specified
    recreate_fs(); // create the token fs
}

// constructor with external start/end positions only
tInputItem::tInputItem(string id, int startposition, int endposition
                       , string surface, string stem
                       , const tPaths &paths
                       , int token_class
                       , const std::list<int> &infl_rules
                       , const postags &pos
                       , modlist fsmods)
  : tItem(-1, -1, paths, fs(), surface.c_str())
    , _input_id(id), _class(token_class), _surface(surface), _stem(stem)
    , _fsmods(fsmods), _postags(pos)
{
  _startposition = startposition;
  _endposition = endposition;
  _trait = INPUT_TRAIT;
  _inflrs_todo = copy_list(infl_rules);
  recreate_fs(); // create the token fs
}

// constructor for complex input items
tInputItem::tInputItem(string id, const list< tInputItem * > &dtrs
                       , string stem, int token_class
                       , const std::list<int> &infl_rules
                       , const postags &pos
                       , modlist fsmods)
  : tItem(-1, -1, tPaths(), fs(), "")
    , _input_id(id), _class(token_class), _stem(stem)
    , _fsmods(fsmods), _postags(pos)
{
  _daughters.insert(_daughters.begin(), dtrs.begin(), dtrs.end());
  _startposition = dtrs.front()->startposition();
  _endposition = dtrs.back()->endposition();
  // _printname = "" ;
  // _surface = "" ;
  for(list< tInputItem * >::const_iterator it = dtrs.begin()
        ; it != dtrs.end(); ++it){
    _paths.intersect((*it)->_paths);
  }
  _trait = INPUT_TRAIT;
  _inflrs_todo = copy_list(infl_rules);
  recreate_fs(); // create the token fs
}

void
tInputItem::print_gen(class tAbstractItemPrinter *ip) const {
  ip->real_print(this);
}

std::string tInputItem::get_yield() {
  return orth().c_str();
}

void
tInputItem::daughter_ids(list<int> &ids)
{
  ids.clear();
}

void
tInputItem::collect_children(list<tItem *> &result)
{
  return;
}

grammar_rule *
tInputItem::rule() const
{
    return NULL;
}

void
tInputItem::recreate_fs()
{
    _fs = tChartUtil::create_input_fs(this);
}

string tInputItem::orth() const {
  if (_daughters.empty()) {
    return _surface;
  } else {
    string result = dynamic_cast<tInputItem *>(_daughters.front())->orth();
    for(list<tItem *>::const_iterator it=(++_daughters.begin())
          ; it != _daughters.end(); ++it){
      result += " " + dynamic_cast<tInputItem *>(*it)->orth();
    }
    return result;
  }
}


/** Return generic lexical entries for this input token. If \a onlyfor is
 * non-empty, only those generic entries corresponding to one of those
 * POS tags are postulated. The correspondence is defined in posmapping.
 */
list<lex_stem *>
tInputItem::generics(postags onlyfor)
{
    list<lex_stem *> result;

    list_int *gens = Grammar->generics();

    if(!gens)
        return result;

    LOG(logGenerics, DEBUG,
        "using generics for [" << _start << " - " << _end << "] `"
        << _surface << "':");

    for(; gens != 0; gens = rest(gens))
    {
        int gen = first(gens);

        const char *suffix = cheap_settings->assoc("generic-le-suffixes",
                                                   type_name(gen));
        if(suffix)
          if(_surface.length() <= strlen(suffix) ||
             strcasecmp(suffix,
                        _surface.c_str() + _surface.length() - strlen(suffix))
             != 0)
            continue;

        if(_postags.license(gen) && (onlyfor.empty() || onlyfor.contains(gen)))
        {
          LOG(logGenerics, DEBUG,
              "  ==> " << print_name(gen)
              << " [" << (suffix == 0 ? "*" : suffix)<< "]");

          result.push_back(Grammar->find_stem(gen));
        }
    }

    return result;
}

void tLexItem::init() {
  if (passive()) {
    _supplied_pos = postags(_stem);

    for(item_iter it = _daughters.begin(); it != _daughters.end(); ++it) {
      (*it)->parents.push_back(this);
    }

    //
    // _fix_me_
    // `FIRST' and `REST' should not be hard-wired in the code. (14-sep-08; oe)
    //
    string orth_path = cheap_settings->req_value("orth-path");
    for (int i = 0 ; i < _stem->inflpos(); ++i) orth_path += ".REST";
    orth_path += ".FIRST";

    // Create a feature structure to put the surface form (if there is one)
    // into the right position of the orth list
    if (_keydaughter->form().size() > 0) {
      _mod_form_fs
        = fs(dag_create_path_value(orth_path.c_str()
               , retrieve_string_instance(_keydaughter->form())));
    } else {
      _mod_form_fs = fs(FAIL);
    }

    // Create two feature structures to put the base resp. the surface form
    // (if there is one) into the right position of the orth list
    _mod_stem_fs
      = fs(dag_create_path_value(orth_path.c_str()
             , retrieve_string_instance(_stem->orth(_stem->inflpos()))));

    characterize(_fs_full, _startposition, _endposition);

    // \todo Not nice to overwrite the _fs field.
    // A copy of _fs_full and the containing dag is made
   if(opt_packing)
     _fs = packing_partial_copy(_fs_full, Grammar->packing_restrictor(),
                                false);

    _trait = LEX_TRAIT;
    // \todo Berthold says, this is the right number. Settle this
    // ++stats.words;

    _qc_vector_unif = _fs_full.get_unif_qc_vector();
    _qc_vector_subs = _fs_full.get_subs_qc_vector();

    // compute _score score for lexical items
    if(Grammar->sm())
      score(Grammar->sm()->scoreLeaf(this));

    characterize(_fs, _startposition, _endposition);
  }

#ifdef PETDEBUG
  LOG(logParse, DEBUG, "new lexical item (`" << printname() << "'):" << *this);
#endif
}

//
// _fix_me_
// i doubt the _ldot and _rdot logic here.  currently, lex_stem has hardwired
// inflpos() to the last position, i.e. for an MWE with two tokens (Palo Alto),
// inflpos() will be 1; for non-MWEs, it will be 0.  hence, all active lexical
// items are effectively left-extending, and _rdot never gets to play a role.
// yet, i believe it should be initialized to 1, for right-extending items, not
// inflpos() + 1, as it used to be.                             (16-jan-09; oe)
//
tLexItem::tLexItem(lex_stem *stem, tInputItem *i_item
                   , fs &f, const list_int *inflrs_todo)
  : tItem(i_item->start(), i_item->end(), i_item->paths()
          , f, stem->printname())
  , _ldot(stem->inflpos())
  , _stem(stem), _fs_full(f), _hypo(NULL)
{
  _rdot = (_ldot ? stem->length() : 1);
  _startposition = i_item->startposition();
  _endposition = i_item->endposition();
  _inflrs_todo = copy_list(inflrs_todo);
  _key_item = this;
  _daughters.push_back(i_item);
  _keydaughter = i_item;
  init();
}

tLexItem::tLexItem(tLexItem *from, tInputItem *newdtr, fs f)
  : tItem(-1, -1, from->paths().common(newdtr->paths()),
          (f.valid() ? f : from->get_fs()), from->printname()),
  _ldot(from->_ldot), _rdot(from->_rdot), _keydaughter(from->_keydaughter),
  _stem(from->_stem), _fs_full(f.valid() ? f : from->get_fs()), _hypo(NULL)
{
  _daughters = from->_daughters;
  _inflrs_todo = copy_list(from->_inflrs_todo);
  _key_item = this;
  if(from->left_extending()) {
    _start = newdtr->start();
    _startposition = newdtr->startposition();
    _end = from->end();
    _endposition = from->endposition();
    _daughters.push_front(newdtr);
    --_ldot;
    // register this expansion to avoid duplicates
    from->_expanded.push_back(_start);
  } else {
    _start = from->start();
    _startposition = from->startposition();
    _end = newdtr->end();
    _endposition = newdtr->endposition();
    _daughters.push_back(newdtr);
    ++_rdot;
    from->_expanded.push_back(_end);
  }
  init();
}

tPhrasalItem::tPhrasalItem(grammar_rule *R, tItem *pasv, fs &f)
  : tItem(pasv->start(), pasv->end(), pasv->paths(), f, R->printname()),
    _adaughter(0), _rule(R) {
  _startposition = pasv->startposition();
  _endposition = pasv->endposition();
  _tofill = R->restargs();
  _nfilled = 1;
  _daughters.push_back(pasv);
  _key_item = pasv->_key_item;

  if(R->trait() == INFL_TRAIT) {
    // We don't copy here, so only the tLexItem is responsible for deleting
    // the list
    _inflrs_todo = rest(pasv->_inflrs_todo);
    // output of morphographemic rules (rules with INFL_TRAIT) gets LEX_TRAIT
    _trait = LEX_TRAIT;
    // Modify the feature structure to contain the surface form in the
    // right place
    if(inflrs_complete_p()) {
      _fs.modify_eagerly(_key_item->_mod_form_fs);
    } else {
      _fs.modify_eagerly(_key_item->_mod_stem_fs);
    }
  } else {
    _inflrs_todo = pasv->_inflrs_todo;
    _trait = R->trait();
  }

  _spanningonly = R->spanningonly();

#ifdef PETDEBUG
  LOG(logParse, DEBUG, "A " << pasv->id() << " < " << id());
#endif
  pasv->parents.push_back(this);

  // rule stuff + characterization
  if(passive()) {
    _qc_vector_unif = f.get_unif_qc_vector();
    _qc_vector_subs = f.get_subs_qc_vector();
    R->passives++;
    characterize(_fs, _startposition, _endposition);
  } else {
    _qc_vector_unif = f.get_unif_qc_vector(nextarg());
    R->actives++;
  }

#ifdef PETDEBUG
  LOG(logParse, DEBUG, "new rule tItem (`" << R->printname() << "' + "
      << pasv->id() << "@" << R->nextarg() << "):" << *this);
#endif
}

tPhrasalItem::tPhrasalItem(tPhrasalItem *active, tItem *pasv, fs &f)
  : tItem(-1, -1, active->paths().common(pasv->paths()),
          f, active->printname()),
    _adaughter(active), _rule(active->_rule)
{
    _daughters = active->_daughters;
    if(active->left_extending())
    {
      _start = pasv->start();
      _startposition = pasv->startposition();
      _end = active->end();
      _endposition = active->endposition();
      _daughters.push_front(pasv);
    }
    else
    {
      _start = active->start();
      _startposition = active->startposition();
      _end = pasv->end();
      _endposition = pasv->endposition();
      _daughters.push_back(pasv);
    }

#ifdef PETDEBUG
    LOG(logParse, DEBUG,
        "A " << pasv->id() << " " <<  active->id() << " < " << id());
#endif
    pasv->parents.push_back(this);
    active->parents.push_back(this);

    _tofill = active->restargs();
    _nfilled = active->nfilled() + 1;

    _trait = SYNTAX_TRAIT;

    // rule stuff
    if(passive()) {
      _qc_vector_unif = f.get_unif_qc_vector();
      _qc_vector_subs = f.get_subs_qc_vector();
      characterize(_fs, _startposition, _endposition);
      active->rule()->passives++;
    } else {
      _qc_vector_unif = f.get_unif_qc_vector(nextarg());
      active->rule()->actives++;
    }

#ifdef PETDEBUG
    LOG(logParse, DEBUG, "new combined item (" << active->id() << " + "
        << pasv->id() << "@" << active->nextarg() << "):" << *this);
#endif
}

tPhrasalItem::tPhrasalItem(tPhrasalItem *sponsor, vector<tItem *> &dtrs, fs &f)
    : tItem(sponsor->start(), sponsor->end(), sponsor->_paths,
           f, sponsor->printname()),
      _adaughter(0), _rule(sponsor->rule())
{
    // _fix_me_
    //  copy(dtrs.begin(), dtrs.end(), _daughters.begin());
    for(vector<tItem *>::iterator it = dtrs.begin(); it != dtrs.end(); ++it)
        _daughters.push_back(*it);

    _trait = SYNTAX_TRAIT;
    _nfilled = dtrs.size();
    _result_root = sponsor->result_root();
}


tPhrasalItem::tPhrasalItem(grammar_rule *rule, tItem *pasv)
  : tItem(pasv->_start, pasv->_end, pasv->_paths, rule->printname()),
    _adaughter(0), _rule(rule) {
  _startposition = pasv->startposition();
  _endposition = pasv->endposition();
  _tofill = rule->restargs();
  _nfilled = 1;
  _daughters.push_back(pasv);
  _key_item = pasv->_key_item;

  _trait = PCFG_TRAIT;

  _spanningonly = rule->spanningonly();

  pasv->parents.push_back(this);
  if (passive()) {
    rule->passives++;
  }
}

tPhrasalItem::tPhrasalItem(tPhrasalItem *active, tItem *pasv)
  : tItem(-1, -1, active->_paths.common(pasv->_paths), active->printname()),
    _adaughter(active), _rule(active->_rule) {
  _daughters = active->_daughters;
  _start = active->_start;
  _startposition = active->startposition();
  _end = pasv->_end;
  _endposition = pasv->endposition();
  _daughters.push_back(pasv);

  pasv->parents.push_back(this);
  active->parents.push_back(this);

  _tofill = active->restargs();
  _nfilled = active->nfilled() + 1;

  _trait = PCFG_TRAIT;

  if (passive()) {
    active->rule()->passives++;
  } else {
    active->rule()->actives++;
  }
}

tPhrasalItem::tPhrasalItem(tPhrasalItem *sponsor, std::vector<tItem *> &dtrs)
  : tItem(sponsor->start(), sponsor->end(), sponsor->_paths, sponsor->printname()),
    _adaughter(0), _rule(sponsor->rule()) {
  for(vector<tItem *>::iterator it = dtrs.begin(); it != dtrs.end(); ++it)
    _daughters.push_back(*it);
  _trait = PCFG_TRAIT;
  _nfilled = dtrs.size();
  _result_root = sponsor->result_root();
}

void
tPhrasalItem::set_result_root(type_t rule)
{
    if(result_contrib() == false)
    {
        for(list<tItem *>::iterator pos = _daughters.begin();
            pos != _daughters.end();
            ++pos)
            (*pos)->set_result_contrib();

        if(_adaughter)
            _adaughter->set_result_contrib();
    }

    set_result_contrib();
    _result_root = rule;
}


void
tLexItem::print_gen(class tAbstractItemPrinter *ip) const
{
  ip->real_print(this);
}

string
tLexItem::description() const
{
    if(_daughters.empty()) return string();
    return _keydaughter->description();
}

string
tLexItem::orth() const
{
    string orth = dynamic_cast<const tInputItem *>(_daughters.front())->orth();
    for(item_citer it=(++_daughters.begin())
          ; it != _daughters.end(); ++it){
      orth += " " + dynamic_cast<const tInputItem *>(*it)->orth();
    }
    return orth;
}

void
tPhrasalItem::print_gen(class tAbstractItemPrinter *ip) const {
  ip->real_print(this);
}

std::string tLexItem::get_yield() {
  return orth().c_str();
}

void
tLexItem::daughter_ids(list<int> &ids)
{
    ids.clear();
}

void
tLexItem::collect_children(list<tItem *> &result)
{
    if(blocked())
        return;
    frost();
    result.push_back(this);
}

std::string
tPhrasalItem::get_yield() {
  std::string result;
  item_iter pos = _daughters.begin();
  if (pos != _daughters.end()) { result += (*pos)->get_yield(); ++pos; }
  for(; pos != _daughters.end(); ++pos) {
    result += " " ;
    result += (*pos)->get_yield();
  }
  return result;
}

void
tPhrasalItem::daughter_ids(list<int> &ids)
{
    ids.clear();

    for(item_citer pos = _daughters.begin(); pos != _daughters.end(); ++pos)
    {
        ids.push_back((*pos)->id());
    }
}

void
tPhrasalItem::collect_children(item_list &result)
{
    if(blocked())
        return;
    frost();
    result.push_back(this);

    for(item_iter pos = _daughters.begin(); pos != _daughters.end(); ++pos)
    {
        (*pos)->collect_children(result);
    }
}

grammar_rule *
tLexItem::rule() const
{
    return NULL;
}

grammar_rule *
tPhrasalItem::rule() const
{
    return _rule;
}

void
tLexItem::recreate_fs()
{
    throw tError("cannot rebuild lexical item's feature structure");
}

void tPhrasalItem::recreate_fs()
{
    if(!passive())
    {
        assert(_rule->arity() <= 2);
        _fs = _rule->instantiate();
        fs arg = _rule->nextarg(_fs);
        _fs = unify_np(_fs, _daughters.front()->get_fs(), arg);
        if(!_fs.valid())
            throw tError("trouble rebuilding active item (1)");
    }
    else
    {
        throw tError("won't rebuild passive item");
    }
#ifdef PETDEBUG
    {
        temporary_generation t(_fs.temp());
        LOG(logParse, DEBUG, "recreated fs of " << *this)
    }
#endif
}

bool
tItem::contains_p(const tItem *it) const
{
  const tItem *pit = this;
  while (true) {
    if (it->startposition() != pit->startposition() ||
        it->endposition() != pit->endposition())
      return false;
    else if (it->id() == pit->id())
      return true;
    else if (pit->_daughters.size() != 1)
      return false;
    else
      pit = pit->daughters().front();
  }
}


/** check if packing current item into \a it
    will create a cycle in the packed forest */
bool
tItem::cyclic_p(const tItem *it) const {
  list<const tItem*> ilist;
  const tItem *pit = this;
  ilist.push_back(pit);
  while (!ilist.empty()) {
    pit = ilist.front();
    ilist.pop_front();
    if (it->startposition() != pit->startposition() ||
        it->endposition() != pit->endposition() ||
        pit->_daughters.size() != 1)
      continue;
    else if (it->id() == pit->id())
      return true;
    else {
      pit = pit->daughters().front();
      ilist.push_back(pit);
      for (item_citer iter = pit->packed.begin();
           iter != pit->packed.end();
           ++iter)
        ilist.push_back(*iter);
    }
  }
  return false;
}

//
// Blocking (frosting and freezing) for packing
//

void
tItem::block(int mark, bool freeze_parents)
{
  LOG(logPack, DEBUG,
      (mark == 1 ? "frost" : "freez") << "ing" << endl << *this) ;
  if (!blocked() || mark == 2) {
    if (mark == 2)
      stats.p_frozen++;

    _blocked = mark;
  }
  if (freeze_parents) {
    for (item_iter p = parents.begin(); p != parents.end(); ++p) {
      (*p)->freeze();
    }
  }
}


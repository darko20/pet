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

#include "pet-system.h"
#include "cheap.h"
#include "utility.h"
#include "types.h"
#include "item.h"
#include "item-printer.h"
#include "parse.h"
#include "tsdb++.h"
#include "sm.h"

//#define DEBUG
//#define DEBUGPOS

// this is a hoax to get the cfrom and cto values into the mrs
#ifdef DYNAMIC_SYMBOLS
modlist characterize_modlist;

/** Set characterization paths and modlist. */
void init_characterization() {
  characterize_modlist.clear();
  char *cfrom_path = cheap_settings->value("mrs-cfrom-path");
  char *cto_path = cheap_settings->value("mrs-cto-path");
  if ((cfrom_path != NULL) && (cto_path != NULL)) {
    characterize_modlist.push_front(pair<string, int>(cfrom_path, 0));
    characterize_modlist.push_back(pair<string, int>(cto_path, 0));
  }
}

inline bool characterize_mods() { return ! characterize_modlist.empty(); }

modlist &characterize_mods(unsigned int from, unsigned int to) {
  characterize_modlist.front().second = lookup_unsigned_symbol(from);
  characterize_modlist.back().second = lookup_unsigned_symbol(to);
  return characterize_modlist;
}
#endif

item_owner *tItem::_default_owner = 0;
int tItem::_next_id = 1;

tItem::tItem(int start, int end, const tPaths &paths,
             const fs &f, const char *printname)
    : _id(_next_id++),
      _start(start), _end(end), _startposition(-1), _endposition(-1),
      _spanningonly(false), _paths(paths),
      _fs(f), _tofill(0), _nfilled(0), _inflrs_todo(0),
      _result_root(-1), _result_contrib(false),
      _qc_vector_unif(0), _qc_vector_subs(0),
      _score(0.0), _printname(printname),
      _blocked(0), _unpack_cache(0), parents(), packed()
{
    if(_default_owner) _default_owner->add(this);
}

tItem::tItem(int start, int end, const tPaths &paths,
             const char *printname)
    : _id(_next_id++),
      _start(start), _end(end), _spanningonly(false), _paths(paths),
      _fs(), _tofill(0), _nfilled(0), _inflrs_todo(0),
      _result_root(-1), _result_contrib(false),
      _qc_vector_unif(0), _qc_vector_subs(0),
      _score(0.0), _printname(printname),
      _blocked(0), _unpack_cache(0), parents(), packed()
{
    if(_default_owner) _default_owner->add(this);
}

tItem::~tItem()
{
    delete[] _qc_vector_unif;
    delete[] _qc_vector_subs;
    free_list(_inflrs_todo);
    delete _unpack_cache;
}

/*****************************************************************************
 INPUT ITEM
 *****************************************************************************/

tInputItem::tInputItem(string id, int start, int end, string surface
                       , string stem, const tPaths &paths, int token_class
                       , modlist fsmods)
  : tItem(-1, -1, paths, surface.c_str())
    , _input_id(id), _class(token_class), _surface(surface), _stem(stem)
    , _fsmods(fsmods)
{
  _startposition = start;
  _endposition = end;
  _trait = INPUT_TRAIT;
}


tInputItem::tInputItem(string id, const list< tInputItem * > &dtrs
                       , string stem, int token_class, modlist fsmods)
  : tItem(-1, -1, tPaths(), "")
    , _input_id(id), _class(token_class), _stem(stem)
    , _fsmods(fsmods)
{
  _daughters.insert(_daughters.begin(), dtrs.begin(), dtrs.end());
  _startposition = dtrs.front()->startposition();
  _endposition = dtrs.back()->endposition();
  // _printname = "" ;
  // _surface = "" ;
  for(list< tInputItem * >::const_iterator it = dtrs.begin()
        ; it != dtrs.end(); it++){
    _paths.intersect((*it)->_paths);
  }
  _trait = INPUT_TRAIT;
}
  
void tInputItem::print(FILE *f, bool compact)
{
  fprintf(f, "[%d - %d] (%s) \"%s\" \"%s\" "
          , _startposition, _endposition, _input_id.c_str()
          , _stem.c_str(), _surface.c_str());

  list_int *li = _inflrs_todo;
  while(li != NULL) {
    fprintf(f, "+%s", type_name(first(li)));
    li = rest(li);
  }

  // fprintf(f, "@%d", inflpos);

  fprintf(ferr, " {");
  _postags.print(ferr);
  fprintf(ferr, "}");
}

void
tInputItem::print_gen(class tItemPrinter *ip) const
{
  ip->real_print(this);
}

void
tInputItem::print_derivation(FILE *f, bool quoted) {
    fprintf (f, "(%s\"%s%s\" %.2f %d %d)",
             quoted ? "\\" : "", orth().c_str(), quoted ? "\\" : "", score(),
             _start, _end);

    fprintf(f, ")");
}

void 
tInputItem::print_yield(FILE *f)
{
  fprintf(f, "%s ", orth().c_str());
}

string
tInputItem::tsdb_derivation(int protocolversion)
{
    ostringstream res;
  
    res << "(\"" << orth() << "\" " << _start << " " << _end << "))";

    return res.str();
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

void
tInputItem::set_result_root(type_t rule)
{
    set_result_contrib();
    _result_root = rule;
}

grammar_rule *
tInputItem::rule()
{
    return NULL;
}

void
tInputItem::recreate_fs()
{
    throw tError("cannot rebuild input item's feature structure");
}

/** \brief Since tInputItems do not have a feature structure, they need not be
 *  unpacked. Unpacking does not proceed past tLexItem 
 */
list<tItem *>
tInputItem::unpack1(int limit)
{
    list<tItem *> res;
    res.push_back(this);
    return res;
}

string tInputItem::orth() const {
  if (_daughters.empty()) {
    return _surface;
  } else {
    string result = dynamic_cast<tInputItem *>(_daughters.front())->orth();
    for(list<tItem *>::const_iterator it=(++_daughters.begin())
          ; it != _daughters.end(); it++){
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

    if(verbosity > 4)
        fprintf(ferr, "using generics for [%d - %d] `%s':\n", 
                _start, _end, _surface.c_str());

    for(; gens != 0; gens = rest(gens))
    {
        int gen = first(gens);

        char *suffix = cheap_settings->assoc("generic-le-suffixes",
                                             type_name(gen));
        if(suffix)
          if(_surface.length() <= strlen(suffix) ||
             strcasecmp(suffix,
                        _surface.c_str() + _surface.length() - strlen(suffix))
             != 0)
            continue;

        if(_postags.license(gen) && (onlyfor.empty() || onlyfor.contains(gen)))
        {
          if(verbosity > 4)
              fprintf(ferr, "  ==> %s [%s]\n", print_name(gen),
                      suffix == 0 ? "*" : suffix);
	  
          result.push_back(Grammar->find_stem(gen));
        }
    }

    return result;
}

void tLexItem::init(fs &f) {
  if (passive()) {
    _supplied_pos = postags(_stem);

    for(item_iter it = _daughters.begin(); it != _daughters.end(); it++) {
      (*it)->parents.push_back(this);
    }

    // _fix_me_
    // Not nice to overwrite the _fs field.
    if(opt_packing)
      _fs = packing_partial_copy(f, Grammar->packing_restrictor(), false);

    if(_inflrs_todo)
      _trait = INFL_TRAIT;
    else
      _trait = LEX_TRAIT;
    
    if(opt_nqc_unif != 0)
      _qc_vector_unif = get_qc_vector(qc_paths_unif, qc_len_unif, f);

    if(opt_nqc_subs != 0)
      _qc_vector_subs = get_qc_vector(qc_paths_subs, qc_len_subs, f);

    // compute _score score for lexical items
    if(Grammar->sm())
      score(Grammar->sm()->scoreLeaf(this));

#ifdef DYNAMIC_SYMBOLS
    if((opt_mrs != 0) && (characterize_mods())) {
      assert(_startposition >= 0 && _endposition >= 0);
      _fs.modify_eagerly_searching(characterize_mods(_startposition
                                                   , _endposition));
    }
#endif

#if 0
#ifdef YY
    if(opt_k2y)
      {
        // _fix_me_ this should be done in some other place, maybe added to the
        // mods in tokenization?
        mrs_stamp_fs(_fs, _id);

        for(list<tItem *>::iterator it=_daughters.begin()
              ; it != _daughters.end(); it++)
          mrs_map_id(_id, (*it)->id());

        // _fix_me_ could somebody please write a comment for this
        set<string> senses = cheap_settings->smap("type-to-sense", _fs.type());
        for(set<string>::iterator it = senses.begin(); it != senses.end();
            ++it)
          mrs_map_sense(_id, *it);
      }
#endif
#endif

#ifdef DEBUG
    fprintf(ferr, "new lexical item (`%s[%s]'):", 
            le->printname(), le->affixprintname());
    print(ferr);
    fprintf(ferr, "\n");
#endif
  }
}

tLexItem::tLexItem(lex_stem *stem, tInputItem *i_item
                   , fs &f, const list_int *inflrs_todo)
  : tItem(i_item->start(), i_item->end(), i_item->_paths
          , f, stem->printname())
  , _ldot(stem->inflpos()), _rdot(stem->inflpos() + 1)
  , _stem(stem), _fs_full(f)
{
  _startposition = i_item->startposition();
  _endposition = i_item->endposition();
  _inflrs_todo = copy_list(inflrs_todo);
  _daughters.push_back(i_item);
  _keydaughter = i_item;
  init(f);
}

tLexItem::tLexItem(tLexItem *from, tInputItem *newdtr)
  : tItem(-1, -1, from->_paths.common(newdtr->_paths)
          , from->get_fs(), from->printname())
    , _ldot(from->_ldot), _rdot(from->_rdot)
    , _keydaughter(from->_keydaughter)
    , _stem(from->_stem), _fs_full(from->get_fs())
{
  _daughters = from->_daughters;
  _inflrs_todo = copy_list(from->_inflrs_todo);
  if(from->left_extending()) {
    _start = newdtr->start();
    _startposition = newdtr->startposition();
    _end = from->end();
    _endposition = from->endposition();
    _daughters.push_front(newdtr);
    _ldot--;
    // register this expansion to avoid duplicates
    from->_expanded.push_back(_start);
  } else {
    _start = from->start();
    _startposition = from->startposition();
    _end = newdtr->end();
    _endposition = newdtr->endposition();
    _daughters.push_back(newdtr);
    _rdot++;
    from->_expanded.push_back(_end);
  }
  init(_fs);
}

#if 0
bool
same_lexitems(const tLexItem &a, const tLexItem &b)
{
    if(a.start() != b.start() || a.end() != b.end())
        return false;

    return a._dtrs[a._keydtr]->form() == b._dtrs[b._keydtr]->form();
}
#endif

tPhrasalItem::tPhrasalItem(grammar_rule *R, tItem *pasv, fs &f)
    : tItem(pasv->_start, pasv->_end, pasv->_paths,
           f, R->printname()),
      _adaughter(0), _rule(R)
{
    _startposition = pasv->startposition();
    _endposition = pasv->endposition();
    _tofill = R->restargs();
    _nfilled = 1;
    _daughters.push_back(pasv);

    _trait = R->trait();
    if(_trait == INFL_TRAIT)
    {
        _inflrs_todo = copy_list(rest(pasv->_inflrs_todo));
        if(_inflrs_todo == 0)
            _trait = LEX_TRAIT;
    }
  
    _spanningonly = R->spanningonly();

#ifdef DEBUG
    fprintf(stderr, "A %d < %d\n", pasv->id(), id());
#endif
    pasv->parents.push_back(this);

    if(opt_nqc_unif != 0)
    {
        if(passive())
            _qc_vector_unif = get_qc_vector(qc_paths_unif, qc_len_unif, f);
        else
            _qc_vector_unif = get_qc_vector(qc_paths_unif, qc_len_unif, 
                                            nextarg(f));
    }

    if(opt_nqc_subs != 0)
        if(passive())
            _qc_vector_subs = get_qc_vector(qc_paths_subs, qc_len_subs, f);

#ifdef DYNAMIC_SYMBOLS
    if((opt_mrs != 0) && (characterize_mods()))
      if(passive()) {
        assert(_startposition >= 0 && _endposition >= 0);
        _fs.modify_eagerly_searching(characterize_mods(_startposition
                                                       , _endposition));
      }
#endif

    // rule stuff
    if(passive())
        R->passives++;
    else
        R->actives++;

#ifdef DEBUG
    fprintf(ferr, "new rule tItem (`%s' + %d@%d):", 
            R->printname(), pasv->id(), R->nextarg());
    print(ferr);
    fprintf(ferr, "\n");
#endif
}

tPhrasalItem::tPhrasalItem(tPhrasalItem *active, tItem *pasv, fs &f)
    : tItem(-1, -1, active->_paths.common(pasv->_paths),
           f, active->printname()),
      _adaughter(active), _rule(active->_rule)
{
    _daughters = active->_daughters;
    if(active->left_extending())
    {
        _start = pasv->_start;
        _startposition = pasv->startposition();
        _end = active->_end;
        _endposition = active->endposition();
        _daughters.push_front(pasv);
    }
    else
    {
        _start = active->_start;
        _startposition = active->startposition();
        _end = pasv->_end;
        _endposition = pasv->endposition();
        _daughters.push_back(pasv);
    }
  
#ifdef DEBUG
    fprintf(stderr, "A %d %d < %d\n", pasv->id(), active->id(), id());
#endif
    pasv->parents.push_back(this);
    active->parents.push_back(this);

    _tofill = active->restargs();
    _nfilled = active->nfilled() + 1;

    _trait = SYNTAX_TRAIT;

    if(opt_nqc_unif != 0)
    {
        if(passive())
            _qc_vector_unif = get_qc_vector(qc_paths_unif, qc_len_unif, f);
        else
            _qc_vector_unif = get_qc_vector(qc_paths_unif, qc_len_unif, 
                                            nextarg(f));
    }
    
    if(opt_nqc_subs != 0)
        if(passive())
            _qc_vector_subs = get_qc_vector(qc_paths_subs, qc_len_subs, f);

#ifdef DYNAMIC_SYMBOLS
    if((opt_mrs != 0) && (characterize_mods()))
      if(passive()) {
        assert(_startposition >= 0 && _endposition >= 0);
        _fs.modify_eagerly_searching(characterize_mods(_startposition
                                                       , _endposition));
      }
#endif

    // rule stuff
    if(passive())
        active->rule()->passives++;
    else
        active->rule()->actives++;

#ifdef DEBUG
    fprintf(ferr, "new combined item (%d + %d@%d):", 
            active->id(), pasv->id(), active->nextarg());
    print(ferr);
    fprintf(ferr, "\n");
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
}

void
tLexItem::set_result_root(type_t rule)
{
    set_result_contrib();
    _result_root = rule;
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
tItem::print(FILE *f, bool compact)
{
    fprintf(f, "[%d %d-%d %s (%d) ", _id, _start, _end, _fs.printname(),
            _trait);

    fprintf(f, "%.4g", _score);

    fprintf(f, " {");

    list_int *l = _tofill;
    while(l)
    {
        fprintf(f, "%d ", first(l));
        l = rest(l);
    }

    fprintf(f, "} {");

    l = _inflrs_todo;
    while(l)
    {
        fprintf(f, "%s ", print_name(first(l)));
        l = rest(l);
    }

    fprintf(f, "} {");

    list<int> paths = _paths.get();
    for(list<int>::iterator it = paths.begin(); it != paths.end(); ++it)
    {
        fprintf(f, "%s%d", it == paths.begin() ? "" : " ", *it);
    }
  
    fprintf(f, "}]");

    print_family(f);
    print_packed(f);

    if(verbosity > 2 && compact == false)
    {
        print_derivation(f, false);
    }
}

void
tLexItem::print(FILE *f, bool compact)
{
    fprintf(f, "L ");
    tItem::print(f);
    if(verbosity > 10 && compact == false)
    {
        fprintf(f, "\n");
        _fs.print(f);
    }
}

void
tLexItem::print_gen(class tItemPrinter *ip) const
{
  ip->real_print(this);
}

string
tLexItem::description()
{
    if(_daughters.empty()) return string();
    return _keydaughter->description();
}

string
tLexItem::orth()
{
    string orth = dynamic_cast<tInputItem *>(_daughters.front())->orth();
    for(list<tItem *>::iterator it=(++_daughters.begin())
          ; it != _daughters.end(); it++){
      orth += " " + dynamic_cast<tInputItem *>(*it)->orth();
    }
    return orth;
}

void
tPhrasalItem::print(FILE *f, bool compact)
{
    fprintf(f, "P ");
    tItem::print(f);

    if(verbosity > 10 && compact == false)
    {
        fprintf(f, "\n");
        _fs.print(f);
    }
}

void
tPhrasalItem::print_gen(class tItemPrinter *ip) const
{
  ip->real_print(this);
}

void
tItem::print_packed(FILE *f)
{
    if(packed.size() == 0)
        return;

    fprintf(f, " < packed: ");
    for(list<tItem *>::iterator pos = packed.begin();
        pos != packed.end(); ++pos)
        fprintf(f, "%d ",(*pos)->_id);
    fprintf(f, ">");
}

void
tItem::print_family(FILE *f)
{
    fprintf(f, " < dtrs: ");
    for(list<tItem *>::iterator pos = _daughters.begin();
        pos != _daughters.end(); ++pos)
        fprintf(f, "%d ",(*pos)->_id);
    fprintf(f, " parents: ");
    for(list<tItem *>::iterator pos = parents.begin();
        pos != parents.end(); ++pos)
        fprintf(f, "%d ",(*pos)->_id);
    fprintf(f, ">");
}

static int derivation_indentation = 0; // not elegant

void
tLexItem::print_derivation(FILE *f, bool quoted)
{
    if(derivation_indentation == 0)
        fprintf(f, "\n");
    else
        fprintf(f, "%*s", derivation_indentation, "");

    //_dtrs[_keydtr]->print_derivation(f, quoted, _id, type(), score(),
    //                                 _inflrs_todo, orth());

    fprintf (f, "(%d %s/%s %.2f %d %d ", _id, _stem->printname(),
             print_name(type()), score(), _start, _end);

    fprintf(f, "[");
    for(list_int *l = _inflrs_todo; l != 0; l = rest(l))
        fprintf(f, "%s%s", print_name(first(l)), rest(l) == 0 ? "" : " ");
    fprintf(f, "] ");

    _keydaughter->print_derivation(f, quoted);
}

void
tLexItem::print_yield(FILE *f)
{
  fprintf(f, "%s ", orth().c_str());
}

string
tLexItem::tsdb_derivation(int protocolversion)
{
    ostringstream res;
  
    res << "(" << _id << " " << _stem->printname()
        << " " << score() << " " << _start <<  " " << _end
        << " " << _keydaughter->tsdb_derivation(protocolversion);

    return res.str();
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

void
tPhrasalItem::print_derivation(FILE *f, bool quoted)
{
    if(derivation_indentation == 0)
        fprintf(f, "\n");
    else
        fprintf(f, "%*s", derivation_indentation, "");

    fprintf(f, 
            "(%d %s %.2f %d %d", 
            _id, printname(), _score, _start, _end);

    if(packed.size())
    {
        fprintf(f, " {");
        for(list<tItem *>::iterator pack = packed.begin();
            pack != packed.end(); ++pack)
        {
            fprintf(f, "%s%d", pack == packed.begin() ? "" : " ", (*pack)->id()); 
        }
        fprintf(f, "}");
    }

    if(_result_root != -1)
    {
        fprintf(f, " [%s]", print_name(_result_root));
    }
  
    if(_inflrs_todo)
    {
        fprintf(f, " [");
        for(list_int *l = _inflrs_todo; l != 0; l = rest(l))
        {
            fprintf(f, "%s%s", print_name(first(l)), rest(l) == 0 ? "" : " ");
        }
        fprintf(f, "]");
    }

    derivation_indentation+=2;
    for(list<tItem *>::iterator pos = _daughters.begin();
        pos != _daughters.end(); ++pos)
    {
        fprintf(f, "\n");
        (*pos)->print_derivation(f, quoted);
    }
    derivation_indentation-=2;

    fprintf(f, ")");
}

void
tPhrasalItem::print_yield(FILE *f)
{
    for(list<tItem *>::iterator pos = _daughters.begin();
        pos != _daughters.end(); ++pos)
    {
        (*pos)->print_yield(f);
    }
}

string
tPhrasalItem::tsdb_derivation(int protocolversion)
{
    ostringstream result;
    
    result << "(" << _id << " " << printname() << " " << _score
           << " " << _start << " " << _end;

    for(list<tItem *>::iterator pos = _daughters.begin();
        pos != _daughters.end(); ++pos)
    {
        result << " ";
        if(protocolversion == 1)
            result << (*pos)->tsdb_derivation(protocolversion);
        else
            result << (*pos)->id();
    }

    result << ")";

    return result.str();
}

void
tPhrasalItem::daughter_ids(list<int> &ids)
{
    ids.clear();

    for(list<tItem *>::iterator pos = _daughters.begin();
        pos != _daughters.end(); ++pos)
    {
        ids.push_back((*pos)->id());
    }
}

void 
tPhrasalItem::collect_children(list<tItem *> &result)
{
    if(blocked())
        return;
    frost();
    result.push_back(this);
    
    for(list<tItem *>::iterator pos = _daughters.begin();
        pos != _daughters.end(); ++pos)
    {
        (*pos)->collect_children(result);
    }
}

grammar_rule *
tLexItem::rule()
{
    return NULL;
}

grammar_rule *
tPhrasalItem::rule()
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
#ifdef DEBUG
    {
        temporary_generation t(_fs.temp());
        fprintf(ferr, "recreated fs of ");
        print(ferr, false);
        fprintf(ferr, "\n");
    }
#endif
}

//
// Blocking (frosting and freezing) for packing
//

void
tItem::block(int mark)
{
    if(verbosity > 4)
    {
        fprintf(ferr, "%sing ", mark == 1 ? "frost" : "freez");
        print(ferr);
        fprintf(ferr, "\n");
    }
    if(!blocked() || mark == 2)
    {
        if(mark == 2) 
            stats.p_frozen++;

        _blocked = mark;
    }  

    for(list<tItem *>::iterator parent = parents.begin();
        parent != parents.end(); ++parent)
    {
      (*parent)->freeze();
    }
}

//
// Unpacking
//

// for printing debugging output
int unpacking_level;

list<tItem *>
tItem::unpack(int upedgelimit)
{
    list<tItem *> res;

    unpacking_level++;
    if(verbosity > 3)
        fprintf(stderr, "%*s> unpack [%d]\n", unpacking_level * 2, "", id());

    // Ignore frozen items.
    if(frozen())
    {
        if(verbosity > 3)
            fprintf(stderr, "%*s< unpack [%d] ( )\n", unpacking_level * 2, "", id());
        unpacking_level--;
        return res;
    }

    if(_unpack_cache)
    {
        // This item has already been unpacked
        unpacking_level--;
        return *_unpack_cache;
    }

    // Check if we reached the unpack edge limit. Caller is responsible for
    // checking this to verify completeness of results.
    if(upedgelimit > 0 && stats.p_upedges >= upedgelimit)
        return res;

    // Recursively unpack items that are packed into this item.
    for(list<tItem *>::iterator p = packed.begin();
        p != packed.end(); ++p)
    {
        // Append result of unpack_item on packed item.
        list<tItem *> tmp = (*p)->unpack(upedgelimit);
        res.splice(res.begin(), tmp);
    }

    // Unpack children.
    list<tItem *> tmp = unpack1(upedgelimit);
    res.splice(res.begin(), tmp);

    if(verbosity > 3)
    {
        fprintf(stderr, "%*s< unpack [%d] ( ", unpacking_level * 2, "", id());
        for(list<tItem *>::iterator i = res.begin(); i != res.end(); ++i)
            fprintf(stderr, "%d ", (*i)->id());
        fprintf(stderr, ")\n");
    }

    _unpack_cache = new list<tItem *>(res);

    unpacking_level--;
    return res;
}

list<tItem *>
tLexItem::unpack1(int limit)
{
    list<tItem *> res;
    res.push_back(this);
    return res;
}

list<tItem *>
tPhrasalItem::unpack1(int upedgelimit)
{
    // Collect expansions for each daughter.
    vector<list<tItem *> > dtrs;
    for(list<tItem *>::iterator dtr = _daughters.begin();
        dtr != _daughters.end(); ++dtr)
    {
        dtrs.push_back((*dtr)->unpack(upedgelimit));
    }

    // Consider all possible combinations of daughter structures
    // and collect the ones that combine. 
    vector<tItem *> config(rule()->arity());
    list<tItem *> res;
    unpack_cross(dtrs, 0, config, res);
 
    return res;
}

void
print_config(FILE *f, int motherid, vector<tItem *> &config)
{
    fprintf(f, "%d[", motherid);
    for(vector<tItem *>::iterator it = config.begin(); it != config.end(); ++it)
        fprintf(f, "%s%d", it == config.begin() ? "" : " ", (*it)->id());
    fprintf(f, "]");
}

// Recursively compute all configurations of dtrs, and accumulate valid
// instantiations (wrt mother) in res.
void
tPhrasalItem::unpack_cross(vector<list<tItem *> > &dtrs,
                           int index, vector<tItem *> &config,
                           list<tItem *> &res)
{
    if(index >= rule()->arity())
    {
        tItem *combined = unpack_combine(config);
        if(combined)
        {
            if(verbosity > 9)
            {
                fprintf(stderr, "%*screated edge %d from ",
                        unpacking_level * 2, "", combined->id());
                print_config(stderr, id(), config);
                fprintf(stderr, "\n");
                combined->print(stderr);
                fprintf(stderr, "\n");
                if(verbosity > 14)
                    dag_print(stderr, combined->get_fs().dag());
            }
            res.push_back(combined);
        }
        else
        {
            stats.p_failures++;
            if(verbosity > 9)
            {
                fprintf(stderr, "%*sfailure instantiating ",
                        unpacking_level * 2, "");
                print_config(stderr, id(), config);
                fprintf(stderr, "\n");
            }
        }
	return;
    }

    for(list<tItem *>::iterator i = dtrs[index].begin(); i != dtrs[index].end();
        ++i)
    {
        config[index] = *i;
        unpack_cross(dtrs, index + 1, config, res);
    }
}

// Try to instantiate mother with a particular configuration of daughters.
// _fix_me_
// This is quite similar to functionality in task.cpp - common functionality
// should be factored out.
tItem *
tPhrasalItem::unpack_combine(vector<tItem *> &daughters)
{
    fs_alloc_state FSAS(false);

    fs res = rule()->instantiate(true);

    list_int *tofill = rule()->allargs();
    
    while(res.valid() && tofill)
    {
        fs arg = res.nth_arg(first(tofill));
        if(rest(tofill))
        {
            if(res.temp())
                unify_generation = res.temp();
            res = unify_np(res, daughters[first(tofill)-1]->get_fs(true), arg);
        }
        else
        {
            // _fix_me_ 
            // the whole _np architecture is rather messy
            if(res.temp())
                unify_generation = res.temp();
            res = unify_restrict(res, daughters[first(tofill)-1]->get_fs(true),
                                 arg,
                                 Grammar->deleted_daughters());
        }
        tofill = rest(tofill);
    }
    
    if(!res.valid())
    {
        FSAS.release();
        return 0;
    }

#ifdef DYNAMIC_SYMBOLS
    if((opt_mrs != 0) && (characterize_mods()))
      if(passive()) {
        assert(_startposition >= 0 && _endposition >= 0);
        res.modify_eagerly_searching(characterize_mods(_startposition
                                                       , _endposition));
      }
#endif

    stats.p_upedges++;
    return new tPhrasalItem(this, daughters, res);
}


#if 0
tLexItem::tLexItem(int start, int end, const tPaths &paths,
                   int ndtrs, int keydtr, input_token **dtrs, 
                   fs &f, const char *printname)
    : tItem(start, end, paths, f, printname),
      _ndtrs(ndtrs), _keydtr(keydtr), _fs_full(f)
{
    // _fix_me_
    // Not nice to overwrite the _fs field.
    if(opt_packing)
        _fs = packing_partial_copy(f, Grammar->packing_restrictor(), false);

    if(_keydtr >= _ndtrs)
        throw tError("keydtr > ndtrs for tLexItem");

    _dtrs = new input_token*[_ndtrs] ;

    for (int i = 0; i < _ndtrs; i++)
        _dtrs[i] = dtrs[i] ;

    _inflrs_todo = copy_list(_dtrs[_keydtr]->form().affixes());
    if(_inflrs_todo)
        _trait = INFL_TRAIT;
    else
        _trait = LEX_TRAIT;

    if(opt_nqc_unif != 0)
        _qc_vector_unif = get_qc_vector(qc_paths_unif, qc_len_unif, f);

    if(opt_nqc_subs != 0)
        _qc_vector_subs = get_qc_vector(qc_paths_subs, qc_len_subs, f);

    // compute _score score for lexical items
    if(Grammar->sm())
        score(Grammar->sm()->scoreLeaf(this));

#ifdef YY
    if(opt_k2y)
    {
        mrs_stamp_fs(_fs, _id);

        for(int i = 0; i < _ndtrs; i++)
            mrs_map_id(_id, _dtrs[i]->id());

        set<string> senses = cheap_settings->smap("type-to-sense", _fs.type());
        for(set<string>::iterator it = senses.begin(); it != senses.end();
            ++it)
            mrs_map_sense(_id, *it);
    }
#endif

#ifdef DEBUG
    fprintf(ferr, "new lexical item (`%s[%s]'):", 
            le->printname(), le->affixprintname());
    print(ferr);
    fprintf(ferr, "\n");
#endif
}
#endif

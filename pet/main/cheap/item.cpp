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
#include "../common/utility.h"
#include "types.h"
#include "item.h"
#include "parse.h"
#include "tsdb++.h"
#include "sm.h"

#ifdef YY
#include "mrs.h"
#endif

//#define DEBUG
//#define DEBUGPOS

item_owner *item::_default_owner = 0;
int item::_next_id = 1;

item::item(int start, int end, const tPaths &paths,
           int p, fs &f, const char *printname)
    : _id(_next_id++), _stamp(-1),
      _start(start), _end(end), _spanningonly(false), _paths(paths),
      _fs(f), _tofill(0), _nfilled(0), _inflrs_todo(0),
      _result_root(-1), _result_contrib(false), _nparents(0), _qc_vector(0),
      _p(p), _score_model(0), _printname(printname), _done(0),
      _blocked(0), parents(), packed()
{
    if(_default_owner) _default_owner->add(this);
}

item::item(int start, int end, const tPaths &paths,
           int p, const char *printname)
    : _id(_next_id++), _stamp(-1),
      _start(start), _end(end), _spanningonly(false), _paths(paths),
      _fs(), _tofill(0), _nfilled(0), _inflrs_todo(0),
      _result_root(-1), _result_contrib(false), _nparents(0), _qc_vector(0),
      _p(p), _score_model(0), _printname(printname), _done(0),
      _blocked(0), parents(), packed()
{
    if(_default_owner) _default_owner->add(this);
}

item::~item()
{
    if(_qc_vector) delete[] _qc_vector;
    free_list(_inflrs_todo);
}

lex_item::lex_item(int start, int end, const tPaths &paths,
                   int ndtrs, int keydtr, input_token **dtrs, 
                   int p, fs &f, const char *printname)
    : item(start, end, paths, p, f, printname),
      _ndtrs(ndtrs), _keydtr(keydtr), _fs_full(f)
{
    // _fix_me_
    // Not nice to overwrite the _fs field.
    if(opt_packing)
        _fs = packing_partial_copy(f, Grammar->packing_restrictor(), false);

    if(_keydtr >= _ndtrs)
        throw error("keydtr > ndtrs for lex_item");

    _dtrs = New input_token*[_ndtrs] ;

    for (int i = 0; i < _ndtrs; i++)
        _dtrs[i] = dtrs[i] ;

    _inflrs_todo = copy_list(_dtrs[_keydtr]->form().affixes());
    if(_inflrs_todo)
        _trait = INFL_TRAIT;
    else
        _trait = LEX_TRAIT;

    if(opt_nqc != 0)
        _qc_vector = get_qc_vector(f);

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

bool same_lexitems(const lex_item &a, const lex_item &b)
{
    if(a.start() != b.start() || a.end() != b.end())
        return false;

    return a._dtrs[a._keydtr]->form() == b._dtrs[b._keydtr]->form();
}

phrasal_item::phrasal_item(grammar_rule *R, item *pasv, fs &f)
    : item(pasv->_start, pasv->_end, pasv->_paths,
           R->priority(pasv->priority()), f, R->printname()),
    _daughters(), _adaughter(0), _rule(R)
{
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

    pasv->_nparents++; pasv->parents.push_back(this);

    if(opt_nqc != 0)
    {
        if(passive())
            _qc_vector = get_qc_vector(f);
        else
            _qc_vector = get_qc_vector(nextarg(f));
    }

    // rule stuff
    if(passive())
        R->passives++;
    else
        R->actives++;

#ifdef DEBUG
    fprintf(ferr, "new rule item (`%s' + %d@%d):", 
            R->printname(), pasv->id(), R->nextarg());
    print(ferr);
    fprintf(ferr, "\n");
#endif
}

phrasal_item::phrasal_item(phrasal_item *active, item *pasv, fs &f)
    : item(-1, -1, active->_paths.common(pasv->_paths),
           active->priority(), f, active->printname()),
    _daughters(active->_daughters), _adaughter(active), _rule(active->_rule)
{
    if(active->left_extending())
    {
        _start = pasv->_start;
        _end = active->_end;
        _daughters.push_front(pasv);
    }
    else
    {
        _start = active->_start;
        _end = pasv->_end;
        _daughters.push_back(pasv);
    }
  
    pasv->_nparents++; pasv->parents.push_back(this);
    active->_nparents++; active->parents.push_back(this);

    _tofill = active->restargs();
    _nfilled = active->nfilled() + 1;

    _trait = SYNTAX_TRAIT;

    if(opt_nqc != 0)
    {
        if(passive())
            _qc_vector = get_qc_vector(f);
        else
            _qc_vector = get_qc_vector(nextarg(f));
    }

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

phrasal_item::phrasal_item(phrasal_item *sponsor, vector<item *> &dtrs, fs &f)
    : item(sponsor->start(), sponsor->end(), sponsor->_paths,
           sponsor->id(), f, sponsor->printname()),
      _daughters(),
      _adaughter(0), _rule(sponsor->rule())
{
    // _fix_me_
    //  copy(dtrs.begin(), dtrs.end(), _daughters.begin());
    for(vector<item *>::iterator it = dtrs.begin(); it != dtrs.end(); ++it)
        _daughters.push_back(*it);

    _trait = SYNTAX_TRAIT;
    _nfilled = dtrs.size(); 
}

void lex_item::set_result_root(type_t rule)
{
    set_result_contrib();
    _result_root = rule;
}

void phrasal_item::set_result_root(type_t rule)
{
    if(result_contrib() == false)
    {
        for(list<item *>::iterator pos = _daughters.begin();
            pos != _daughters.end();
            ++pos)
            (*pos)->set_result_contrib();

        if(_adaughter)
            _adaughter->set_result_contrib();
    }
  
    set_result_contrib();
    _result_root = rule;
}

void item::print(FILE *f, bool compact)
{
    fprintf(f, "[%d %d-%d %s (%d) ", _id, _start, _end, _fs.printname(),
            _trait);
    if(_score_model)
    {
        fprintf(f, "%.4g", _score);
    }
    else
    {
        fprintf(f, "%d", _p);
    }

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
        fprintf(f, "%s ", printnames[first(l)]);
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

void lex_item::print(FILE *f, bool compact)
{
    fprintf(f, "L ");
    item::print(f);
    if(verbosity > 10 && compact == false)
    {
        fprintf(f, "\n");
        _fs.print(f);
    }
}

string
lex_item::description()
{
    if(_ndtrs == 0) return string();
    return _dtrs[_keydtr]->description();
}

void phrasal_item::print(FILE *f, bool compact)
{
    fprintf(f, "P ");
    item::print(f);

    if(verbosity > 10 && compact == false)
    {
        fprintf(f, "\n");
        _fs.print(f);
    }
}

void
item::print_packed(FILE *f)
{
    if(packed.size() == 0)
        return;

    fprintf(f, " < packed: ");
    for(list<item *>::iterator pos = packed.begin();
        pos != packed.end(); ++pos)
        fprintf(f, "%d ",(*pos)->_id);
    fprintf(f, ">");
}

void
phrasal_item::print_family(FILE *f)
{
    fprintf(f, " < dtrs: ");
    for(list<item *>::iterator pos = _daughters.begin();
        pos != _daughters.end(); ++pos)
        fprintf(f, "%d ",(*pos)->_id);
    fprintf(f, " parents: ");
    for(list<item *>::iterator pos = parents.begin();
        pos != parents.end(); ++pos)
        fprintf(f, "%d ",(*pos)->_id);
    fprintf(f, ">");
}

static int derivation_indentation = 0; // not elegant

void lex_item::print_derivation(FILE *f, bool quoted)
{
    if(derivation_indentation == 0)
        fprintf(f, "\n");
    else
        fprintf(f, "%*s", derivation_indentation, "");

    string orth;

    for(int i = 0; i < _ndtrs; i++)
    {
        if(i != 0) orth += " ";
        orth += _dtrs[i]->orth();
    }

    _dtrs[_keydtr]->print_derivation(f, quoted, _id, _p, _inflrs_todo,
                                     orth);
}

void lex_item::print_yield(FILE *f)
{
    list<string> orth;
    for(int i = 0; i < _ndtrs; i++)
        orth.push_back(_dtrs[i]->orth());

    _dtrs[_keydtr]->print_yield(f, _inflrs_todo, orth);
}

void
lex_item::getTagSequence(list<string> &tags, list<list<string> > &words)
{
    list<string> orth;
    for(int i = 0; i < _ndtrs; i++)
        orth.push_back(_dtrs[i]->orth());

    _dtrs[_keydtr]->getTagSequence(_inflrs_todo, orth, tags, words);
}

string lex_item::tsdb_derivation()
{
    string orth;
    for(int i = 0; i < _ndtrs; i++)
    {
        if(i != 0) orth += string(" ");
        orth += _dtrs[i]->orth();
    }

    return _dtrs[_keydtr]->tsdb_derivation(_id, orth);
}

void phrasal_item::print_derivation(FILE *f, bool quoted)
{
    if(derivation_indentation == 0)
        fprintf(f, "\n");
    else
        fprintf(f, "%*s", derivation_indentation, "");

    fprintf(f, 
            "(%d %s %d %d %d", 
            _id, printname(), _p, _start, _end);

    if(packed.size())
    {
        fprintf(f, " {");
        for(list<item *>::iterator pack = packed.begin();
            pack != packed.end(); ++pack)
        {
            fprintf(f, "%s%d", pack == packed.begin() ? "" : " ", (*pack)->id()); 
        }
        fprintf(f, "}");
    }

    if(_result_root != -1)
    {
        fprintf(f, " [%s]", printnames[_result_root]);
    }
  
    if(_inflrs_todo)
    {
        fprintf(f, " [");
        for(list_int *l = _inflrs_todo; l != 0; l = rest(l))
        {
            fprintf(f, "%s%s", printnames[first(l)], rest(l) == 0 ? "" : " ");
        }
        fprintf(f, "]");
    }

    derivation_indentation+=2;
    for(list<item *>::iterator pos = _daughters.begin();
        pos != _daughters.end(); ++pos)
    {
        fprintf(f, "\n");
        (*pos)->print_derivation(f, quoted);
    }
    derivation_indentation-=2;

    fprintf(f, ")");
}

void phrasal_item::print_yield(FILE *f)
{
    for(list<item *>::iterator pos = _daughters.begin();
        pos != _daughters.end(); ++pos)
    {
        (*pos)->print_yield(f);
    }
}

void
phrasal_item::getTagSequence(list<string> &tags, list<list<string> > &words)
{
    for(list<item *>::iterator pos = _daughters.begin();
        pos != _daughters.end(); ++pos)
    {
        (*pos)->getTagSequence(tags, words);
    }
}
        
string phrasal_item::tsdb_derivation()
{
    string result;

    result = string("(") +
        inttostr(_id) + string(" ") +
        string(printname()) + string(" ") +
        inttostr(_p) + string(" ") +
        inttostr(_start) + string(" ") +
        inttostr(_end);

    for(list<item *>::iterator pos = _daughters.begin();
        pos != _daughters.end(); ++pos)
    {
        result += string(" ");
        result += (*pos)->tsdb_derivation();
    }

    result += string(")");

    return result;
}

grammar_rule *lex_item::rule()
{
    return NULL;
}

grammar_rule *phrasal_item::rule()
{
    return _rule;
}

void lex_item::recreate_fs()
{
    throw error("cannot rebuild lexical item's feature structure");
}

void phrasal_item::recreate_fs()
{
    if(!passive())
    {
        assert(_rule->arity() <= 2);
        _fs = _rule->instantiate();
        fs arg = _rule->nextarg(_fs);
        _fs = unify_np(_fs, _daughters.front()->get_fs(), arg);
        if(!_fs.valid())
            throw error("trouble rebuilding active item (1)");
    }
    else
    {
        throw error("won't rebuild passive item");
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

void
lex_item::adjust_priority(const char *settingname)
{
    setting *set = cheap_settings->lookup(settingname);
    if(set == 0)
        return;
    
    for(int i = 0; i < set->n; i+=3)
    {
        if(i+2 > set->n)
        {
            fprintf(ferr, "warning: incomplete last entry "
                    "in POS mapping `%s' - ignored\n", settingname);
            break;
        }
        
        char *lhs = set->values[i],
             *rhs = set->values[i+1];
        
        int prio = strtoint(rhs, "as priority value in POS mapping");

        if(get_in_postags().contains(lhs))
        {
            if(verbosity > 4)
            {
                fprintf(ferr, "Adjusting (%s/%d) ", lhs, prio);
                print(ferr);
            }
            adjust_priority(prio);
        }
    }
}

//
// Blocking (frosting and freezing) for packing
//

void
item::block(int mark)
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

    for(list<item *>::iterator parent = parents.begin();
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

list<item *>
item::unpack()
{
    list<item *> res;

    unpacking_level++;
    if(verbosity > 2)
        fprintf(stderr, "%*s> unpack [%d]\n", unpacking_level * 2, "", id());

    // Ignore frozen items.
    if(frozen())
    {
        if(verbosity > 2)
            fprintf(stderr, "%*s< unpack [%d] ( )\n", unpacking_level * 2, "", id());
        unpacking_level--;
        return res;
    }

    // Recursively unpack items that are packed into this item.
    for(list<item *>::iterator p = packed.begin();
        p != packed.end(); ++p)
    {
        // Append result of unpack_item on packed item.
        list<item *> tmp = (*p)->unpack();
        res.splice(res.begin(), tmp);
    }

    // Unpack children.
    list<item *> tmp = unpack1();
    res.splice(res.begin(), tmp);

    if(verbosity > 2)
    {
        fprintf(stderr, "%*s< unpack [%d] ( ", unpacking_level * 2, "", id());
        for(list<item *>::iterator i = res.begin(); i != res.end(); ++i)
            fprintf(stderr, "%d ", (*i)->id());
        fprintf(stderr, ")\n");
    }

    return res;
}

list<item *>
lex_item::unpack1()
{
    list<item *> res;
    res.push_back(this);
    return res;
}

list<item *>
phrasal_item::unpack1()
{
    // Collect expansions for each daughter.
    vector<list<item *> > dtrs;
    for(list<item *>::iterator dtr = _daughters.begin();
        dtr != _daughters.end(); ++dtr)
    {
        dtrs.push_back((*dtr)->unpack());
    }

    // Consider all possible combinations of daughter structures
    // and collect the ones that combine. 
    vector<item *> config(rule()->arity());
    list<item *> res;
    unpack_cross(dtrs, 0, config, res);
 
    return res;
}

void
print_config(FILE *f, int motherid, vector<item *> &config)
{
    fprintf(f, "%d[", motherid);
    for(vector<item *>::iterator it = config.begin(); it != config.end(); ++it)
        fprintf(f, "%s%d", it == config.begin() ? "" : " ", (*it)->id());
    fprintf(f, "]");
}

// Recursively compute all configurations of dtrs, and accumulate valid
// instantiations (wrt mother) in res.
void
phrasal_item::unpack_cross(vector<list<item *> > &dtrs,
                           int index, vector<item *> &config,
                           list<item *> &res)
{
    if(index >= rule()->arity())
    {
        item *combined = unpack_combine(config);
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

    for(list<item *>::iterator i = dtrs[index].begin(); i != dtrs[index].end();
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
item *
phrasal_item::unpack_combine(vector<item *> &daughters)
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

    return New phrasal_item(this, daughters, res);
}

//
// Scoring
//


double lex_item::score(tSM *sm)
{
    if(_score_model == sm)
    {
        // Score was already computed for this model.
        return _score;
    }

    // _fix_me_
    // Compute something useful here

    _score_model = sm;
    return _score = sm->neutralScore();
}

double phrasal_item::score(tSM *sm)
{
    if(_score_model == sm)
    {
        // Score was already computed for this model.
        return _score;
    }

    vector<int> v;
    v.push_back(1);
    v.push_back(rule()->type());

    double total = sm->neutralScore();

    for(list<item *>::iterator dtr = _daughters.begin();
        dtr != _daughters.end(); ++dtr)
    {
        v.push_back((*dtr)->identity());
        total = sm->combineScores(total, (*dtr)->score(sm));
    }

    tSMFeature f(v);

    _score_model = sm;
    return _score = sm->combineScores(total, sm->score(f));
}

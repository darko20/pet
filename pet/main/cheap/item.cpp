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
      _p(p), _q(0), _r(0), _printname(printname), _done(0), _frozen(0),
      parents(), packed()
{
    if(_default_owner) _default_owner->add(this);
}

item::item(int start, int end, const tPaths &paths,
           int p, const char *printname)
    : _id(_next_id++), _stamp(-1),
      _start(start), _end(end), _spanningonly(false), _paths(paths),
      _fs(), _tofill(0), _nfilled(0), _inflrs_todo(0),
      _result_root(-1), _result_contrib(false), _nparents(0), _qc_vector(0),
      _p(p), _q(0), _r(0), _printname(printname), _done(0), _frozen(0),
      parents(), packed()
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
    : item(start, end, paths, p, f, printname), _ndtrs(ndtrs), _keydtr(keydtr)
{
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

    _q = pasv->priority();

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

    _q = pasv->priority();
  
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
    fprintf(f, "[%d %d-%d %s (%d) %d {", _id, _start, _end, _fs.printname(),
            _trait, _p);

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

    _dtrs[_keydtr]->print_derivation(f, quoted, _id, _p, _q, _inflrs_todo,
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
            "(%d %s %d/%d %d %d", 
            _id, printname(), _p, _q, _start, _end);

    if(_result_root != -1)
    {
        fprintf(f, " [%s %d]", printnames[_result_root], _r);
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


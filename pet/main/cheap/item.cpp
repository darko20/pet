/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* chart items */

#include "pet-system.h"
#include "cheap.h"
#include "../common/utility.h"
#include "types.h"
#include "item.h"
#include "parse.h"
#include "tsdb++.h"

//#define DEBUG
//#define DEBUGPOS

item_owner *item::_default_owner = 0;
int item::_next_id = 1;

item::item(int start, int end, int p, fs &f, const char *printname)
  : _id(_next_id++), _stamp(-1),
    _start(start), _end(end), _spanningonly(false), _fs(f), _tofill(0), 
    _inflrs_todo(0),
    _result_root(-1), _result_contrib(false), _nparents(0), _qc_vector(0),
    _p(p), _q(0), _r(0), _printname(printname), _done(0), _frozen(0),
    parents(), packed()
{
  if(_default_owner) _default_owner->add(this);
}

item::item(int start, int end, int p, const char *printname)
  : _id(_next_id++), _stamp(-1),
    _start(start), _end(end), _spanningonly(false), _fs(), _tofill(0),
    _inflrs_todo(0),
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

lex_item::lex_item(int start, int end, int ndtrs, int keydtr,
		   input_token **dtrs, int p, fs &f, const char *printname)
  : item(start, end, p, f, printname), _ndtrs(ndtrs), _keydtr(keydtr)
{
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
#ifdef DEBUG
  fprintf(ferr, "new lexical item (`%s[%s]'):", 
	  le->printname(), le->affixprintname());
  print(ferr);
  fprintf(ferr, "\n");
#endif
}

bool
lex_item::same_dtrs(int ndtrs, input_token **dtrs) const
{
  if(_ndtrs != ndtrs)
    return false;
  
  for(int i = 0; i < _ndtrs; i++)
    {
      if(i == _keydtr)
	{
	  if(*_dtrs[i] != *dtrs[i])
	    return false;
	}
      else
	{
	  if(_dtrs[i]->orth() != dtrs[i]->orth())
	    return false;
	}
    }
  
  return true;
}

phrasal_item::phrasal_item(grammar_rule *R, item *pasv, fs &f)
  : item(pasv->_start, pasv->_end, R->priority(pasv->priority()), f, R->printname()), _daughters(), _adaughter(0), _rule(R)
{
  _tofill = R->restargs();
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
  : item(-1, -1, active->priority(), f, active->printname()), _daughters(active->_daughters), _adaughter(active), _rule(active->_rule)
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
  
  fprintf(f, "}]");

  if(verbosity > 2 && compact == false)
    {
      fprintf(f, "\n");
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

  fprintf(f, " <");
  for(list<item *>::iterator pos = _daughters.begin();
      pos != _daughters.end(); ++pos)
    fprintf(f, "%d ",(*pos)->_id);
  fprintf(f, ">");

  if(verbosity > 10 && compact == false)
    {
      fprintf(f, "\n");
      _fs.print(f);
    }
}

static int derivation_indentation = 0; // not elegant

void lex_item::print_derivation(FILE *f, bool quoted, int offset)
{
  fprintf(f, "%*s", derivation_indentation, "");

  string orth;

  for(int i = 0; i < _ndtrs; i++)
    {
      if(i != 0) orth += " ";
      orth += _dtrs[i]->orth();
    }

  _dtrs[_keydtr]->print_derivation(f, quoted, offset, _id, _p, _q, _inflrs_todo, orth);
}

string lex_item::tsdb_derivation(int offset)
{
  string orth;
  for(int i = 0; i < _ndtrs; i++)
    {
      if(i != 0) orth += string(" ");
      orth += _dtrs[i]->orth();
    }

  return _dtrs[_keydtr]->tsdb_derivation(offset, _id, orth);
}

void phrasal_item::print_derivation(FILE *f, bool quoted, int offset)
{
  if(derivation_indentation == 0)
    fprintf(f, "\n");
  else
    fprintf(f, "%*s", derivation_indentation, "");

  fprintf(f, 
          "(%d %s %d/%d %d %d", 
          _id, printname(), _p, _q, _start - offset, _end - offset);

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
      // the offset here gives us positions relative to the mother node
      (*pos)->print_derivation(f, quoted, _start);
    }
  derivation_indentation-=2;

  fprintf(f, ")");
}

string phrasal_item::tsdb_derivation(int offset)
{
  string result;

  result = string("(") +
    inttostr(_id) + string(" ") +
    string(printname()) + string(" ") +
    inttostr(_p) + string(" ") +
    inttostr(_start - offset) + string(" ") +
    inttostr(_end - offset);

  for(list<item *>::iterator pos = _daughters.begin();
      pos != _daughters.end(); ++pos)
    {
      result += string(" ");
      result += (*pos)->tsdb_derivation(_start);
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

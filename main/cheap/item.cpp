/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* chart items */

#include <assert.h>

#include "cheap.h"
#include "../common/utility.h"
#include "types.h"
#include "item.h"
#include "parse.h"
#include "tsdb++.h"

//#define DEBUG
//#define DEBUGPOS

int item::next_id = 1;

item::item(int start, int end, fs &f, char *name)
  : _stamp(-1), _start(start), _end(end), _fs(f), _tofill(0), _result_root(0), _result_contrib(0), _nparents(0), _name(name), parents(), packed()
{
  _id = next_id++;
  _qc_vector = 0;
  _done = 0;
#ifdef PACKING
  _frozen = 0;
#endif
}

item::item(int start, int end, char *name)
  : _stamp(-1), _start(start), _end(end), _tofill(0), _result_root(0), _result_contrib(0), _nparents(0), _name(name), parents(), packed()
{
  _id = next_id++;
  _qc_vector = 0;
  _done = 0;
  _fs = fs();
}

item::~item()
{
  if(_qc_vector != 0) delete[] _qc_vector;
}

lex_item::lex_item(class lex_entry *le, int pos, 
                   fs &f, const postags &POS, int offset)
  : item(pos - le->offset(), pos - le->offset() + le->length(), f, le->name()),
    _pos()
{
  _le = le;
  _offset = offset;

  if(opt_nqc != 0)
    _qc_vector = get_qc_vector(f);

  _p = le->priority();
  
#ifdef YY
  //
  // _hack_ 
  // stamp in position information for K2Y: link input words to K2Y clauses
  //
  if(opt_k2y)
    {
      fs label = f.get_path_value("SYNSEM.LOCAL.KEYS.KEY.LABEL");
      if(!label.valid())
        label = f.get_path_value("SYNSEM.LOCAL.KEYS.ALTKEY.LABEL");
      if(label.valid() && label.type() == BI_CONS)
	{
	  int *span = new int[le->length()];
	  for(int i = 0; i < le->length(); i++)
	    span[i] = _start + i;
	  fs ras = fs(dag_listify_ints(span, le->length()));
	  delete[] span;
	  fs result = unify(f, label, ras);
	  if(result.valid()) _fs = result;
	}
    }

  setting *set;
  if((set = cheap_settings->lookup("type-to-pos")) != 0)
    {
      for(int i = 0; i < set->n; i+=2)
	{
	  if(i+2 > set->n)
	    {
	      fprintf(ferr, "warning: incomplete last entry in type to POS mapping - ignored\n");
	      break;
	    }
      
	  char *lhs = set->values[i], *rhs = set->values[i+1];
	  int id = lookup_type(lhs);
	  if(id != -1)
	    {
	      if(subtype(_le->basetype(), id))
		{
		  _pos = string(rhs);
		  break;
		}
	    }
	  else
	    fprintf(ferr, "warning: unknown type `%s' in type to POS mapping - ignored\n", lhs);

	}

    }
#endif

#ifdef DEBUG
  fprintf(ferr, "new lexical item (`%s[%s]'):", 
	  le->name(), le->affixname());
  print(ferr);
  fprintf(ferr, "\n");
#endif
}

//
// Adjust priority for generic lexical item according to POS information
// and mapping specified in configuration file
//
// Find first matching tuple in mapping, and set to this priority
// If not match found, leave unchanged
//

void generic_le_item::compute_pos_priority(const postags &POS)
{
  setting *set = cheap_settings->lookup("posmapping");
  if(set == 0) return;

#ifdef DEBUGPOS
  fprintf(ferr, "adjusting priority for %s(`%s')\n", typenames[_type],
          _orth.c_str());
#endif

  for(int i = 0; i < set->n; i+=3)
    {
      if(i+3 > set->n)
        {
          fprintf(ferr, "warning: incomplete last entry in POS mapping - ignored\n");
          break;
        }
      
      char *lhs = set->values[i], *p = set->values[i+1],
        *rhs = set->values[i+2];
      
#ifdef DEBUGPOS
      fprintf(ferr, "checking triple %d (%s %s %s)\n",
              i, lhs, p, rhs);
#endif

      int prio = strtoint(p, "as priority value in POS mapping");

      int type = lookup_type(rhs);
      if(type == -1)
        {
          fprintf(ferr, "warning: unknown type `%s' in POS mapping\n",
                  rhs);
        }
      else
	{
	  if(subtype(_type, type))
	    {
#ifdef DEBUGPOS
	      fprintf(ferr, "entry #%d matches rhs (%s, %d, %s)\n",
		      i/3, rhs, prio, lhs);
#endif
	      if(POS.contains(lhs))
		{
#ifdef DEBUGPOS
		  fprintf(ferr, "  also matches lhs\n");
#endif
		  _p = prio;
		  return;
		}
	      
	    }
	}
    }

}

generic_le_item::generic_le_item(int type, int pos, 
                                 string orth, const postags &POS, int discount)
  : item(pos, pos+1, typenames[type]), _orth(orth)
{
  _type = type;
  
  fs e(leaftype_parent(_type));
  if(!e.valid())
    throw error("unable to instantiate base for generic");

  _fs = unify(e, fs(_type), e);
  if(!_fs.valid())
    throw error("unable to expand generic");

  if(opt_nqc != 0)
    _qc_vector = get_qc_vector(_fs);

  char *v;
  if((v = cheap_settings->value("default-gen-le-priority")) != 0)
    {
      _p = strtoint(v, "as value of default-le-priority");
    }

  if((v = cheap_settings->sassoc("likely-le-types", _type)) != 0)
    {
      _p = strtoint(v, "in value of likely-le-types");
    }

  if((v = cheap_settings->sassoc("unlikely-le-types", _type)) != 0)
    {
      _p = strtoint(v, "in value of unlikely-le-types");
    }

  compute_pos_priority(POS);
  _p -= discount;
  if(_p < 0) _p = 0;

#ifdef YY
  //
  // _hack_ 
  // stamp in position information for K2Y: link input words to K2Y clauses
  //
  if(opt_k2y) {
    fs label = _fs.get_path_value("SYNSEM.LOCAL.KEYS.KEY.LABEL");
    if(label.valid() && label.type() == BI_CONS) {
      int span[1];
      span[0] = pos;
      fs ras = fs(dag_listify_ints(span, 1));
      fs result = unify(_fs, label, ras);
      if(result.valid()) _fs = result;
    } /* if */
  } /* if */
#endif

#if (defined(DEBUG) || defined(DEBUGPOS))
  fprintf(ferr, "new generic lexical item (`%s'):",
         typenames[_type]);
  print(ferr);
  fprintf(ferr, "\n");
#endif
}

phrasal_item::phrasal_item(grammar_rule *R, item *pasv, fs &f)
  : item(pasv->_start, pasv->_end, f, R->name()), _daughters(), _adaughter(0), _rule(R)
{
  _tofill = R->restargs();
  _daughters.push_back(pasv);

  pasv->_nparents++; pasv->parents.push_back(this);

  if(opt_nqc != 0)
    {
      if(passive())
	_qc_vector = get_qc_vector(f);
      else
	_qc_vector = get_qc_vector(nextarg(f));
    }

  _p = R->priority();
  _q = pasv->priority();

  // rule stuff
  if(passive())
    R->passives++;
  else
    R->actives++;

#ifdef DEBUG
  fprintf(ferr, "new rule item (`%s' + %d@%d):", 
	  R->name(), pasv->id(), R->nextarg());
  print(ferr);
  fprintf(ferr, "\n");
#endif
}

phrasal_item::phrasal_item(phrasal_item *active, item *pasv, fs &f)
  : item(-1, -1, f, active->name()), _daughters(active->_daughters), _adaughter(active), _rule(active->_rule)
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

  if(opt_nqc != 0)
    {
      if(passive())
	_qc_vector = get_qc_vector(f);
      else
	_qc_vector = get_qc_vector(nextarg(f));
    }

  _p = active->priority();
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

string lex_item::postag()
{
  return _pos;
}

void lex_item::set_result(bool root)
{
  _result_contrib = 1;
  if(root) _result_root = 1;
}

void generic_le_item::set_result(bool root)
{
  _result_contrib = 1;
  if(root) _result_root = 1;
}

void phrasal_item::set_result(bool root)
{
  if(_result_contrib == 0)
    {
      for(list<item *>::iterator pos = _daughters.begin();
          pos != _daughters.end();
          ++pos)
	(*pos)->set_result(false);

      if(_adaughter)
	_adaughter->set_result(false);
    }
  
  _result_contrib = 1;
  if(root) _result_root = 1;
}

void item::print(FILE *f, bool compact)
{
  fprintf(f, "[%d %d-%d %s (%d) %d {", _id, _start, _end, _fs.name(), _fs.size(), _p);

  list_int *l = _tofill;
  while(l)
    {
      fprintf(f, "%d ", first(l));
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
  if(!_pos.empty()) fprintf(f, "%s ", _pos.c_str());
  item::print(f);
  if(verbosity > 10 && compact == false)
    {
      fprintf(f, "\n");
      _fs.print(f);
    }
}

void generic_le_item::print(FILE *f, bool compact)
{
  fprintf(f, "G ");
  item::print(f);
  if(verbosity > 10 && compact == false)
    {
      fprintf(f, "\n");
      _fs.print(f);
    }
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

void lex_item::print_derivation(FILE *f, bool quoted, int offset)
{
  _le->print_derivation(f, quoted, _start - offset, _end - offset, _id);
}

#ifdef TSDBAPI
void lex_item::tsdb_print_derivation(int offset)
{
  _le->tsdb_print_derivation(_start - offset, _end - offset, _id);
}
#endif

void generic_le_item::print_derivation(FILE *f, bool quoted, int offset)
{
  fprintf(f, 
          "(%d %s %d %d %d (%s\"", 
          _id, name(), _p, _start, _end, quoted ? "\\" : "");
  fprintf(f, "%s", _orth.c_str());
  fprintf(f, "%s\" %d %d))", quoted ? "\\" : "", _start, _end);
}

#ifdef TSDBAPI
void generic_le_item::tsdb_print_derivation(int offset)
{
  capi_printf("(%d %s %d %d %d (\\\"", 
              _id, name(), _p, _start, _end);
  //
  // _hack_
  // really, capi_printf() should do appropriate escaping, depending on the
  // context; but we would need to elaborate it significantly, i guess, so 
  // hack around it for the time being: it seems very unlikely, we will see 
  // embedded quotes anyplace else. 
  //                                                  (1-feb-01  -  oe@yy)
  for(char *foo = (char *)_orth.c_str(); *foo; foo++) {
    if(*foo == '"' || *foo == '\\') {
      capi_putc('\\');
      capi_putc('\\');
      capi_putc('\\');
    } /* if */
    capi_putc(*foo);
  } /* for */
  capi_printf("\\\" %d %d))",  _start, _end);
}
#endif

void phrasal_item::print_derivation(FILE *f, bool quoted, int offset)
{
  fprintf(f, 
          "(%d %s %d %d %d", 
          _id, name(), _p, _start - offset, _end - offset);

  for(list<item *>::iterator pos = _daughters.begin();
      pos != _daughters.end(); ++pos)
    {
      fprintf(f, " ");
      (*pos)->print_derivation(f, quoted, _start);
    }

  fprintf(f, ")");
}

#ifdef TSDBAPI
void phrasal_item::tsdb_print_derivation(int offset)
{
  capi_printf("(%d %s %d %d %d", 
              _id, name(), _p, _start - offset, _end - offset);

  for(list<item *>::iterator pos = _daughters.begin();
      pos != _daughters.end(); ++pos)
    {
      capi_printf(" ");
      (*pos)->tsdb_print_derivation(_start);
    }

  capi_printf(")");
}
#endif

grammar_rule *lex_item::rule()
{
  return NULL;
}

grammar_rule *generic_le_item::rule()
{
  return NULL;
}

grammar_rule *phrasal_item::rule()
{
  return _rule;
}

void lex_item::recreate_fs()
{
  _fs = _le->instantiate();
}

void generic_le_item::recreate_fs()
{
  _fs = fs(_type);
  if(!_fs.valid())
    throw error("unable to expand generic");
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

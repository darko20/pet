/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * 2001 Bernd Kiefer kiefer@dfki.de
 */

#include "pet-system.h"
#include "cheap.h"
#include "parse.h"
#include "inputchart.h"
#include "item.h"
#include "dag-common.h"

#ifdef YY
#include "k2y.h"
#endif

input_token::input_token(int s, int e, class full_form ff, string o,
			 int p, const postags &pos, input_chart *cont,
			 bool synthesized) :
  _synthesized(synthesized), _start(-1), _end(-1),
  _startposition(s), _endposition(e),
  _orth(o), _in_pos(pos), _supplied_pos(ff), _p(p), _form(ff),
  _container(cont)
{
}

void
input_token::print(ostream &f)
{
  f << "{[" << _start << _end << "] " << _orth
    << "->" << _form.affixprintname() << "}" << endl;
}

void
input_token::print(FILE *f)
{
  fprintf(f, "{[%d %d](%d %d) <%d> `%s' ",
	  _start, _end, _startposition, _endposition,
	  _p, _orth.c_str());

  _form.print(f);
  fprintf(f, " In:");
  _in_pos.print(f);
  fprintf(f, " Supplied:");
  _supplied_pos.print(f);

  fprintf(f, "}");
}

string
input_token::description()
{
  return _orth + "[" + _form.description() + "]";
}

void
input_token::print_derivation(FILE *f, bool quoted, int offset,
			      int id, int p, int q,
			      list_int *inflrs_todo, string orth)
{
  // offset is used to print positions relative to the mother node
  int start = _start - offset; 
  int end = _end - offset;

  fprintf (f, "(%d %s %d/%d %d %d ", id, _form.stemprintname(),
	   p, q, start, end);

  fprintf(f, "[");
  for(list_int *l = inflrs_todo; l != 0; l = rest(l))
    fprintf(f, "%s%s", printnames[first(l)], rest(l) == 0 ? "" : " ");
  fprintf(f, "] ");

  fprintf (f, "(%s\"%s%s\" %d %d %d)",
	   quoted ? "\\" : "", orth.c_str(), quoted ? "\\" : "", _p,
	   start, end);

  fprintf(f, ")");
}

#ifdef TSDBAPI
void
input_token::tsdb_print_derivation(int offset, int id, string orth)
{
  // offset is used to print positions relative to the mother node
  int start = _start - offset; 
  int end = _end - offset;

  capi_printf("(%d %s %d %d %d (", id, _form.stemprintname(), _p,
	      start, end);
  capi_putstr(orth.c_str(), true);
  capi_printf(" %d %d))", start, end);
}
#endif

fs
input_token::instantiate()
{
  return _form.instantiate();
}  

list<lex_item *>
input_token::generics(int discount, postags onlyfor)
// Add generic lexical entries for token at position `i'. If `onlyfor' is 
// non-empty, only those generic entries corresponding to one of those
// POS tags are postulated. The correspondence is defined in posmapping.
{
  list<lex_item *> result;

  list_int *gens = Grammar->generics();

  if(!gens)
    return result;

  if(verbosity > 4)
    fprintf(ferr, "using generics for [%d - %d] `%s':\n", 
	    _start, _end, _orth.c_str());

  for(; gens != 0; gens = rest(gens))
    {
      int gen = first(gens);

      char *suffix = cheap_settings->assoc("generic-le-suffixes",
					   typenames[gen]);
      if(suffix)
	if(_orth.length() <= strlen(suffix) ||
	   strcasecmp(suffix,
		      _orth.c_str() + _orth.length() - strlen(suffix)) != 0)
	  continue;

      if(!onlyfor.empty() && !onlyfor.contains(gen))
	continue;

      if(verbosity > 4)
	fprintf(ferr, "  ==> %s [%s]\n", printnames[gen],
		suffix == 0 ? "*" : suffix);
	  
      int p = 0; 

      char *v;
      if((v = cheap_settings->value("default-gen-le-priority")) != 0)
	p = strtoint(v, "as value of default-gen-le-priority");
      
      if((v = cheap_settings->sassoc("likely-le-types", gen)) != 0)
	p = strtoint(v, "in value of likely-le-types");

      if((v = cheap_settings->sassoc("unlikely-le-types", gen)) != 0)
	p = strtoint(v, "in value of unlikely-le-types");

      p = _in_pos.priority(gen, p); 

      p -= discount; if(p < 0) p = 0;

      input_token *dtrs[1];
      dtrs[0] = _container->add_token(_startposition, _endposition,
				     full_form(Grammar->lookup_stem(gen)),
				     _orth, p, _in_pos, true);

      _container->assign_position(dtrs[0]);

      fs f = fs(gen); 

#ifdef YY
      if(opt_k2y)
	k2y_stamp_fs(f, 1, dtrs);
#endif

      lex_item *lex =
	New lex_item(_start, _end, 1, 0, dtrs, p, f, _orth.c_str());

      result.push_back(lex);
    }

  return result;
}

bool contains(list<lex_item *> &les, lex_item *le)
{
  for(list<lex_item *>::iterator it = les.begin();
      it != les.end(); ++it)
    if(**it == *le)
      return true;
  
  return false;
}

void
input_token::add_result(int start, int end, int ndtrs, int keydtr,
			input_token **dtrs, list<lex_item *> &result)
{
  // preserve allocation state so we can return in case of duplicates
  fs_alloc_state FSAS(false);
  
  // instantiate feature structure of lex_entry with modifications
  fs f = this->instantiate();

  // if fs is valid, create a new lex item and task
  if(f.valid())
    {
      int p = dtrs[keydtr]->priority();
#ifdef YY
      if(opt_k2y)
	k2y_stamp_fs(f, ndtrs, dtrs);
#endif
      
      lex_item *it = New lex_item(start, end, ndtrs, keydtr,
				  dtrs, p, f,
				  _form.description().c_str());
      
      if(contains(result, it))
	{
	  FSAS.release();
	}
      else
	result.push_back(it);
    }
}

void
input_token::expand_rec(int arg_pos, int start, int end, input_token ** dtrs,
			 list<lex_item *> &result)
{
  if(arg_pos <= _form.offset())        // expand to the left
    {
      if(arg_pos < 0)
	{
	  // change direction of expansion
	  expand_rec(_form.offset() + 1, start, end, dtrs, result);
	}
      else
	{
	  // iterate over all left adjacent input items matching the string
	  // _orth[arg_pos]
	  for(adj_iterator padj = _container->begin(adj_iterator::left, start,
						    _form.stem()->orth(arg_pos));
	      padj.valid(); padj++)
	    {
	      dtrs[arg_pos] = *padj;
	      expand_rec(arg_pos - 1, (*padj)->start(), end, dtrs, result);
	    }
	}
    }
  else
    {				        // expand to the right
      if(arg_pos == _form.stem()->length())
	{
	  // we managed to match an entry completely
	  add_result(start, end, _form.stem()->length(),
		     _form.offset(), dtrs, result);
	}
      else
	{
	  // iterate over all right adjacent input items matching the string
	  // _orth[arg_pos]
	  for(adj_iterator padj = _container->begin(adj_iterator::right, end,
						    _form.stem()->orth(arg_pos));
	      padj.valid(); padj++)
	    {
	      dtrs[arg_pos] = *padj;
	      expand_rec(arg_pos + 1, start, (*padj)->end(), dtrs, result);
	    }
	}
    }
}

void
input_token::expand(list<lex_item *> &result)
{
  if(_form.valid())
    {
      // there is a lex_entry corresponding to tok input token: try to create
      // an input item for the chart. input_token without valid entry can be
      // used in multi-word lexemes
      input_token **dtrs = New input_token* [_form.stem()->length()];
      dtrs[_form.offset()] = this;
      if(_form.stem()->length() > 1)
	expand_rec(_form.offset() - 1, _start, _end, dtrs, result);
      else
	add_result(_start, _end, 1, 0, dtrs, result);
      delete[] dtrs;  
    }
}

/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 * (C) 2001 Bernd Kiefer kiefer@dfki.de
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

#include "pet-system.h"
#include "cheap.h"
#include "parse.h"
#include "inputchart.h"
#include "item.h"
#include "dag-common.h"
#include "grammar.h"

input_token::input_token(int id, int s, int e, class full_form ff, string o,
                         double p, const postags &pos, const tPaths &paths,
                         input_chart *cont, bool synthesized) :
    _synthesized(synthesized), _id(id), _start(-1), _end(-1),
    _startposition(s), _endposition(e),
    _orth(o), _in_pos(pos), _supplied_pos(ff), _p(p), _form(ff),
    _paths(paths),
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
    fprintf(f, "{[%d %d](%d %d) %d <%.2f> `%s' ",
            _start, _end, _startposition, _endposition, _id,
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
input_token::print_derivation(FILE *f, bool quoted,
                              int id, type_t letype, double p,
                              list_int *inflrs_todo, string orth)
{
    int start = _start; 
    int end = _end;

    fprintf (f, "(%d %s/%s %.2f %d %d ", id, _form.stemprintname(),
             printnames[letype], p, start, end);

    fprintf(f, "[");
    for(list_int *l = inflrs_todo; l != 0; l = rest(l))
        fprintf(f, "%s%s", printnames[first(l)], rest(l) == 0 ? "" : " ");
    fprintf(f, "] ");

    fprintf (f, "(%s\"%s%s\" %.2f %d %d)",
             quoted ? "\\" : "", orth.c_str(), quoted ? "\\" : "", _p,
             start, end);

    fprintf(f, ")");
}

void
input_token::print_yield(FILE *f, list_int *inflrs, list<string> &orth)
{
    for(list<string>::iterator it = orth.begin(); it != orth.end(); ++it)
    {
        fprintf(f, "%s ", it->c_str());
    }
}

string
input_token::tsdb_derivation(int id, string orth)
{
    ostringstream res;
  
    res << "(" << id << " " << _form.stemprintname()
        << " " << _p << " " << _start <<  " " << _end
        << " (\"" << orth << "\" " << _start << " " << _end << "))";

    return res.str();
}

fs
input_token::instantiate()
{
    return _form.instantiate();
}  

list<tLexItem *>
input_token::generics(postags onlyfor)
// Add generic lexical entries for token at position `i'. If `onlyfor' is 
// non-empty, only those generic entries corresponding to one of those
// POS tags are postulated. The correspondence is defined in posmapping.
{
    list<tLexItem *> result;

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
                          _orth.c_str() + _orth.length() - strlen(suffix))
               != 0)
                continue;

        if(!_in_pos.license("posmapping", gen))
            continue;

        if(!onlyfor.empty() && !onlyfor.contains(gen))
            continue;

        if(verbosity > 4)
            fprintf(ferr, "  ==> %s [%s]\n", printnames[gen],
                    suffix == 0 ? "*" : suffix);
	  
        input_token *dtrs[1];
        dtrs[0] = _container->add_token(_id, _startposition, _endposition,
                                        full_form(Grammar->find_stem(gen)),
                                        _orth, 0.0, _in_pos, list<int>(), 
                                        true);

        _container->assign_position(dtrs[0]);

        fs f = fs(gen); 

        tLexItem *lex =
            new tLexItem(_start, _end, _paths, 1, 0, dtrs, f,
                         _orth.c_str());

        result.push_back(lex);
    }

    return result;
}

bool
contains(list<tLexItem *> &les, tLexItem *le)
{
    for(list<tLexItem *>::iterator it = les.begin();
        it != les.end(); ++it)
        if(same_lexitems(**it, *le))
            return true;
  
    return false;
}

bool
input_token::add_result(int start, int end, int ndtrs, int keydtr,
                        input_token **dtrs, list<tLexItem *> &result)
{
    // preserve allocation state so we can return in case of duplicates
    fs_alloc_state FSAS(false);
    
    // instantiate feature structure of lex_entry with modifications
    fs f = this->instantiate();
  
    // if fs is valid, create a new lex item and task
    if(f.valid())
    {
        tLexItem *it = new tLexItem(start, end, _paths, 
                                    ndtrs, keydtr, dtrs, f,
                                    _form.description().c_str());

        if(contains(result, it))
        {
            if(verbosity > 4)
            {
                fprintf(stderr, "Filtering duplicate input chart item:\n");
                it->print(stderr);
                fprintf(stderr, "\n");
            }
            FSAS.release();
        }
        else
        {
            result.push_back(it);
        }
        return true;
    }
    return false;
}

bool
input_token::expand_rec(int arg_pos, int start, int end, input_token ** dtrs,
                        list<tLexItem *> &result)
{
    if(arg_pos <= _form.offset())        // expand to the left
    {
        if(arg_pos < 0)
        {
            // change direction of expansion
            if(expand_rec(_form.offset() + 1, start, end, dtrs, result))
                return true;
        }
        else
        {
            // iterate over all left adjacent input items matching the string
            // _orth[arg_pos]
            for(adj_iterator padj =
                    _container->begin(adj_iterator::left, start,
                                      _form.stem()->orth(arg_pos));
                padj.valid(); padj++)
            {
                dtrs[arg_pos] = *padj;
                if(expand_rec(arg_pos - 1, (*padj)->start(), end, dtrs,
                              result))
                    return true;
            }
        }
    }
    else
    {				        // expand to the right
        if(arg_pos == _form.stem()->length())
        {
            // we managed to match an entry completely
            if(add_result(start, end, _form.stem()->length(),
                          _form.offset(), dtrs, result))
                return true;
        }
        else
        {
            // iterate over all right adjacent input items matching the string
            // _orth[arg_pos]
            for(adj_iterator padj =
                    _container->begin(adj_iterator::right, end,
                                      _form.stem()->orth(arg_pos));
                padj.valid(); padj++)
            {
                dtrs[arg_pos] = *padj;
                if(expand_rec(arg_pos + 1, start, (*padj)->end(), dtrs,
                              result))
                    return true;
            }
        }
    }
    return false;
}

void
input_token::expand(list<tLexItem *> &result)
{
    if(_form.valid())
    {
        // there is a lex_entry corresponding to tok input token: try to create
        // an input item for the chart. input_token without valid entry can be
        // used in multi-word lexemes
        if(_form.stem()->length() == _end - _start)
        {
            input_token **dtrs = new input_token* [1];
            dtrs[0] = this;
            add_result(_start, _end, 1, 0, dtrs, result);
            delete[] dtrs;  
        }
        else
        {
            input_token **dtrs = new input_token* [_form.stem()->length()];
            dtrs[_form.offset()] = this;
            expand_rec(_form.offset() - 1, _start, _end, dtrs, result);
            delete[] dtrs;  
        }
    }
}

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
#include "options.h"
#include "grammar.h"
#include "parse.h"
#include "item.h"
#include "tokenizer.h"
#include "inputchart.h"

inline bool
skip_ws(const string &line, string::size_type &position)
{
    return(position = line.find_first_not_of(" \t", position)) != STRING_NPOS;
}

bool
get_int(const string &line, int &number, string::size_type &position)
{
    string::size_type curr = position;

    if(skip_ws(line, position) && isdigit(line[position]))
    {
        string::size_type pend = line.find_first_not_of("0123456789",
                                                        position);
        string s = line.substr(position, (STRING_NPOS == pend) ?
                               pend : pend - position);
        number = strtol(s.c_str(), NULL, 10);
        position = pend;
        return true;
    }
    position = curr;
    return false;
}

bool
get_float(const string &line, float &number, string::size_type &position)
{
    string::size_type curr = position;

    if(skip_ws(line, position) && (isdigit(line[position])
                                   || line[position] == '.'))
    {
        string::size_type pend =
            line.find_first_not_of("0123456789eE+-.", position);
        string s = line.substr(position, (STRING_NPOS == pend) ?
                               pend : pend - position);
        number = atof(s.c_str());
        if((0 != number) || (s.find_first_not_of('0') == STRING_NPOS))
        {
            position = pend;
            return true;
        }
    }
    position = curr;
    return false;
}

bool
get_string(const string &line, string &result, string::size_type &position)
{

    string::size_type curr = position;

    if(skip_ws(line, position))
    {
        string::size_type pend = line.find_first_of(" ()\t", position);
        if(pend > position)
        {
            result =
                line.substr(position,
                            (STRING_NPOS == pend) ? pend : pend - position);
            position = pend;
            return true;
        }
    }
    position = curr;
    return false;
}

bool
get_special_char(const string &line, char c, string::size_type &position)
{
    string::size_type curr = position;

    if(skip_ws(line, position) && (line[position] == c))
    {
        position++;
        return true;
    }

    position = curr;
    return false;
}

inline bool
get_open_paren(const string &line, string::size_type &position)
{
    return get_special_char(line, '(', position);
}

inline bool
get_close_paren(const string &line, string::size_type &position)
{
    return get_special_char(line, ')', position);
}

void
input_chart::populate(tokenizer *t)
{
    clear();
    t->add_tokens(this);
    delete t;
}

input_token *
input_chart::add_token(input_token *p_item)
{
    _positionmap->add_start_position(p_item->startposition());
    _positionmap->add_end_position(p_item->endposition());
    _inputtokens.push_back(p_item);
    return p_item;
}

/*
This constructor of add_token gets a string with a lexical specification 
of the following form:

<start> <end> <instance> <orth> <inflection> <neg-log-probability>
   \{ ( <feature>\{.<feature>\}* <type-name> ) \}*

*/

input_token *
input_chart::add_token(const string &tokenstring)
{
    int start, end;
    float prob;
    string orth, instance_name, affix_name;

    // first the necessary fields:
    // start end instance_name orthography probability

    string::size_type position = 0;
    if(get_int(tokenstring, start, position)
       && get_int(tokenstring, end, position)
       && get_string(tokenstring, instance_name, position)
       && get_string(tokenstring, orth, position)
       && get_string(tokenstring, affix_name, position)
       && get_float(tokenstring, prob, position))
    {

        int instance = lookup_type(instance_name.c_str());
        lex_stem *stem = Grammar->lookup_stem(instance);

        int affix_nr = lookup_type(affix_name.c_str());
        string path, value;

        // now we have to process the (path_with_dots value) pairs
        modlist *mods = New modlist;

        while(get_open_paren(tokenstring, position))
        {
            if(get_string(tokenstring, path, position)
               && get_string(tokenstring, value, position)
               && get_close_paren(tokenstring, position))
                mods->push_back(pair<string,int>
                                (path, lookup_symbol(value.c_str())));
            else
            {
                cerr << "Wrong modifier in input token: \""
                     << tokenstring.substr(position) << "\"" << endl;
                return 0;
            }
        }

        if(mods->empty())
        {
            delete mods;
            mods = 0;
        }

        list_int *affixes = cons(affix_nr, 0);
        input_token *new_token =
            New input_token(0, start, end, full_form(stem, *mods, affixes),
                            orth, prob, postags(), tPaths(),
                            this);
        free_list(affixes);

        return add_token(new_token);
    }
    else
    {
        cerr << "Wrong obligatory argument in input token: \""
             << tokenstring.substr(position) << "\"" << endl;
        return 0;
    }
}

/* 
if the instance specified by an input_token is a multi-word lexeme, the
input is checked for the existence of the appropriate string. This may result
in more than one item.  After expansion of an items instance, the expanded
types are put under the specified paths, and finally, the inflectional rule is
applied. If fs creation succeeds, the items probability is computed on the
basis of the input item probabilities and a new parser task is created
*/

class less_than_topo
{
public:
    bool operator() (lex_item *i, lex_item *j)
    {
        if(i->start() == j->start())
            return i->end() < j->end();
        else
            return i->start() < j->start();
    }
};

input_chart::gaplist
input_chart::gaps(int max, list<lex_item *> &input)
{
    if(verbosity > 4)
        fprintf(ferr, "finding gaps (0 - %d):", max);

    // sort by ascending start position resp. equal start, smaller end, results
    // in a topologically sorted chart graph
    input.sort(less_than_topo());

    short int *active = New short int[max + 1];
    for(int i = 0; i <= max; i++)
        active[i] = i;

    // after that loop, the following holds: active[j] is the smallest node
    // number reachable from j
    for(list<lex_item *>::iterator it = input.begin(); it != input.end();
        it++)
    {
        if(active[(*it)->start()] < active[(*it)->end()])
            active[(*it)->end()] = active[(*it)->start()];
    }
  
    int gapend = active[max];
    int gapstart;

    gaplist result;

    while(gapend > 0)
    {
        gapstart = gapend;
      
        while(gapstart > 0 && active[gapstart] == gapstart)
            gapstart--;

        if(verbosity > 4)
            fprintf(ferr, " <%d, %d>", gapstart, gapend);
        result.push_back(make_pair(gapstart, gapend));

        if(gapstart > 0)
            gapend = active[gapstart];
        else
            gapend = 0;
    }

    if(verbosity > 4)
        fprintf(ferr, "\n");

    delete[] active;

    return result;
}

set<string>
input_chart::forms(int p1, int p2)
{
    set<string> result;
    for(input_chart::iterator it = begin(); it != end(); it++)
        if(((*it)->start() >= p1) && ((*it)->end() <= p2))
        {
            result.insert((*it)->orth());
        }

    return result;
}

string
input_chart::uncovered(const gaplist &gaps)
{
    string result;

    for(gaplist::const_iterator g = gaps.begin(); g != gaps.end(); ++g)
    {
        set<string> spellings(forms(g->first, g->second));
        for(set<string>::iterator it = spellings.begin();
            it != spellings.end(); ++it)
        {
            if(!result.empty())
                result += ", ";

            result += "\"" + *it + "\"";
        }
    }
	    
    return result;
}

void
merge_generic_les(list<lex_item *> &res, list<lex_item *> &add)
{
    for(list<lex_item *>::iterator it = add.begin(); it != add.end(); ++it)
    {
        if((*it)->score() == 0.0) continue;
        bool good = true;
        for(list<lex_item *>::iterator it2 = res.begin();
            it2 != res.end(); ++it2)
            if(same_lexitems(**it, **it2))
            {
                if((*it)->score() > (*it2)->score())
                    (*it2)->score((*it)->score());
                good = false;
                break;
            }
        if(good)
            res.push_back(*it);
    }
}

list<lex_item *>
input_chart::cover_gaps(const gaplist &gaps)
{
    list<lex_item *> results;

    for(gaplist::const_iterator g = gaps.begin(); g != gaps.end(); ++g)
    {
        for(input_chart::iterator it = begin(); it != end(); it++)
            if(((*it)->start() >= g->first) && ((*it)->end() <= g->second))
            {
                list<lex_item *> gens = (*it)->generics(0);
                merge_generic_les(results, gens);
            }
    }

    assign_positions();
    return results;
}

list<lex_item *>
spanning_native_les(list<lex_item *> &les, int start, int end)
{
    list<lex_item *> res;

    for(list<lex_item *>::iterator it = les.begin(); it != les.end(); ++it)
        if((*it)->start() <= start && (*it)->end() >= end
           && !(*it)->synthesized())
            res.push_back(*it);

    return res;
}

void
input_chart::add_generics(list<lex_item *> &input)
{
    if(verbosity > 4)
        fprintf(ferr, "adding generic les\n");

    for(input_chart::iterator it = begin(); it != end(); it++)
    {
        if((*it)->synthesized())
            continue;

        if(verbosity > 4)
        {
            fprintf(ferr, "  token ");
            (*it)->print(ferr);
            fprintf(ferr, "\n");
        }

        list<lex_item *> gens;
        list<lex_item *> les = spanning_native_les(input, (*it)->start(),
                                                   (*it)->end());

        if(verbosity > 4)
            fprintf(ferr, "    found %d les for this position\n", les.size());

        if(les.empty())
        {
            gens = (*it)->generics(0);
        }
        else if(cheap_settings->lookup("pos-completion"))
        {
            postags missing((*it)->get_in_postags());

            if(verbosity > 4)
            {
                fprintf(ferr, "    token provides tags:");
                missing.print(ferr);
                fprintf(ferr, "\n    already supplied:");
                postags(les).print(ferr);
                fprintf(ferr, "\n");
            }

            missing.remove(postags(les));

            if(verbosity > 4)
            {
                fprintf(ferr, "    -> missing tags:");
                missing.print(ferr);
                fprintf(ferr, "\n");
            }
            if(!missing.empty())
                gens = (*it)->generics(strtoint(
                    cheap_settings->req_value("discount-gen-le-priority"),
                    "as value of discount-gen-le-priority"), missing);
        }
        if(!gens.empty())
            merge_generic_les(input, gens);
    }

    assign_positions();
}

void
dependency_filter(list <lex_item *> &result, struct setting *deps,
                  bool unidirectional)
{
    if(deps == 0 || opt_chart_man == false)
        return;

    vector<set <int> > satisfied(deps->n);
    multimap<lex_item *, pair <int, int> > requires;

    lex_item *lex;
    fs f;

    for(list < lex_item * >::iterator it = result.begin();
        it != result.end(); it++)
    {
        lex = *it;
        f = lex->get_fs();
    
        if(verbosity > 4)
            fprintf(fstatus, "dependency information for %s:\n",
                    lex->description().c_str());
    
        for(int j = 0; j < deps->n; j++)
        {
            fs v = f.get_path_value(deps->values[j]);
            if(v.valid())
            {
                if(verbosity > 4)
                    fprintf(fstatus, "  %s : %s\n", deps->values[j], v.name());

                if(!unidirectional || j % 2 != 0)
                {
                    satisfied[j].insert(v.type());
                }
        
                if(!unidirectional || j % 2 == 0)
                {
                    requires.insert(make_pair(lex,
                                              make_pair((j % 2 == 0)
                                                        ? j + 1 : j - 1,
                                                        v.type())));
                }
            }
        }
    }
  
    list<lex_item *> filtered;
  
    for(list<lex_item *>::iterator it = result.begin();
        it != result.end(); ++it)
    {
        lex = *it;

        pair<multimap<lex_item *, pair<int, int> >::iterator,
            multimap<lex_item *, pair<int, int> >::iterator> eq =
            requires.equal_range(lex);

        bool ok = true;
        for(multimap<lex_item *, pair<int, int> >::iterator dep = eq.first;
            ok && dep != eq.second; ++dep)
        {
            // we have to resolve a required dependency
            pair<int, int> req = dep->second;
            if(verbosity > 4)
                fprintf(fstatus, "`%s' requires %s at %d -> ",
                        lex->description().c_str(),
                        typenames[req.second], req.first);
       
            bool found = false;
            for(set<int>::iterator it2 = satisfied[req.first].begin();
                it2 != satisfied[req.first].end(); ++it2)
            {
                if(glb(*it2, req.second) != -1)
                {
                    if(verbosity > 4)
                        fprintf(fstatus, "[%s]", typenames[*it2]);
                    found = true;
                    break;
                }
            }
       
            if(!found)
            {
                ok = false;
                stats.words_pruned++;
            }

            if(verbosity > 4)
                fprintf(fstatus, "%s satisfied\n", ok ? "" : "not");
        }
    
        if(ok)
            filtered.push_back(lex);
    }
  
    result = filtered;
}

void
input_chart::assign_position(input_token *t)
{
    t->start(_positionmap->chart_start_position(t->startposition()));
    t->end(_positionmap->chart_end_position(t->endposition()));
}

void
input_chart::assign_positions()
{
    // assign chart positions to the input tokens
    for(list<input_token *>::iterator it = _inputtokens.begin();
        it != _inputtokens.end(); it++)
        assign_position(*it);
}

int
input_chart::expand_all(list<lex_item *> &result)
{
    assign_positions();

    // expand each input token
    for(list<input_token *>::iterator it = _inputtokens.begin();
        it != _inputtokens.end(); it++)
    {
        (*it)->expand(result);
    }

    return _positionmap->max_chart_position();
}

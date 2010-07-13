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

/* parser for files in TDL syntax */

/* straightforward hand implemented top down parser, builds simple internal
   representation that closely resembles the BNF */

#include "flop.h"
#include "hierarchy.h"
#include "options.h"
#include "lex-tdl.h"
#include "utility.h"
#include "list-int.h"
#include "logging.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>

using std::string;
using boost::algorithm::to_upper;
using boost::algorithm::to_lower;
using boost::lexical_cast;

int default_status = NO_STATUS; /* for section-wide default status values */

void tdl_start(int toplevel);

int template_mode = 0;
/* true when we are inside a template definition/call, so that we can support
   attribute names in template parameters. the idea is to delay inserting
   names into the type table until a the template is actually called */

void tdl_domainname() {
  if(consume_if(T_COLON)) {
    if(! consume_if(T_ID))
      syntax_error("expecting domain name", LA(0));
  }
  else if(consume_if(T_QUOTE)) {
    if(! consume_if(T_ID))
      syntax_error("expecting domain name", LA(0));
  }
  else if(! consume_if(T_STRING))
    syntax_error("expecting domain name", LA(0));
}

int tdl_opt_inst_status()
{
  string statusname;
  
  if(LA(0)->tag == T_COLON && is_keyword(LA(1), "status"))
    {
      consume(2);
      statusname = match(T_ID, "status value", false);
    }

  if(!statusname.empty())
    {
      to_lower(statusname);
      if(statustable.id(statusname) == -1)
        statustable.add(statusname);
      return statustable.id(statusname);
    }
  else
    return NO_STATUS;
}

int tdl_option(bool readonly) {
// returns value of last `status option, if any
  string statusname;

  if(LA(0)->tag != T_KEYWORD) {
    syntax_error("option expected", LA(0));
  } else {
    if(is_keyword(LA(0), "status")) {
      consume(1);
      match(T_COLON, "`:' after `status'", true);
      
      statusname = match(T_ID, "status value", readonly);
    } else {
      syntax_error("unknown option", LA(0));
      consume(1);
    }
  }
  
  if(!statusname.empty()) {
    to_lower(statusname);
    if(statustable.id(statusname) == -1)
      statustable.add(statusname);
    
    return statustable.id(statusname);
  }
  else
    return NO_STATUS;
}


void tdl_subtype_def(const std::string& sname, const std::string& printname) {
  int nr = 0;
  Type *subt = NULL;
  bool readonly = false;
  string name = sname;
  while(consume_if(T_ISA)) {
    string super = match(T_ID, "name of supertype", readonly);
    
    if(!super.empty()) {
      // name ISA super
      to_lower(super);
      
      int sup;
      Type *supt;
      
      int sub = types.id(name); sup = types.id(super);
      
      if(!readonly) {
        if(sub == -1) {
          subt = new_type(name, false);
          subt->def = LA(0)->loc;
        } else {
          subt = types[sub];
          if(subt->implicit == false) {
            if(!allow_redefinitions)
              LOG(logSyntax, WARN,
                  LA(0)->loc.fname << ":" << LA(0)->loc.linenr 
                  << ": warning: redefinition of `" << types.name(sub) << "'");
            undo_subtype_constraints(subt->id);
            
            subt->constraint = NULL;
          }
        }
        subt->printname = printname;
        subt->implicit = false;
        
        if(sup == -1) {
          supt = new_type(super, false);
          supt->def = LA(0)->loc;
          supt->implicit = true;
        } else {
          supt = types[sup];
        }
        subtype_constraint(subt->id, supt->id);
        subt->parents.push_back(supt->id);
      }
    }
    ++nr;
    name = super;
  }
  
  if(nr < 1) syntax_error("at least one supertype expected", LA(0));
  
  while(consume_if(T_COMMA)) {
    int status;
    status = tdl_option(readonly);
    if(!readonly && status != NO_STATUS) {
      if(nr > 1) {
        syntax_error("not sure about the semantics of this - ignoring", LA(0));
      } else {
        subt->status = status;
        subt->defines_status = true;
      }
    }
  }
}

Param *tdl_templ_par(Coref_table *, bool readonly);
void tdl_template_def(const std::string& name);

string tdl_attribute(Coref_table *co, bool readonly)
{
    string s;
    if(LA(0)->tag == T_DOLLAR) { 
        Param *par = tdl_templ_par(co, readonly);

        if(par->value != NULL) {
            syntax_error("unexpected assignment to template parameter within body",
                LA(0));
        }

        s = "$" + par->name;
    } else {
        s = match(T_ID, "attribute", readonly);
        to_upper(s);

        if(attributes.id(s) == -1)
            attributes.add(s);
    }
    return s;
}

Conjunction *tdl_conjunction(Coref_table *, bool readonly);
Conjunction *tdl_add_conjunction(Coref_table *, Conjunction *, bool readonly);

Attr_val *tdl_attr_val(Coref_table *co, bool readonly) {
  Attr_val *av = NULL, *av_inner = NULL, *av_outer = NULL;

  string attribute = tdl_attribute(co, readonly);

  if(!readonly) {
    av = new Attr_val();
    av->attr = attribute;
    av_inner = av_outer = av;
  }

  while(consume_if(T_DOT)) {
    // build a "path" of attribute-value structs, av_inner is the Attr_val
    // struct at the bottom, av_outer at the top of the path
    Term *T = NULL;
    
    attribute = tdl_attribute(co, readonly);
    if(!readonly) {
      av_inner = new Attr_val();
      av_outer->val = new Conjunction();
      T = add_term(av_outer->val, new_avm_term());
      T->A->av.clear();
      T->A->av.push_back(av_inner);
      av_inner->attr = attribute;
      av_outer = av_inner;
    }
  }

  Conjunction *conj = tdl_conjunction(co, readonly); 
  if(!readonly)
    av_inner->val = conj;

  return av;
}

Avm *tdl_feature_term(Coref_table *co, bool readonly)
{
  Avm *A = NULL;

  // T_LBRACKET already consumed !
  if(!readonly) A = new Avm();
  
  if(LA(0)->tag != T_RBRACKET) {
    Attr_val *av;
    do {
      av = tdl_attr_val(co, readonly);
      if(!readonly)
        A->add_attr_val(av);
    }
    while(consume_if(T_COMMA));
  }

  match(T_RBRACKET, "`]' at the end of feature term", true);

  return A;
}

Tdl_list *tdl_diff_list(Coref_table *co, bool readonly) {
  Tdl_list *L = NULL;

  // T_LDIFF already consumed !!
  if(!readonly) {
    L = new Tdl_list();
    L->difflist = 1;
  }

  if(LA(0)->tag != T_RDIFF) {
    Conjunction *conj;
    do {
      conj = tdl_conjunction(co, readonly);
      if(!readonly)
        L->add_conjunction(conj);
    }
    while(consume_if(T_COMMA));
  }

  match(T_RDIFF, "`!>' at the end of difference list", true);

  return L;
}

Tdl_list *tdl_list(Coref_table *co, bool readonly) {
  Tdl_list *L = NULL;
  int openlist = 0;

  // T_LANGLE already consumed 
  if(!readonly) L = new Tdl_list();

  if(LA(0)->tag != T_RANGLE) {
    Conjunction *conj = tdl_conjunction(co, readonly);
    if(!readonly)
      L->add_conjunction(conj);
      
    while(consume_if(T_COMMA)) {
        if(consume_if(T_DOT)) {
          match(T_DOT, "`...' - got `.'", true);
          match(T_DOT, "`...' - got `..'", true);
          openlist = 1;
        } else {
          conj = tdl_conjunction(co, readonly);
          if(!readonly)
            L->add_conjunction(conj);
        }
    }
      
    if(!openlist) {
      if(consume_if(T_DOT)) { // dotted pair at the end
        conj = tdl_conjunction(co, readonly);
        if(!readonly) {
          L->dottedpair = 1;
          L->rest = conj;
        }
      }
    }

    if(!readonly) L->openlist = openlist;
  }

  match(T_RANGLE, "`>' at the end of list", true);

  return L;
}

int tdl_coref(Coref_table *co, bool readonly) {
  // T_HASH already consumed !!
  string name = match(T_ID, "coreference name", readonly);
  if(!name.empty())
    return add_coref(co, name);

  return -1;
}

Param *tdl_templ_par(Coref_table *co, bool readonly) {
  Param *p = NULL;

  match(T_DOLLAR, "`$' at start of templ_par", true);

  string name = match(T_ID, "templ-var", readonly);
  if(!readonly) {
    p = new Param(name);
  }
  
  if(consume_if(T_EQUALS)) {
    Conjunction *conj = tdl_conjunction(co, readonly);
    if(!readonly)
      p->value = conj;
  }  
  return p;
}

Param_list *tdl_templ_par_list(Coref_table *co, bool readonly)
{
  Param_list *pl = NULL;

  int save = template_mode;
  template_mode = 1;

  if(!readonly) {
    pl = new Param_list();
  }

  match(T_LPAREN, "`(' starting template parameter list", true);

  if(LA(0)->tag != T_RPAREN) {
    do {
      Param *par = tdl_templ_par(co, readonly);
      if(!readonly) {
        pl->param.push_back(par);
        }
    } while (consume_if(T_COMMA)) ;
  }

  match(T_RPAREN, "`)' at the end of  template parameter list", true);

  template_mode = save;

  return pl;
}

Templ *tdl_templ_call(Coref_table *co, bool readonly)
{
  Templ *t = NULL;
  // T_AT already consumed !!

  string name = match(T_ID, "template name", readonly);
  Param_list *params = tdl_templ_par_list(co, readonly);

  if(!readonly) {
    t = new Templ();
    t->name = name;
    if(templates.id(t->name) == -1) {
      LOG(logSyntax, WARN, LA(0)->loc.fname << ":" << LA(0)->loc.linenr
          << ": warning: call to undefined template `" << t->name << "'");
    }
    t->params = params;
    t->constraint = NULL;
  }
  
  return t;
}


Term *tdl_term(Coref_table *co, bool readonly)
{
  Term *t = NULL;

  if(!readonly) t = new Term();

  if(LA(0)->tag == T_ID) { // type name
    if(!readonly) {
      int id = -1;
      Type *typ = NULL;
      
      t->tag = TYPE;
      t->value = LA(0)->text;
      to_lower(t->value);
      
      id = types.id(t->value);
      if(id == -1 && !template_mode) {
        typ = new_type(t->value, false);
        typ->def = LA(0)->loc;
        id = typ->id;
        typ->implicit = true;
      }
      
      t->type = id;
    }
    
    consume(1);
  }
  else if(consume_if(T_QUOTE)) { // atom
    string value = match(T_ID, "atom expected (term)", readonly);
    if(!readonly) {
      t->tag = ATOM;
      t->value = value;
      string printname = t->value;
      to_lower(t->value);
      
      string s = "'" + t->value;
      if(types.id(s) == -1) {
        Type *ty = new_type(s, false);
        subtype_constraint(ty->id, BI_SYMBOL);
        ty->status = ATOM_STATUS;
        ty->printname = printname;
      }
    }
  }
  else if(LA(0)->tag == T_STRING) { // atom
    if(!readonly) {
      t->tag = STRING;
      t->value = LA(0)->text;
      
      string s = "\"" + t->value + "\"";
      
      if(types.id(s) == -1) {
        Type *ty = new_type(s, false);
        ty->def = LA(0)->loc;
        subtype_constraint(ty->id, BI_STRING);
        ty->status = ATOM_STATUS;
        ty->printname = t->value;
      }
    }
    
    consume(1);
  }
  else if(consume_if(T_LBRACKET)) {
    Avm *new_avm = tdl_feature_term(co,readonly);
    if(!readonly) {
      t->tag = FEAT_TERM;
      t->A = new_avm;
    }
  }
  else if(consume_if(T_LDIFF)) {
    Tdl_list *new_list = tdl_diff_list(co, readonly);
    if(!readonly) {
      t->tag = DIFF_LIST;
      t->L = new_list;
    }
  }
  else if(consume_if(T_LANGLE)) {
    Tdl_list *new_list = tdl_list(co,readonly);
    if(!readonly) {
      t->tag = LIST;
      t->L = new_list;
    }
  }
  else if(consume_if(T_HASH)) {
    int coref = tdl_coref(co,readonly);
    if(!readonly) {
      t->tag = COREF;
      t->coidx = coref;
    }
  }
  else if(consume_if(T_AT)) {
    Templ *temp = tdl_templ_call(co, readonly);
    if(!readonly) {
      t->tag = TEMPL_CALL;
      t->value = temp->name;
      t->params = temp->params;
    }
  }
  else if(LA(0)->tag == T_DOLLAR) {
    Param *par = tdl_templ_par(co, readonly);
    if(!readonly) {
      if(par->value != NULL)
        syntax_error("unexpected assignment to template parameter within body",
                     LA(0));
      
      t->tag = TEMPL_PAR;
      t->value = par->name;
    }
  }
  else {
    t = NULL;
    syntax_error("expecting term", LA(0));
  }

  return t;
}


Conjunction *tdl_conjunction(Coref_table *co, bool readonly) {
  return tdl_add_conjunction(co, NULL, readonly);
}

Conjunction *
tdl_add_conjunction(Coref_table *co, Conjunction *con, bool readonly)
{
  if(!readonly && con == NULL)
    con = new Conjunction();

  Term * currterm;
  do {
    currterm = tdl_term(co, readonly);
    if (!readonly)
      add_term(con, currterm);
  } while(consume_if(T_AMPERSAND));

  return con;
}

void tdl_opt_inflr(Type *t)
{
  while(LA(0)->tag == T_INFLR)
    {
      LOG(logSyntax, DEBUG,
          "inflr <" << (t == 0 ? "(global)" : types.name(t->id))
          << ">: `" << LA(0)->text << "'");

      if(t == 0)
        global_inflrs += LA(0)->text;
      else
        t->inflr += LA(0)->text;

      consume(1);
    }
}


void tdl_avm_constraints(Type *t, bool is_instance, bool readonly) {
  int status = NO_STATUS;

  if(!readonly)
    {
      t->constraint = tdl_add_conjunction(t->coref, t->constraint, readonly);
    }
  else
    tdl_conjunction(NULL, readonly);

  // process TDL rule definition code, if present
  if (LA(0)->tag == T_ARROW)
    {
      consume(1);
      if (LA(0)->tag != T_LANGLE)
        syntax_error("rule definintion not followed by a list", LA(0));

      if (! readonly)
        {
          // read the list and build the ARGS attribute value pair
          Attr_val *attrval = new Attr_val();
          attrval->val = new Conjunction();
          add_term(attrval->val, tdl_term(t->coref, readonly));
          string s("ARGS");
          attrval->attr = s;
          Avm *A = new Avm();
          A->add_attr_val(attrval);
          Term *T = new Term();
          T->tag = FEAT_TERM;
          T->A = A;
          add_term(t->constraint, T);
        }
      else
        tdl_term(NULL, readonly);
    }

  while(LA(0)->tag == T_COMMA)
    {
      consume(1);
      status = tdl_option(readonly);
    }
  
  if(status == NO_STATUS && is_instance)
    status = default_status;
  
  if(!readonly && t)
    {
      t->status = status;
      if(status != NO_STATUS) t->defines_status = 1;
    }
}


void tdl_avm_def(const std::string& name, const std::string& printname, bool is_instance, bool readonly)
{
  int t_id;
  Type *t = NULL;

  match(T_ISEQ, "avm definition starting with `:='", true);

  if(!readonly)
    {
      t_id = types.id(name);
      
      if(t_id != -1)
        {
          t = types[t_id];
          
          if(t->implicit == false)
            {
              if(!allow_redefinitions)
                LOG(logSyntax, WARN,
                    LA(0)->loc.fname << ":" << LA(0)->loc.linenr
                    << ": warning: redefinition of `" << name << "'");
              
              undo_subtype_constraints(t->id);

            }
        }
      else
        {
          t = new_type(name, is_instance);
        }

      t->def = LA(0)->loc;
      t->implicit = false;
      t->printname = printname;

      t->coref = new Coref_table();
      t->constraint = NULL;
    }
  
  tdl_opt_inflr(t);

  tdl_avm_constraints(t, is_instance, readonly);
}


/*
 definition questions:
 1. can this be done for instances?
    i assume no: this is assured in tdl_type_def
 2. must the type exist, or is it simply created if it is not there?
    i define it and issue a warning
 3. can it be used with infl rules
    i assume not
 4. can rule syntax be used?
    i assume yes
 The location of the addition is lost, might affect grammar debugging
*/
void tdl_avm_add(const std::string& name, const std::string& printname, bool is_instance, bool readonly)
{
  int t_id;
  Type *t = NULL;

  match(T_ISPLUS, "avm definition starting with `:+'", true);

  if(!readonly)
    {
      t_id = types.id(name);
      
      if(t_id != -1)
        {
          t = types[t_id];
          // make all old coref names distinct from possibly new ones
          new_coref_domain(t->coref);
        }
      else
        {
          LOG(logSyntax, WARN, LA(0)->loc.fname << ":" << LA(0)->loc.linenr
              << ": warning: defining type `" << name << "' with `:+'");
              
          t = new_type(name, is_instance);

          t->def = LA(0)->loc;
          t->implicit = false;
          t->printname = printname;

          t->coref = new Coref_table();
        }
    }
  
  //  tdl_opt_inflr(t);

  tdl_avm_constraints(t, is_instance, readonly);
}

void tdl_type_def(bool instance) {
  string name;
  string printname;
  bool readonly = false;
  const char *what = (instance ? "instance name" : "type name");

  if(LA(0)->tag == T_STRING) {
    name = match(T_STRING, what, readonly);
    string s = "\"" + name + "\"";
    printname = name;
    name = s;
  } else {
    name = match(T_ID, what, readonly);
    printname = name;
    to_lower(name);
  }
  
  if (! instance) {
    if(LA(0)->tag == T_ISEQ) {
      tdl_avm_def(name, printname, false, readonly);
    }
    else if(LA(0)->tag == T_ISPLUS) {
      tdl_avm_add(name, printname, false, readonly);
    }
    else if(LA(0)->tag == T_ISA) {
      tdl_subtype_def(name, printname);
    }
    else {
      syntax_error("avm or subtype definition expected", LA(0));
      recover(T_DOT);
    }
  } else {
    string iname = "$" + name;
    tdl_avm_def(iname, printname, true, readonly);
  }

  match(T_DOT, (instance 
                ? "`.' at end of instance definition"
                : "`.' at end of type definition")
        , true);
}


void tdl_template_def(const std::string& name)
{
  int old = -1;
  template_mode = 1;

  Templ* t = new Templ();
  t->name = name;
  t->calls = 0;

  if((old = templates.id(t->name)) >= 0)
    {
      if(!allow_redefinitions)
        LOG(logSyntax, WARN, LA(0)->loc.fname << ":" << LA(0)->loc.linenr 
            << ": warning: redefinition of template `" << t->name << "'");
      templates[old] = t;
    }
  else
    {
      templates[templates.add(t->name)] = t;
    }

  t->loc = LA(0)->loc;
  t->params = tdl_templ_par_list(NULL, false);

  match(T_ISEQ, "`:=' in template definition", true);
  
  t->coref = new Coref_table();

  t->constraint = tdl_conjunction(t->coref, false);

  while(LA(0)->tag == T_COMMA)
    {
      consume(1);
      tdl_option(true);
    }

  template_mode = 0;
}

void block_not_closed(const std::string& kind, const std::string& fname, int lnr)
{
    string msg = "missing `end' for " + kind + " block starting at " + fname + ":"
        + lexical_cast<string>(lnr);
    syntax_error(msg.c_str(), LA(0));
}

void tdl_block()
{

  static int nesting = 0;

  string b_fname = curr_fname();
  int b_lnr = curr_line();

  nesting++;

  consume(1); // `begin`
  
  match(T_COLON, "`:'", true);
  
  if(LA(0)->tag != T_KEYWORD)
    {
      syntax_error("expecting a keyword to start block", LA(0));
      recover(T_KEYWORD); // futile attempt ...
    }

  if(is_keyword(LA(0), "control"))
    {
      consume(1); match(T_DOT, "`.'", true);

      syntax_error("tdl control block not implemented", LA(0));

      recover(T_DOT);
    }
  else if(is_keyword(LA(0), "declare"))
    {
      consume(1); match(T_DOT, "`.'", true);

      syntax_error("tdl declare block not implemented", LA(0));

      recover(T_DOT);
    }
  else if(is_keyword(LA(0), "domain"))
    {
      consume(1);
      
      tdl_domainname();
      match(T_DOT, "`.'", true);
      
      tdl_start(0);

      consume_if(T_COLON);
      
      if(!is_keyword(LA(0), "end"))
        {
          block_not_closed("domain", b_fname, b_lnr);
        }
      else
        {
          match_keyword("end");
          match(T_COLON, "`:'", true);
          match_keyword("domain");
          tdl_domainname();
          match(T_DOT, "`.'", true);
        }
    }
  else if (is_keyword(LA(0), "instance"))
    {
      consume(1);

      int status, saved_status;

      status = tdl_opt_inst_status();

      saved_status = default_status;
      if(status != NO_STATUS)
        default_status = status;

      match(T_DOT, "`.'", true);

      do
        if(LA(0)->tag == T_ID || LA(0)->tag == T_STRING)
          {
            tdl_type_def(true);
          }
        else if(is_keyword(LA(0), "end") ||
                (LA(0)->tag == T_COLON && is_keyword(LA(1), "end")))
          {
            break;
          }
        else if(LA(0)->tag == T_INFLR)
          {
            tdl_opt_inflr(0);
          }
        else if(LA(0)->tag == T_EOF)
          {
            break;
          }
        else
          {
            tdl_start(0);
          }
      while (1);

      consume_if(T_COLON);
      
      if(!is_keyword(LA(0), "end"))
        {
          block_not_closed("instance", b_fname, b_lnr);
        }
      else
        {
          match_keyword("end");
          match(T_COLON, "`:'", true);
          match_keyword("instance");
          match(T_DOT, "`.'", true);
        }

      default_status = saved_status;
    }
  else if (is_keyword(LA(0), "lisp"))
    {
      consume(1); match(T_DOT, "`.'", true);

      lisp_mode = 1;

      while(consume_if(T_LISP))
        {
        }
      
      lisp_mode = 0;

      consume_if(T_COLON);

      if(!is_keyword(LA(0), "end"))
        {
          block_not_closed("lisp", b_fname, b_lnr);
        }
      else
        {
          match_keyword("end");
          match(T_COLON, "`:'", true);
          match_keyword("lisp");
          match(T_DOT, "`.'", true);
        }
    }
  else if (is_keyword(LA(0), "template"))
    {
      consume(1); match(T_DOT, "`.'", true);

      do
        if(LA(0)->tag == T_ID)
          {
            string name = match(T_ID, "template name", false);
            tdl_template_def(name);
            match(T_DOT, "`.' at the end of template definition", true);
          }
        else if(is_keyword(LA(0), "end") ||
                (LA(0)->tag == T_COLON && is_keyword(LA(1), "end")))
          {
            break;
          }
        else if (LA(0)->tag == T_EOF)
          {
            break;
          }
        else
          {
            tdl_start(0);
          }
      while (1);

      consume_if(T_COLON);
            if(!is_keyword(LA(0), "end"))
        {
          block_not_closed("template", b_fname, b_lnr);
        }
      else
        {
          match_keyword("end");
          match(T_COLON, "`:'", true);
          match_keyword("template");
          match(T_DOT, "`.'", true);
        }
    }
  else if (is_keyword(LA(0), "type"))
    {
      consume(1); match(T_DOT, "`.'", true);

      do
        if(LA(0)->tag == T_ID || LA(0)->tag == T_STRING)
          {
            tdl_type_def(false);
          }
        else if(is_keyword(LA(0), "end") ||
                (LA(0)->tag == T_COLON && is_keyword(LA(1), "end")))
          {
            break;
          }
        else if (LA(0)->tag == T_EOF)
          {
            break;
          }
        else if(LA(0)->tag == T_INFLR)
          {
            tdl_opt_inflr(0);
          }
        else
          {
            tdl_start(0);
          }
      while (1);

      consume_if(T_COLON);

      if(!is_keyword(LA(0), "end"))
        {
          block_not_closed("type", b_fname, b_lnr);
        }
      else
        {
          match_keyword("end");
          match(T_COLON, "`:'", true);
          match_keyword("type");
          match(T_DOT, "`.'", true);
        }
    }
  else
    {
      syntax_error("unknown/illegal type of block", LA(0));
      recover(T_DOT);
    }

  nesting --;
}

void tdl_defdomain_option()
{
  match(T_COLON, "`:'", true);
  
  if(is_keyword(LA(0), "errorp"))
    {
      consume(1);
      
      match(T_ID, "`t' or `nil'", true);
    }
  else if (is_keyword(LA(0), "delete-package-p"))
    {
      consume(1);

      match(T_ID, "`t' or `nil'", true);
    }
  else
    {
      syntax_error("unknown defdomain option", LA(0));
    }
}

void tdl_statement() {
  if(is_keyword(LA(0), "defdomain")) {
    consume(1);
    tdl_domainname();

    while(LA(0)->tag != T_DOT && LA(0)->tag != T_EOF) {
      tdl_defdomain_option();
    }

    match(T_DOT, "`.'", true);
  }
  else if(is_keyword(LA(0), "deldomain")) {
    consume(1);
    tdl_domainname();

    while(LA(0)->tag != T_DOT && LA(0)->tag != T_EOF) {
      tdl_defdomain_option();
    }

    match(T_DOT, "`.'", true);
  }
  else if(is_keyword(LA(0), "expand-all-instances")) {
    consume(1);

    match(T_DOT, "`.'", true);
  }
  else if(is_keyword(LA(0), "include")) {
    consume(1);

    if(LA(0)->tag != T_STRING) {
      syntax_error("expecting name of file to include", LA(0));
      recover(T_DOT);
    }
    else {
      string ofname = LA(0)->text;
      consume(1);

      match(T_DOT, "`.'", true);

      string fname = find_file(ofname, TDL_EXT);
      
      if(fname.empty()) {
        LOG(logSyntax, WARN, "file `" << ofname << "' not found. skipping...");
      } else {
        push_file(fname, "including");
      }
    }
  }
  else if(is_keyword(LA(0), "leval")) {
    consume(1);

    lisp_mode = 1;

    if(LA(0)->tag != T_LISP) {
      syntax_error("expecting LISP expression", LA(0));
      recover(T_DOT);
    }
    else
      consume(1);

    lisp_mode = 0;
    match(T_DOT, "`.'", true);
  }
  else if(is_keyword(LA(0), "end!")) {
    consume(1);
    match(T_DOT, "`.'", true);
  }
  else {
    syntax_error("unknown type of statement", LA(0));
    recover(T_DOT);
  }
}

void tdl_start(int toplevel)
{
  int start = tokensdelivered;

  do
    {
      if(LA(0)->tag == T_EOF)
        {
          if(toplevel)
            {
              consume(1);
              return;
            }
          syntax_error("end of input, but not on toplevel", LA(0));
          return;
        }
      else if(LA(0)->tag == T_KEYWORD)
        {
          if(is_keyword(LA(0), "begin"))
            tdl_block();
          else if(!is_keyword(LA(0), "end"))
            tdl_statement();
        }
      else if(LA(0)->tag == T_COLON)
        {
          if(LA(1)->tag == T_KEYWORD)
            {
              if(is_keyword(LA(1), "begin"))
                {
                  consume(1);
                  tdl_block();
                }
              else if(!is_keyword(LA(1), "end"))
                {
                  consume(1);
                  tdl_statement();
                }
              else
                break;
            }
          else
            {
              syntax_error("expecting keyword after `:'", LA(0));
              recover(T_DOT);
              break;
            }
        }
      else if(toplevel)
        {
          syntax_error("expecting block or statement", LA(0));
          recover(T_DOT);
        }
      else
        break;
    } while (1);

  if(tokensdelivered == start)
    {
      syntax_error("expecting nested block or statement",
                   LA(0));
      consume(1);
    }
}

/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 *   2004 Bernd Kiefer bk@dfki.de
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

/*

YY input format: (id, start, end, path, "stem" "surface", inflpos, inflrs, postags?)*

inflrs: - "null" = do internal morph analysis
        - stems and inflrules

*/

#include "yy-tokenizer.h"
#include "cheap.h"
#include <iostream>

using std::string;
using std::list;

tYYTokenizer::tYYTokenizer(position_map position_mapping, char classchar)
  : tTokenizer()
    , _position_mapping(position_mapping)
    , _class_name_char(classchar) { }

bool tYYTokenizer::eos()
{
  return _yypos >= _yyinput.length();
}

char tYYTokenizer::cur()
{
  if(eos())
    throw tError("yy_tokenizer: trying to read beyond end of string");
  return _yyinput[_yypos];
}

const char *tYYTokenizer::res()
{
  if(eos())
    throw tError("yy_tokenizer: trying to read beyond end of string");
  return _yyinput.c_str() + _yypos;
}

bool tYYTokenizer::adv(int n)
{
  while(n > 0)
    {
      if(eos())
        return false;
      _yypos++; n--;
    }
  return true;
}

// read whitespace
bool tYYTokenizer::read_ws()
{
  if(eos())
    return false;
  
  while(!eos() && isspace(cur())) adv();

  return true;
}

bool tYYTokenizer::read_special(char c)
{
  read_ws();

  if(eos())
    return false;

  if(cur() != c)
    return false;

  adv();

  return true;
}

bool tYYTokenizer::read_int(int &target)
{
  read_ws();

  if(eos())
    return false;
  
  const char *begin = res();
  char *end;

  target = strtol(begin, &end, 10);
  if(begin == end)
    return false;

  adv(end-begin);

  return true;
}

bool tYYTokenizer::read_double(double &target)
{
  read_ws();

  if(eos())
    return false;
  
  const char *begin = res();
  char *end;

  target = strtod(begin, &end);
  if(begin == end)
    return false;

  adv(end-begin);

  return true;
}

bool tYYTokenizer::read_string(string &target, bool quotedp, bool downcasep)
{
  target = "";

  read_ws();

  if(eos())
    return false;

  if(quotedp)
    {
      if(cur() != '"')
        return false;
      else
        adv();

      char last = (char)0;
      while(!eos() && (cur() != '"' || last == '\\'))
        {
          if(cur() != '\\' || last == '\\') {
            if(downcasep && (unsigned char)cur() < 127)
              target += tolower(cur());
            else
              target += cur();
          } // if
          last = (last == '\\' ? 0 : cur());
          adv();
        }

      if(cur() != '"')
        return false;
      else
        adv();
    }
  else
    {
      while(!eos() && (is_idchar(cur())))
        {
          target +=
            (downcasep && (unsigned char)cur() < 127 ? tolower(cur()) : cur());
          adv();
        }
      if(target.empty())
        return false;
    }

  return true;
}

bool tYYTokenizer::read_pos(string &tag, double &prob)
{
  tag = ""; prob = 0;

  read_ws();

  if(eos())
    return false;

  if(read_string(tag, true))
    {
      read_ws();

      if(eos())
        return false;

      if(read_double(prob))
        return true;
      else
        return false;
    }
  else
    return false;
}

int max(int a, int b)
{
  int max = a>b?a:b ;
  return max;
}


tInputItem *
tYYTokenizer::read_token()
{
  int id, start, end, path, inflpos;
  // we usually supply the inflection information, only perform lexicon lookup
  int token_class = STEM_TOKEN_CLASS;

  list<int> paths;
  list<int> infl_rules;
  string stem, surface;

  if(!read_special('('))
    throw tError("yy_tokenizer: ill-formed token (expected '(')");
  
  if(!read_int(id) || !read_special(','))
    throw tError("yy_tokenizer: ill-formed token (expected id)");
  
  if(!read_int(start) || !read_special(','))
    throw tError("yy_tokenizer: ill-formed token (expected start pos)");
  
  if(!read_int(end) || !read_special(','))
    throw tError("yy_tokenizer: ill-formed token (expected end pos)");
 
  while(read_int(path))
    paths.push_back(path);
  
  if(paths.empty() || !read_special(','))
    throw tError("yy_tokenizer: ill-formed token (expected paths)");
  
  bool downcasep = true;
  if(!read_string(stem, true, downcasep))
    throw tError("yy_tokenizer: ill-formed token (expected stem)");

  // translate iso-8859-1 german umlaut and sz
  translate_iso(stem);

  // skip empty tokens and punctuation
  if(stem.empty() || punctuationp(stem)) {
    if(verbosity > 4)
      fprintf(ferr, " - punctuation");

    token_class = SKIP_TOKEN_CLASS;
  } else {
    if (stem[0] == _class_name_char && stem.length() > 1) {
      // The stem is already a grammar type name
      token_class = lookup_type(stem.c_str());
      if (token_class == -1) {
        token_class = lookup_type(stem.c_str() + 1);
      }
      if (token_class == -1)
        throw tError("yy_tokenizer: unknown HPSG type given");
    }
  }

  // Read (optional) surface string.
  read_string(surface, true);
  if(surface.size() == 0) {
    surface = stem;
  } else {
    // translate iso-8859-1 german umlaut and sz
    translate_iso(surface);
  }

  // add surface form of ersatzes to fsmod, if necessary
  string ersatz_suffix("ersatz"); 
  modlist fsmods = modlist() ;
  char* ersatz_carg_path = cheap_settings->value("ersatz-carg-path");

  if ((ersatz_carg_path != NULL) and (stem.substr(max(0,stem.length()-ersatz_suffix.length())) == ersatz_suffix))
    {    
      string val = '"' + surface + '"'; // ensure val will become a string (surely there is a better way to do this???)
      fsmods.push_back(pair<string, type_t>(ersatz_carg_path, lookup_symbol(val.c_str())));
    }

  if(!read_special(','))
    throw tError("yy_tokenizer: ill-formed token (expected , after stem)");
  
  if(!read_int(inflpos) || !read_special(','))
    throw tError("yy_tokenizer: ill-formed token (expected inflpos)");
  
  string inflr;

  if(! read_string(inflr, true))
    throw tError("yy_tokenizer: ill-formed token (expected inflrs)");
  do {
    if(inflr == "zero")
      continue;
    
    if(inflr == "null") {
      if (token_class == STEM_TOKEN_CLASS) {
        // Unless signalled by a special "null" token, we expect stems and
        // inflectional rules. If we find a "null" token, the flag
        // `formlookup' will be set, and we do morphological analysis in house.
        token_class = WORD_TOKEN_CLASS;
        surface = stem;
        break;
      } else
        throw tError("yy_tokenizer: illegal \"null\" spec");
    }
    
    type_t infl_rule = lookup_type(inflr.c_str());
    if((infl_rule >= 0) 
       // XXX _fix_me_ the next test seems to be desirable
       // && (Grammar->find_rule(infl_rule)->trait() == INFL_TRAIT)
       ) {
      infl_rules.push_back(infl_rule);
    } else {
      if(verbosity > 4)
        fprintf(ferr, "Ignoring token containing unknown "
                "infl rule %s.\n", inflr.c_str());
      return NULL;
    }
  } while (read_string(inflr, true)) ; 
  
  postags poss;
  if(read_special(',')) {
    string tag; double prob;
    while(read_pos(tag, prob))
      poss.add(tag, prob);
  }
  
  if(!read_special(')'))
    throw tError("yy_tokenizer: ill-formed token (expected ')')");
  
  read_ws();

  char idstrbuf[6];
  sprintf(idstrbuf, "%d", id);
  tInputItem *res =
    new tInputItem(idstrbuf, start, end, surface, stem, paths, token_class);
  
  // Set the inflection rules and POS tags of the new item
  res->set_inflrs(infl_rules) ;
  res->set_in_postags(poss) ;
  res->set_mods(fsmods);
      
  return res;
}

void
tYYTokenizer::tokenize(myString s, inp_list &result)
{
  _yyinput = s;
  _yypos = 0;

  if(verbosity > 4)
    {
      std::cerr << "received YY tokens:" << std::endl << s 
                << std::endl << std::endl;
      fprintf(ferr, "[processing yy_tokenizer input]\n");
    };
  
  tInputItem *tok = 0;
  read_ws();
  while(! eos()) {
    if ((tok = read_token()) != NULL) {
      result.push_back(tok);
    }
    result.sort(tItem::precedes());
  }
}

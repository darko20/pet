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

/* tokenizer */

#include "pet-system.h"
#include "cheap.h"
#include "grammar.h"
#include "parse.h"
#include "inputchart.h"
#include "tokenizer.h"
#ifdef ICU
#include "unicode.h"
#endif

void tokenizer::print(FILE *f)
{
  fprintf(f, "tokenized<%s>\n", _input.c_str());
}

list<string> lingo_tokenizer::tokenize()
{
  string s(_input);

  // downcase string
  if(true || !cheap_settings->lookup("trivial-tokenizer"))
    for(string::size_type i = 0; i < s.length(); i++)
      s[i] = tolower(s[i]);
  
  // translate iso-8859-1 german umlaut and sz
  if(cheap_settings->lookup("translate-iso-chars"))
    translate_iso_chars(s);

  // replace all punctuation characters by blanks
#ifndef ICU
  for(string::size_type i = 0; i < s.length(); i++)
    if(Grammar->punctuationp(string(1, s[i])))
      s[i] = ' ';
#else
  UnicodeString U = Conv->convert(s);
  StringCharacterIterator it(U);
  UnicodeString Res;

  UChar32 c;
  while(it.hasNext())
    {
      c = it.next32PostInc();
      string coded = Conv->convert(UnicodeString(c));
      if(Grammar->punctuationp(coded))
	Res.append(UChar32(' '));
      else
	Res.append(c);
    }

  s = Conv->convert(Res);
#endif  

  // add some padding at the end
  s += " ";
  
  // split into words seperated by blanks
  list<string> words;
  string::size_type start = 0, end = 0;
  
  while(start < s.length())
    {
      while(start < s.length() && s[start] == ' ') start++;
      
      end = start;
      
      while(end < s.length() && s[end] != ' ') end++;
      if(end < s.length()) end--;
      
      if(end < s.length() && start <= end)
	words.push_back(s.substr(start, end - start + 1));
      
      start = end + 1;
    }

  //
  // if trivial tokenization is all we need, we are done here
  //
  if(cheap_settings->lookup("trivial-tokenizer")) return words;
  
  // handle apostrophs
  list<string> words2;
  string::size_type apo;
  
  for(list<string>::iterator pos = words.begin(); pos != words.end(); ++pos)
  {
      s = *pos;
      apo = s.find('\'');
      if(apo == STRING_NPOS)
      {
          // No apostrophe in word
          if(s.length() > 0)
              words2.push_back(s);
      }
      else
      {
          if(s.find('\'', apo+1) != STRING_NPOS)
              throw error("tokenizer: more than one apostroph in word");
	  
          if(apo == 0 || apo == s.length() - 1)
          {
              words2.push_back(s);
          }
          else
          {
              words2.push_back(s.substr(0, apo));
              words2.push_back(s.substr(apo, s.length() - apo));
          }
      }
  }

  return words2;
}

void lingo_tokenizer::add_tokens(input_chart *i_chart)
{
  list<string> tokens = tokenize();
  if(verbosity > 4)
    fprintf(ferr, "lingo_tokenizer:");

  int i = 0, id = 0;
  for(list<string>::iterator pos = tokens.begin();
      pos != tokens.end(); ++pos)
    {
      if(verbosity > 7)
	fprintf(ferr, " [%d] <%s>", i, pos->c_str());
      
      list<full_form> forms = Grammar->lookup_form(*pos);

      if(forms.empty())
	{
	  i_chart->add_token(id++, i, i+1, full_form(), *pos, 0, postags(), tPaths());
	}
      else
	{
	  for(list<full_form>::iterator currf = forms.begin();
	      currf != forms.end(); ++currf)
	    {
	      i_chart->add_token(id++, i, i+1, *currf, *pos,
				 0, postags(), tPaths());
	    }
	}
      i++;
    }

  if(verbosity > 4)
    fprintf(ferr, "\n");
}

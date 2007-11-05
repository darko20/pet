/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 *   2004 Bernd Kiefer kiefer@dfki.de
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

#include "pet-config.h"
#include "settings.h"
#include "lingo-tokenizer.h"
#include "utility.h"
#ifdef HAVE_ICU
#include "unicode.h"
#endif

using std::string;
using std::list;

extern settings *cheap_settings;


/** Produce a set of tokens from the given string. */
void 
tLingoTokenizer::tokenize(string s, inp_list &result) {
  list<string> tokens = do_it(s);
  if(verbosity > 4)
    fprintf(ferr, "[sending input through lingo_tokenizer]\n");

  char idstrbuf[6];
  int i = 0, id = 0;
  tInputItem *tok = 0;

  for(list<string>::iterator pos = tokens.begin();
      pos != tokens.end(); ++pos)
    {
      sprintf(idstrbuf, "%d", id++);
      tok=new tInputItem(idstrbuf, i, i+1, *pos, "", tPaths());
      
      result.push_back(tok);
      i++;
    }
  
}

list<string>
tLingoTokenizer::do_it(string s) {

  // translate iso-8859-1 german umlaut and sz
  translate_iso(s);

  // replace all punctuation characters by blanks
#ifndef HAVE_ICU
  for(string::size_type i = 0; i < s.length(); i++) {
    if(punctuation_char(s[i], _punctuation_characters))
      s[i] = ' ';
    else 
      //TODO: provide a flag that makes it possible to use the upcase chars
      if (true) 
        s[i] = tolower(s[i]);
  }
#else
  UnicodeString U = Conv->convert(s);
  StringCharacterIterator it(U);
  UnicodeString Res;

  UChar32 c;
  while(it.hasNext()) {
    c = it.next32PostInc();
    if(punctuation_char(c, _punctuation_characters))
      Res.append(UChar32(' '));
    else
      Res.append(c);
  }
  
  //TODO: provide a flag that makes it possible to use the upcase chars
  if(true)
    Res.toLower();

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
      if(apo == std::string::npos)
      {
          // No apostrophe in word
          if(s.length() > 0)
              words2.push_back(s);
      }
      else
      {
          if(s.find('\'', apo+1) != std::string::npos)
              throw tError("tokenizer: more than one apostroph in word");
          
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

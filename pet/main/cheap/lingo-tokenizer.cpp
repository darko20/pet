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

/* tokenizer base class and simple lingo tokenizer */

#include "config.h"
#include "pet-system.h"
#include "settings.h"
#include "lingo-tokenizer.h"
#include "utility.h"
#ifdef HAVE_ICU
#include "unicode.h"
#endif

extern settings *cheap_settings;

tTokenizer::tTokenizer() {
  char *s = cheap_settings->value("punctuation-characters");
  string pcs;
  if(s == 0)
    pcs = " \t?!.:;,()-+*$\n";
  else
    pcs = convert_escapes(string(s));
  
#ifndef HAVE_ICU
  _punctuation_characters = pcs;
#else
  _punctuation_characters = Conv->convert(pcs);
#endif

  _translate_iso_chars = cheap_settings->lookup("translate-iso-chars");
}

#ifdef HAVE_ICU
inline bool 
punctuation_char(const UChar32 c, const UnicodeString &punctuation_chars) {
  return (punctuation_chars.indexOf(c) != -1);
}
#else
inline bool
punctuation_char(const char c, const string &punctuation_chars) {
  return (punctuation_chars.find(c) != STRING_NPOS);
}
#endif

bool tTokenizer::punctuationp(const string &s)
{
#ifndef HAVE_ICU
  if(_punctuation_characters.empty())
    return false;
    
  for(string::size_type i = 0; i < s.length(); i++)
    if(! punctuation_char(s[i], _punctuation_characters))
      return false;
    
  return true;
#else
  UnicodeString U = Conv->convert(s);
  StringCharacterIterator it(U);
    
  UChar32 c;
  while(it.hasNext())
    {
      c = it.next32PostInc();
      if(! punctuation_char(c, _punctuation_characters))
        return false;
    }
    
  return true;
#endif
}


/** Replace all german Umlaut and sz characters in \a s by their isomorphix
 *  counterparts.
 */
void tTokenizer::translate_iso(string &s) {
  if (_translate_iso_chars) {
    for(string::size_type i = 0; i < s.length(); i++)
      {
        switch(s[i])
          {
          case 'ä':
            s.replace(i,1,"ae");
            break;
          case 'Ä':
            s.replace(i,1,"Ae");
            break;
          case 'ö':
            s.replace(i,1,"oe");
            break;
          case 'Ö':
            s.replace(i,1,"Oe");
            break;
          case 'ü':
            s.replace(i,1,"ue");
            break;
          case 'Ü':
            s.replace(i,1,"Ue");
            break;
          case 'ß':
            s.replace(i,1,"ss");
            break;
          }
      }
  }
}


/** Produce a set of tokens from the given string. */
void 
tLingoTokenizer::tokenize(string s, inp_list &result) {
  list<string> tokens = do_it(s);
  if(verbosity > 4)
    fprintf(ferr, "lingo_tokenizer:");

  char idstrbuf[6];
  int i = 0, id = 0;
  for(list<string>::iterator pos = tokens.begin();
      pos != tokens.end(); ++pos)
    {
      if(verbosity > 7)
        fprintf(ferr, " [%d] <%s>", i, pos->c_str());
      
      sprintf(idstrbuf, "%d", id++);
      result.push_back(new tInputItem(idstrbuf, i, i+1, *pos, "", tPaths()));
      i++;
    }
  
  if(verbosity > 4)
    fprintf(ferr, "\n");
  
}

list<string>
tLingoTokenizer::do_it(string s) {

  // downcase string
  if(true || !cheap_settings->lookup("trivial-tokenizer"))
    for(string::size_type i = 0; i < s.length(); i++)
      s[i] = tolower(s[i]);
  
  // translate iso-8859-1 german umlaut and sz
  translate_iso(s);

  // replace all punctuation characters by blanks
#ifndef HAVE_ICU
  for(string::size_type i = 0; i < s.length(); i++)
    if(punctuation_char(s[i], _punctuation_characters))
      s[i] = ' ';
#else
  UnicodeString U = Conv->convert(s);
  StringCharacterIterator it(U);
  UnicodeString Res;

  UChar32 c;
  while(it.hasNext())
    {
      c = it.next32PostInc();
      if(punctuation_char(c, _punctuation_characters))
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

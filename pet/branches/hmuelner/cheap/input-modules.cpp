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
#include "input-modules.h"
#include "settings.h"
#include "configs.h"

extern FILE *fstatus;

/** @name Tokenization options */
//@{
static bool init();
static bool statics_initialized = init();

/** choose tokenizer */
std::string tokenizer_names[] = { 
  "string", "yy", "yy_counts", "pic", "pic_counts", "fsr", "smaf", "INVALID"
} ;
//@}

tokenizer_id string2tokenizerid(const std::string &s) {
  tokenizer_id id = TOKENIZER_STRING;
  while (id < TOKENIZER_INVALID && s.compare(tokenizer_names[id]) != 0)
    id = (tokenizer_id) (id + 1);
  return id;
}

std::string tokenizerid2string(const tokenizer_id &val) {
  return tokenizer_names[val];
}


class TokenizerIDConverter : public AbstractConverter<tokenizer_id> {
  virtual ~TokenizerIDConverter() {}
  virtual std::string toString(const tokenizer_id& id) {
    return tokenizer_names[id];
  }
  virtual tokenizer_id fromString(const std::string& s) {
    tokenizer_id id = TOKENIZER_STRING;
    while (id < TOKENIZER_INVALID && s.compare(tokenizer_names[id]) != 0)
      id = (tokenizer_id) (id + 1);
    return id;
  }
};

/** Initialize all the options for the input modules.
    \todo maybe we should split that into init()s for the modules, but it's
    ok for now.
*/
static bool init() {
  managed_opt<tokenizer_id>("opt_tok", "", TOKENIZER_STRING,
                            //string2tokenizerid, tokenizerid2string
                            new TokenizerIDConverter()
                            );
  return true;
}


using std::string;
using std::list;

extern settings *cheap_settings;

tTokenizer::tTokenizer() {
  const char *s = cheap_settings->value("punctuation-characters");
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

bool tTokenizer::next_input(std::istream &in, string &result) {
  bool done;
  do {
    done = true;
    std::getline(in, result);
    if(_comment_passthrough && ((result[0] == '/' && result[1] == '/')
                                || result[0] == '#')) {
      if(_comment_passthrough > 0) fprintf(fstatus, "%s\n", result.c_str());
      done = false;
    } // if
  } while (! done);
  return ! result.empty();
}

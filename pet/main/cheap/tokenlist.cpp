/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* tokenized input */

#include <ctype.h>

#include <string>
using namespace std;
#include <list>
#include <iterator>

#include "cheap.h"
#include "tokenlist.h"

char *tokenlist::stop_characters = (char *)NULL;

void tokenlist::print(FILE *f)
{
  fprintf(f, "tokenized:");

  for(int i = 0; i < _ntokens; i++)
    {
      fprintf(f, " `%s'", _tokens[i].c_str());
    }

  fprintf(f, "\n");
}

tokenlist::tokenlist(const tokenlist &tl)
{
  throw error("unexpected call to copy constructor of tokenlist");
}

tokenlist::~tokenlist()
{
}

tokenlist &tokenlist::operator=(const tokenlist &tl)
{
  throw error("unexpected call to assignment operator of tokenlist");
}

bool tokenlist::punctuatiop(int i) {

  if(stop_characters == NULL || !stop_characters[0]) return false;
  
  if(i >= 0 && i < _ntokens) {
    for(const char *token = _tokens[i].c_str(); *token; token++)
      if(!isspace(*token) 
         && strchr(stop_characters, *token) == NULL) return false;
    return true;
  } /* if */
  else
    throw error("index out of bounds for tokenlist");

} /* tokenlist::punctuationp() */


string convert_escapes(char *s)
// convert standard C string mnemonic escape sequences
{
  string res = "";
  while(*s)
    {
      if(*s != '\\')
	res += string(1, *s);
      else
	{
	  if(*(s+1) == 0)
	    return res;
	  s++;
	  switch(*s)
	    {
	    case '\"':
	      res += "\"";
	      break;
	    case '\'':
	      res += "\'";
	      break;
	    case '?':
	      res += "\?";
	      break;
	    case '\\':
	      res += "\\";
	      break;
	    case 'a':
	      res += "\a";
	      break;
	    case 'b':
	      res += "\b";
	      break;
	    case 'f':
	      res += "\f";
	      break;
	    case 'n':
	      res += "\n";
	      break;
	    case 'r':
	      res += "\r";
	      break;
	    case 't':
	      res += "\t";
	      break;
	    case 'v':
	      res += "\v";
	      break;
	    default:
	      res += string(1,*s);
	      break;
	    }
	}
      s++;
    }
  return res;
}

void translate_iso_chars(string &s)
{
  for(string::size_type i = 0; i < s.length(); i++)
    {
      switch(s[i])
	{
	case 'ä':
	case 'Ä':
	  s.replace(i,1,"ae");
	  break;
	case 'ö':
	case 'Ö':
	  s.replace(i,1,"oe");
	  break;
	case 'ü':
	case 'Ü':
	  s.replace(i,1,"ue");
	  break;
	case 'ß':
	  s.replace(i,1,"ss");
	  break;
	}
    }
}

lingo_tokenlist::lingo_tokenlist(const char *in)
{
  string s(in);

  // 0) downcase string

  for(string::size_type i = 0; i < s.length(); i++)
    s[i] = tolower(s[i]);
  
  // translate iso-8859-1 german umlaut and sz

  if(cheap_settings->lookup("translate-iso-chars"))
    translate_iso_chars(s);

  // 1) replace all stop characters by blanks
  string stops = convert_escapes(stop_characters);

  s += " ";
  string::size_type pos = 0;
  while(pos != string::npos)
    {
      if((pos = s.find_first_of(stops, pos)) != string::npos)
	{
	  s[pos] = ' ';
	}
    }
  
  // 2) split into words seperated by blanks
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
  
  // 3) handle apostrophs
  
  list<string> words2;
  string::size_type apo;
  
  for(list<string>::iterator pos = words.begin(); pos != words.end(); ++pos)
    {
      s = *pos;
      apo = s.find('\'');
      if(apo == string::npos)
	{
	  if(s.length() > 0)
	    words2.push_back(s);
	}
      else
	{
	  if(s.find('\'', apo+1) != string::npos)
	    throw error("tokenizer: more than one apostroph in word");
	  
	  if(apo == s.length() - 1)
	    {
	      if(apo > 0)
		words2.push_back(s.substr(0, apo));
	      
	      words2.push_back("s");
	    }
	  else
	    {
	      if(apo == 0)
		{
		  // mimic lkb: use apostroph at beginning of word
		  words2.push_back(s.substr(apo, s.length()-apo));
		}
	      else
		{
		  words2.push_back(s.substr(0, apo));
		  words2.push_back(s.substr(apo+1, s.length() - (apo + 1)));
		}
	    }
	}
    }
  
  _ntokens = words2.size();
  _tokens.resize(_ntokens);
  
  int i = 0;
  for(list<string>::iterator pos = words2.begin();
      pos != words2.end(); ++pos)
    {
      _tokens[i] = *pos;
      i++;
    }
}

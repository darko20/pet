/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* general helper functions and classes */

#include "pet-system.h"
#include "utility.h"
#include "errors.h"

void *salloc(size_t size)
{
  void *p = malloc(size);

  if(!p)
    throw error("out of memory");
  
  return p;
}

#ifdef STRDUP
char *strdup(const char *s)
{
  char *s1;

  s1 = (char *) salloc(strlen(s) + 1);
  strcpy(s1, s);
  return s1;
}
#endif

void strtolower(char *s)
{
  if(s == NULL) return;
  while(*s)
  {
    *s = tolower(*s);
    s++;
  }
}

void strtoupper(char *s)
{
  if(s == NULL) return;
  while(*s)
  {
    *s = toupper(*s);
    s++;
  }
}

int strtoint(const char *s, const char *errloc, bool quotedp)
{
  char *endptr = 0;
  const char *foo = NULL;
  if(quotedp)
    {
      if(!*s == '"' || (foo = strrchr(s, '"')) == NULL)
	throw error(string("invalid quoted integer `") + string(s) +
		    string("' ") + string(errloc));
      s++;
    }
  int val = strtol(s, &endptr, 10);
  if(endptr == 0 || (quotedp ? endptr != foo :  *endptr != '\0'))
    throw error(string("invalid integer `") + string(s) + string("' ") 
                + string(errloc));

  return val;
}

string inttostr(int i)
{
  char buff[255];
  
  int l = snprintf(buff, 254, "%d", i);
  
  if(l <= 0)
    throw error(string("could not convert number to string"));

  return string(buff);
}

string convert_escapes(const string &s)
// convert standard C string mnemonic escape sequences
{
  string res = "";
  for(string::size_type i = 0; i < s.length(); i++)
    {
      if(s[i] != '\\')
	res += s[i];
      else
	{
	  i++;
	  if(i >= s.length())
	    return res;
	  switch(s[i])
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
	      res += s[i];
	      break;
	    }
	}
    }
  return res;
}

string escape_string(const string &s)
{
  string res;
  
  for(string::const_iterator it = s.begin(); it != s.end(); ++it)
  {
    if(*it == '"' || *it == '\\')
    {
      res += string("\\");
    }
    res += string(1, *it);
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

char *current_time(void)
{
  time_t foo = time(0);
  struct tm *now = localtime(&foo);
  static char *result = New char[80];

  if(result == 0)
    return "now";

  if(foo > 0 && now != 0)
    strftime(result, 80, "%d-%m-%Y (%H:%M:%S)", now);
  else
    sprintf(result, "now");

  return(result);
}

char *find_file(char *orig, char *ext, bool ext_req)
{
  char *newn;
  struct stat sb;

  if(orig == 0)
    return 0;

  newn = (char *) malloc(strlen(orig) + strlen(ext) + 1);
  
  strcpy(newn, orig);

  if(ext_req == false && access(newn, R_OK) == 0)
    {
      stat(newn, &sb);
      if((sb.st_mode & S_IFDIR) == 0)
        return newn;
    }

  strcat(newn, ext);
      
  if(access(newn, R_OK) == 0)
    {
      stat(newn, &sb);
      if((sb.st_mode & S_IFDIR) == 0)
        return newn;
    }

  return NULL;
}

char *output_name(char *in, char *oldext, const char *newext)
{
  char *out, *ext;

  out = (char *) malloc(strlen(in) + strlen(newext) + 1);

  strcpy(out, in);

  ext = strrchr(out, '.');

  if(ext && strcasecmp(ext, oldext) == 0)
    {
      strcpy(ext, newext);
    }
  else
    {
      strcat(out, newext);
    }

  return out;
}

string read_line(FILE *f)
{
  const int ASBS = 131072; // arbitrary, small buffer size
  static char buff[ASBS];

  if(fgets(buff, ASBS, f) == NULL)
    return string();
  
  if(buff[0] == '\0' || buff[0] == '\n')
    return string();

  buff[strlen(buff) - 1] = '\0';

  return string(buff);
}

#ifdef __BORLANDC__

#include <alloc.h>

void print_borland_heap(FILE *f)
{
  struct heapinfo hi;
  if(heapcheck() == _HEAPCORRUPT)
    {
      fprintf(f, "Heap is corrupted.\n");
      return;
    }

   hi.ptr = 0;
   fprintf(f, "   Block  Size   Status\n");
   fprintf(f, "   -----  ----   ------\n");
   while(heapwalk( &hi ) == _HEAPOK)
   {
     fprintf(f, "%7u    %s\n", hi.size, hi.in_use ? "used" : "free");
   }
}

#endif

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

/* general helper functions and classes */

#include "pet-config.h"
#include "utility.h"
#include "errors.h"

#include <cstdlib>

//#include <iostream>  // only for debugging

#include <sys/stat.h>

using std::string;
using std::list;

/** "Safe" \c malloc: call \c malloc and throw an error if
 *   it returns \c NULL.
 */
void *salloc(size_t size)
{
  void *p = malloc(size);

  if(!p)
    throw tError("out of memory");
  
  return p;
}

#ifndef HAVE_STRDUP
char *strdup(const char *s)
{
  char *s1;

  s1 = (char *) salloc(strlen(s) + 1);
  strcpy(s1, s);
  return s1;
}
#endif

/** Convert all characters in \a s to lower case */
void strtolower(char *s)
{
  if(s == NULL) return;
  while(*s)
  {
    *s = tolower(*s);
    s++;
  }
}

/** Convert all characters in \a s to upper case */
void strtoupper(char *s)
{
  if(s == NULL) return;
  while(*s)
  {
    *s = toupper(*s);
    s++;
  }
}

/** Convert all characters in \a src to lower case and copy to \a dest */
void strtolower(char *dest, const char *src) {
  if (src == NULL || dest == NULL) return;
  while (*src) {
    *dest = tolower(*src);
    src++, dest++;
  }
  *dest = *src;
}

/** Convert all characters in \a src to upper case and copy to \a dest */
void strtoupper(char *dest, const char *src) {
  if (src == NULL || dest == NULL) return;
  while (*src) {
    *dest = toupper(*src);
    src++, dest++;
  }
  *dest = *src;
}

/** Convert a (possibly) quoted integer string \a s into an integer and issue
 *  an error if this does not succeed.
 *  \param s The input string
 *  \param errloc A description of the calling environment
 *  \param quotedp If \c true, the integer has to be enclosed in double quotes.
 *  \return the converted integer
 */
int strtoint(const char *s, const char *errloc, bool quotedp)
{
  char *endptr = 0;
  const char *foo = NULL;
  if(quotedp)
    {
      if(!*s == '"' || (foo = strrchr(s, '"')) == NULL)
        throw tError(string("invalid quoted integer `") + string(s) +
                     string("' ") + string(errloc));
      s++;
    }
  int val = strtol(s, &endptr, 10);
  if(endptr == 0 || (quotedp ? endptr != foo :  *endptr != '\0'))
    throw tError(string("invalid integer `") + string(s) + string("' ") 
                 + string(errloc));

  return val;
}

/** Convert standard C string mnemonic escape sequences */
string convert_escapes(const string &s)
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


/** Escape all double quote and backslash characters in \a s with a preceding
 *  backslash.
 */
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


/** Return the current date and time in a static char array */
const char *current_time(void)
{
  time_t foo = time(0);
  struct tm *now = localtime(&foo);
  static char *result = new char[80];

  if(result == 0)
    return "now";

  if(foo > 0 && now != 0)
    strftime(result, 80, "%d-%m-%Y (%H:%M:%S)", now);
  else
    sprintf(result, "now");

  return(result);
}


/** Return \c true if \a filename exists and is not a directory */
bool file_exists_p(const char *filename) {
  struct stat sb;
  return ((access(filename, R_OK) == 0) && (stat(filename, &sb) != -1)
          && ((sb.st_mode & S_IFDIR) == 0));
}

/** \brief Check if \a name , with or without extension \a ext, is the name of
 *  a readable file. If \base is given in addition, take the directory part of
 *  \a base as the directory component of the pathname
 *
 * \param name  the basename of the file, possibly already with extension
 * \param ext   the extension of the file
 * \param base  if given, the directory component of the pathname.
 *
 * \return the full pathname of the file, if it exists with or without
 *         extension, an empty string otherwise.
 */
string find_file(const char *name, const char *ext, const char *base) {
  if (name == NULL) return string();

  string newname = (base != NULL) ? (dir_name(base) + name) : string(name);

  if (file_exists_p(newname.c_str())) return newname;

  newname += ext;
  return file_exists_p(newname.c_str()) ? newname : string();
}


/** Extract the directory component of a pathname and return it.
 *  \return an empty string, if \a pathname did not contain a path separator
 *          character, the appropriate substring otherwise
 *          (with the path separator at the end)
 */
string dir_name(const char *pathname) {
  char *slash = strrchr(pathname, PATH_SEP[0]);
  // _prefix gets the dirname of the path encoded in base
  return slash == NULL ? string() : string(pathname, slash - pathname + 1);
}


/** Extract only the filename part from a pathname, i.e., without directory and
 *  extension components.
 */
string raw_name(const char *pathname) {
  // return part between last slash and first dot after that
  const char *slash = strrchr((char *) pathname, PATH_SEP[0]);
  if(slash == 0) slash = (char *) pathname; else slash++;
  const char *dot = strchr(slash, '.');
  if(dot == 0) dot = slash + strlen(slash);;
  
  return string(slash, dot - slash);
}


string output_name(const string &in, const char *oldext, const char *newext) {
  string out = in;

  string::size_type ext = out.rfind('.');

  if(ext && strcasecmp(out.c_str() + ext, oldext) == 0)
    // erase the old extension
    out.erase(ext);

  // add the new extension
  out += newext;
  
  return out;
}


string read_line(FILE *f, int commentp)
{
  const int ASBS = 131072; // arbitrary, small buffer size
  static char buff[ASBS];

  if(fgets(buff, ASBS, f) == NULL)
    return string();
  
  if(buff[0] == '\0' || buff[0] == '\n')
    return string();

  //
  // 2004/03/12 Eric Nichols <eric-n@is.naist.jp>
  // allow pass-through of comment lines in input: ignore any line starting
  // with either `#' or `//'; optionally repeat it back on the output stream.
  //
  if(commentp && (buff[0] == '#' || buff[0] == '/' && buff[1] == '/')) {
    if(commentp > 0) fprintf(stderr, "%s\n", buff);
    return read_line(f, commentp);
  } // if

  buff[strlen(buff) - 1] = '\0';

  return string(buff);
}

void
findAndReplace(string &s, const string &oldText, const string &newText)
{
    if(s.empty() || oldText.empty())
        return;
    
    string::size_type start = 0, len;
    len = s.length();

    while(len > 0 && len >= oldText.length())
    {
        string::size_type pos = s.find(oldText, start);
        if(pos == string::npos)
            break;
        s.replace(pos, oldText.length(), newText);
        len -= pos + oldText.length() - start;
        start = pos + newText.length();
    }
}

void
splitStrings(list<string> &strs)
{
    list<string> result;
    
    for(list<string>::iterator it = strs.begin(); it != strs.end(); ++it)
    {
        string::size_type p;
        while((p = it->find(' ')) != string::npos)
        {
            string s = it->substr(0, p);
            it->erase(0, p + 1);
            result.push_back(s);
        }
        result.push_back(*it);
    }
    
    strs.swap(result);
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

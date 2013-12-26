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
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif
#ifndef FLOP
#ifdef HAVE_BOOST_REGEX_ICU_HPP
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#endif
#endif
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>

using boost::algorithm::iequals;
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
      if(!(*s == '"') || (foo = strrchr(s, '"')) == NULL)
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
  for(string::size_type i = 0; i < s.length(); ++i)
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
string current_time(void)
{
  time_t foo = time(0);
  struct tm *now = localtime(&foo);
  static char *result = new char[80];

  if(result == 0)
    return "now";

  if(foo > 0 && now != 0)
    strftime(result, 80, "%d-%m-%Y (%H:%M:%S)", now);
  else
    return "now";

  return result;
}


/** Return \c true if \a filename exists and is not a directory */
bool file_exists_p(const string &fn) {
#ifdef HAVE_BOOST_FILESYSTEM
  return fs::exists(fn) && fs::is_regular_file(fn) && !fs::is_directory(fn);
#else
  const char *filename = fn.c_str();
  struct stat sb;
  return ((access(filename, R_OK) == 0) && (stat(filename, &sb) != -1)
          && ((sb.st_mode & S_IFDIR) == 0));
#endif
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
string
find_file(const string &name, const string &ext, const string &base) {

  string newname = dir_name(base) + name;

  if (file_exists_p(newname)) return newname;

  newname += ext;
  return file_exists_p(newname) ? newname : string();
}


/** look for the file with \a name (dot) \a ext first in \a base_dir, then in
 *  \a base_dir + SET_DIRECTORY.
 *  \return the name of the file, if it exists, an empty string otherwise.
 */
string
find_set_file(const string &name, const string &ext, const string &base){
  string fname;
  string base_dir = dir_name(base);

  // fname contains the full pathname to the settings file, except for the
  // extension, first in the directory the base path points to
  fname = base_dir + name + ext;

  if(! file_exists_p(fname.c_str())) {
    // We could not find the settings file there, so try it in the
    // subdirectory predefined for settings
    fname = base_dir + SET_SUBDIRECTORY + PATH_SEP + name + ext;
  }
  //std::cerr << name << " " << ext << " " << base_dir << ">" << fname
  //          << std::endl;
  return ((file_exists_p(fname.c_str())) ? fname : "");
}

/** Extract the directory component of a pathname and return it.
 *  \return an empty string, if \a pathname did not contain a path separator
 *          character, the appropriate substring otherwise
 *          (with the path separator at the end)
 */
string dir_name(const string &pathname) {
#ifdef HAVE_BOOST_FILESYSTEM
  fs::path full_path = fs::system_complete(fs::path(pathname));
  string result = full_path.parent_path().string();
  if (! result.empty()) result += "/";
#else
  string::size_type lastslash = pathname.rfind(PATH_SEP[0]);
  // _prefix gets the dirname of the path encoded in base
  string result =
    (string::npos == lastslash) ? string() : pathname.substr(0, lastslash + 1);
#endif
  return result;
}


/** Extract only the filename part from a pathname, i.e., without directory and
 *  extension components.
 */
string raw_name(const string &pathname) {
  // return part between last slash and first dot after that
#ifdef HAVE_BOOST_FILESYSTEM
  fs::path p(pathname);
  return p.stem();
#else
  string::size_type lastslash = pathname.rfind(PATH_SEP[0]);
  if(string::npos == lastslash) lastslash = 0; else lastslash++;
  string::size_type dot = pathname.find('.', lastslash);

  string result = pathname.substr(lastslash, dot - lastslash);
  return result;
#endif
}


string output_name(const string& in, const string& oldext, const string& newext)
{
  string out = in;

  string::size_type ext = out.rfind('.');

  if(ext != string::npos && iequals(out.c_str() + ext, oldext))
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
  if(commentp && ((buff[0] == '#') || ((buff[0] == '/') && (buff[1] == '/')))) {
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

/**
 * Escape XML delimiters.
 */
std::string
xml_escape(std::string s) {
  boost::replace_all(s, "<", "&lt;");
  boost::replace_all(s, ">", "&gt;");
  boost::replace_all(s, "&", "&amp;");
  boost::replace_all(s, "'", "&apos;");
  boost::replace_all(s, "\"", "&quot;");
  return s;
}

/**
 * relatively low-level input output routines to work on sockets (or pipes).
 */
static list<FILE *> _socket_loggers;

void
install_socket_logger(FILE *logger) {

  _socket_loggers.push_front(logger);

} // install_socket_logger()

int
socket_write(int socket, char *string) {

  int written, left, n;

  for(written = 0, left = n = strlen(string);
      left > 0;
      left -= written, string += written) {
    if((written = write(socket, string, left)) == -1) {
      for(list<FILE *>::iterator log = _socket_loggers.begin();
          log != _socket_loggers.end();
          ++log) {
        fprintf(*log,
                "[%d] socket_write(): write() error [%d].\n",
                getpid(), errno);
        fflush(*log);
      } /* for */
      return -1;
    } /* if */
  } /* for */

  return n - left;

} /* socket_write() */

int
socket_readline(int socket, char *string, int length, bool ffp) {

  int i, n;
  char c;

  for(i = n = 0; n < length && (i = read(socket, &c, 1)) == 1;) {
    if(c == EOF) {
      return -1;
    } /* if */
    if(c == '\r') {
      (void)read(socket, &c, 1);
    } /* if */
    if(c == '\n') {
      if(!ffp) {
        string[n] = (char)0;
        return n + 1;
      } /* if */
    } /* if */
    else if(c == '\f') {
      if(ffp) {
        string[n] = (char)0;
        return n + 1;
      } /* if */
      else {
       string[n++] = c;
      } /* else */
    } /* if */
    else {
      string[n++] = c;
    } /* else */
  } /* for */
  if(i == -1) {
    return -1;
  } /* if */
  else if(!n && !i) {
    return 0;
  } /* if */
  if(n < length) {
    string[n] = 0;
  } /* if */

  return n + 1;

} /* socket_readline() */

#ifndef FLOP
#ifdef HAVE_BOOST_REGEX_ICU_HPP
/* escape literal braces in regex before giving it to Boost, since Boost::Regex
 * is not completely pcre compatible
 */
string boostescape(string esc)
{
  boost::smatch res;
  boost::u32regex quantifier
    = boost::make_u32regex("^([{](?:[0-9]+(?:,[0-9]*)?)[}])");
  int len = esc.length();
  for (int x=0; x < len; ++x) {
    if (esc.at(x) == '{') {
      if (x > 0 && esc.at(x-1) == '\\')
        continue; //already escaped, keep going
      if (x > 1 && esc.at(x-2) == '\\' && isalpha(esc.at(x-1))) {
        //probably the start of a backslash seq. find and skip past the
        //closing brace. If not a backslash seq, ambiguous - let boost complain
        while (x < len && esc.at(x) != '}')
          x++;
      } else if (boost::u32regex_search(esc.substr(x), res, quantifier)) {
        //quantifier, skip past closing brace
        x += string(res[0]).length()-1;
      } else {
        esc.replace(x,1, "\\{", 2);
        x++;
        len++;
      }
    } else if (esc.at(x) == '}') {
      esc.replace(x,1, "\\}", 2);
      x++;
      len++;
    }
  }
  return esc;
}
#endif
#endif

/* Mode: -*- C++ -*-
 * PET
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

/** \file utility.h
 * Helper functions.
 */

#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <cstring>
#include <list> 
#include <string>
#include <cstdio>

#define SET_SUBDIRECTORY "pet"

// define the path separator char with the configure utility
#ifdef WINDOWS
#define strcasecmp stricmp
#endif

/** allocates \a size bytes of memory - throw error when out of memory */
void *salloc(size_t size);

#ifndef HAVE_STRDUP
/** duplicate string \a s into newly allocated memory */
char *strdup(const char *s);
#endif

/** @name Case Conversion
 * Destructively convert string to all lower/upper case.
 */
/*@{*/
void strtolower(char *s);
void strtoupper(char *s);
/*@}*/

/** @name Nondestructive Case Conversion
 * Nondesctructively convert and copy string to all lower/upper case.
 */
/*@{*/
void strtolower(char *dest, const char *src);
void strtoupper(char *dest, const char *src);
/*@}*/

/** Convert a (possibly) quoted integer string \a s into an integer and issue
 *  an error if this does not succeed.
 *  \param s The input string
 *  \param errloc A description of the calling environment
 *  \param quotedp If \c true, the integer has to be enclosed in double quotes.
 *  \return the converted integer
 */
extern int strtoint(const char *s, const char *errloc, bool = false);

/** convert standard C string mnemonic escape sequences in \a s */
std::string convert_escapes(const std::string &s);

/** escape all '"' and '\' in string \a s using '\' */
std::string escape_string(const std::string &s);

/** return current date and time in static string; client must not free() */
const char *current_time(void);

/** Return \c true if \a filename exists and is not a directory */
bool file_exists_p(const std::string &filename);

/** Extract the directory component of a pathname and return it.
 *  \return an empty string, if \a pathname did not contain a path separator
 *          character, the appropriate substring otherwise
 *          (with the path separator at the end)
 */
std::string dir_name(const std::string &pathname);

/** Extract only the filename part from a pathname, i.e., without directory and
 *  extension components.
 */
std::string raw_name(const std::string &pathname);

/** \brief Check if \a name , with or without extension \a ext, is the name of
 *  a readable file. If \base is given in addition, take the directory part of
 *  \a base as the directory component of the pathname.
 *
 * \param name  the basename of the file, possibly already with extension
 * \param ext   the extension of the file
 * \param base  if given, the directory component of the pathname.
 *
 * \return the full pathname of the file, if it exists with or without
 *         extension, an empty string otherwise.
 */
std::string 
find_file(const std::string &name, const std::string &extension,
          const std::string &base = std::string());

/** \brief look for the file with \a name (dot) \a ext first in \a base_dir,
 *  then in \a base_dir + SET_DIRECTORY. \a base_dir must be a directory
 *  specification only.
 *  
 *  \return the name of the file, if it exists, an empty string otherwise.
 */
std::string
find_set_file(const std::string &name, const std::string &ext,
              const std::string &base_dir);

/** Produce an output file name from an input file name \a in by replacing the 
 *  \a oldextension (if existent) by \a newextension or appending the 
 *  \a newextension otherwise.
 *  \return the new string
 */
std::string
output_name(const std::string &name, const char *oldextension,
            const char *newextension);

/** \brief Read one line from specified file. Returns empty string when no line
 *  can be read.
 */
std::string read_line(FILE *f, int commentp = 0);

/** Replace all occurences of \a oldText in \a s by \a newText. */
void
findAndReplace(std::string &s, const std::string &oldText, const std::string &newText);

/** Split each string in a list of strings into tokens seperated by blanks. */
void
splitStrings(std::list<std::string> &strs);

/** Predicate comparing two plain C strings for equality */
struct cstr_eq {
  bool operator()(const char* s, const char* t) const {
    return strcmp(s, t) == 0;
  }
};

/** Less than predicate for two plain C strings */
struct cstr_lt {
  bool operator()(const char *s, const char *t) const {
    return strcmp(s, t) < 0;
  }
};

/** Case insensitive less than predicate for plain C strings */
struct cstr_lt_case {
  bool operator()(const char *s, const char *t) const {
    return strcasecmp(s, t) < 0;
  }
};

/** A function object comparing two strings for equality */
struct string_eq : public std::binary_function<std::string, std::string, bool> {
  inline bool operator()(const std::string &s1, const std::string &s2) {
    return s1 == s2;
  }
};

/** A function object comparing two strings lexicographically */
struct string_lt {
  bool operator()(const std::string &s, const std::string &t) const {
    return strcmp(s.c_str(), t.c_str()) < 0;
  }
};

/** A function object comparing two strings lexicographically disregarding
 * case.
 */
struct string_lt_case {
  bool operator()(const std::string &s, const std::string &t) const {
    return strcasecmp(s.c_str(), t.c_str()) < 0;
  }
};

#ifdef __BORLANDC__
void print_borland_heap(FILE *f);
#endif

#endif

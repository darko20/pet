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

/** \file utility.h
 * Helper functions.
 */

#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <list> 
#include <string>

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

/** Convert a (possibly) quoted integer string \a s into an integer and issue
 *  an error if this does not succeed.
 *  \param s The input string
 *  \param errloc A description of the calling environment
 *  \param quotedp If \c true, the integer has to be enclosed in double quotes.
 *  \return the converted integer
 */
extern int strtoint(const char *s, const char *errloc, bool = false);

/** convert standard C string mnemonic escape sequences in \a s */
string convert_escapes(const string &s);

/** escape all '"' and '\' in string \a s using '\' */
string escape_string(const string &s);

/** Replace all german Umlaut and sz characters in \a s by their isomorphix
 *  (ae, ue, ss, ...) counterparts.
 */
void translate_iso_chars(string &s);

/** return current date and time in static string; client must not free() */
char *current_time(void);

/** Check if \a orig , possibly concatenated with \a ext, is the name of a
 *  readable file and return a newly allocated char string with the filename.
 * \param orig    The basename of the file, possibly already with extension
 * \param ext     The extension of the file
 * \param ext_req If \c true, only the concatenated name \a orig + \ext is
 *                checked, otherwise, \a orig is checked alone first and 
 *                used as name if a file was found.
 */
char *find_file(char *orig, char *extension, bool ext_req = false);
/** Produce an output file name from an input file name \a in by replacing the 
 *  \a oldextension (if existent) by \a newextension or appending the 
 *  \a newextension otherwise.
 *  The string returned is allocated with \c malloc.
 */
char *output_name(char *in, char *oldextension, const char *newextension);

/** \brief Read one line from specified file. Returns empty string when no line
 *  can be read.
 */
string read_line(FILE *f);

/** Replace all occurences of \a oldText in \a s by \a newText. */
void
findAndReplace(string &s, const string &oldText, const string &newText);

/** Split each string in a list of strings into tokens seperated by blanks. */
void
splitStrings(list<string> &strs);

/** Predicate comparing two plain C strings for equality */
struct cstr_eq
{
  bool operator()(const char* s, const char* t) const
  {
    return strcmp(s, t) == 0;
  }
};

/** Less than predicate for two plain C strings */
struct cstr_lt
{
  bool operator()(const char *s, const char *t) const
  {
    return strcmp(s, t) < 0;
  }
};

/** Case insensitive less than predicate for plain C strings */
struct cstr_lt_case
{
  bool operator()(const char *s, const char *t) const
  {
    return strcasecmp(s, t) < 0;
  }
};

/** A function object comparing two strings lexicographically */
struct string_lt
{
  bool operator()(const string &s, const string &t) const
  {
    return strcmp(s.c_str(), t.c_str()) < 0;
  }
};

/** A function object comparing two strings lexicographically disregarding
 * case.
 */
struct string_lt_case
{
  bool operator()(const string &s, const string &t) const
  {
    return strcasecmp(s.c_str(), t.c_str()) < 0;
  }
};

#ifdef __BORLANDC__
void print_borland_heap(FILE *f);
#endif

#endif

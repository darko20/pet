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

/* helpers */

#ifndef _UTILITY_H_
#define _UTILITY_H_

// allocates .size. bytes of memory - throw error when out of memory
void *salloc(size_t size);

#ifdef STRDUP
// duplicate string
char *strdup(const char *s);
#endif

// destructively convert string to all lower/upper case
void strtolower(char *s);
void strtoupper(char *s);

// convert string to integer, throw error msg if invalid
extern int strtoint(const char *s, const char *errloc, bool = false);

// convert standard C string mnemonic escape sequences
string convert_escapes(const string &s);

// escape all '"' and '\' in a string using '\'
string escape_string(const string &s);

void translate_iso_chars(string &s);

// return current date and time in static string; client must not free()
char *current_time(void);

char *find_file(char *orig, char *extension, bool ext_req = false);
char *output_name(char *in, char *oldextension, const char *newextension);

// Read one line from specified file. Returns empty string when no line
// can be read.
string read_line(FILE *f);

// Replace all occurences of oldText in s by newText.
void
findAndReplace(string &s, const string &oldText, const string &newText);

// Split each string in a list of strings into tokens seperated by blanks.
void
splitStrings(list<string> &strs);

struct cstr_eq
{
  bool operator()(const char* s, const char* t) const
  {
    return strcmp(s, t) == 0;
  }
};

struct cstr_lt
{
  bool operator()(const char *s, const char *t) const
  {
    return strcmp(s, t) < 0;
  }
};

struct cstr_lt_case
{
  bool operator()(const char *s, const char *t) const
  {
    return strcasecmp(s, t) < 0;
  }
};

struct string_lt
{
  bool operator()(const string &s, const string &t) const
  {
    return strcmp(s.c_str(), t.c_str()) < 0;
  }
};

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

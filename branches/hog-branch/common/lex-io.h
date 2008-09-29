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

/** \file lex-io.h
 * Fast memory mapped I/O for lexer - implemented with high lexer throughput
 * in mind.
 */

#ifndef _LEX_IO_H_
#define _LEX_IO_H_

#include <cstdio>
#include <string>

/** maximal nesting depth of include files */
#define MAX_LEX_NEST 16 

/** Total number of lines processed by lexer */
extern int total_lexed_lines;

/** Custom lexer file structure */
typedef struct 
{
  char *buff;
  int fd;
  size_t len;
  size_t pos;
  char *fname;
  int linenr, colnr;
  char *info;
} lex_file;

/** Object to store a file location with line and column number */
struct lex_location 
{
  /** The file name */
  const char *fname;
  /** line nr */
  int linenr;
  /** column nr */
  int colnr;
};

/** Build a new location object with the given parameters. */
struct lex_location *new_location(const char *fname, int linenr, int colnr);

/*@{*/
/** File streams for error and status messages */
extern FILE *ferr, *fstatus;
/*@}*/

/** Push file \a fname onto include stack, where \a info provides a hint in
 *  which context the function is used.
 */
void push_file(const std::string &fname, const char *info);
/** Pop file from include stack
 *  \return nonzero if there are still open files, zero otherwise.
 */
int pop_file();

/*@{*/
/** Return the current location parameters of the lexer */
int curr_line();
int curr_col();
char *curr_fname();
/*@}*/

/** The file stream the lexer is currently working on */
extern lex_file *CURR;

/** lexical lookahead of \a n characters. 
 *  \return \c EOF if past end of file, the character \a n positions away from
 *  the current file pointer otherwise.
 */
inline int LLA(int n)
{ 
  if(CURR == NULL || CURR->pos + n >= CURR->len)
    return EOF;

  return CURR->buff[CURR->pos + n];
}

/** Consume \a n characters */
int LConsume(int n);
/** Get the current pointer into the file buffer as a starting point for
 *  longer tokens such as comments or strings.
 */
char *LMark();

#endif

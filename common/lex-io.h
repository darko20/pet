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

/** Total number of lines processed by lexer */
extern int total_lexed_lines;

/** Custom lexer file structure */
struct Lex_file
{
  char *buff;
  int fd;
  size_t len;
  size_t pos;
  std::string fname;
  int linenr;
  int colnr;
  std::string info;
  Lex_file() : buff(NULL), fd(-1), len(0), pos(0), fname(), linenr(1), colnr(1), info() {}
  ~Lex_file();
};

/** Object to store a file location with line and column number */
struct Lex_location 
{
    /** The file name */
    std::string fname;
    /** line nr */
    int linenr;
    /** column nr */
    int colnr;
    Lex_location() : fname(), linenr(1), colnr(1) {}
    Lex_location(const std::string& infname, int inlinenr, int incolnr)
        : fname(infname), linenr(inlinenr), colnr(incolnr) {}
    void assign(const std::string& infname, int inlinenr, int incolnr)
    {
        fname = infname;
        linenr = inlinenr;
        colnr = incolnr;
    }
};

/** Push file \a fname onto include stack, where \a info provides a hint in
 *  which context the function is used.
 */
void push_file(const std::string &fname, const char *info);

/** Push contents of string \a input onto include stack, where \a info provides
 * a hint in which context the function is used.
 */
void push_string(const std::string &input, const char *info);

/** Pop file from include stack
 *  \return nonzero if there are still open files, zero otherwise.
 */
int pop_file();

/*@{*/
/** Return the current location parameters of the lexer */
int curr_line();
int curr_col();
std::string curr_fname();
/*@}*/

/** lexical lookahead of \a n characters. 
 *  \return \c EOF if past end of file, the character \a n positions away from
 *  the current file pointer otherwise.
 */
extern int LLA(int n);

/** Consume \a n characters */
int LConsume(int n);
/** Get the current pointer into the file buffer as a starting point for
 *  longer tokens such as comments or strings.
 */
char *LMark();

#endif

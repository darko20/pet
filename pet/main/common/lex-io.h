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

/* fast memory mapped I/O for lexer - implemented with high lexer throughput in mind */

#ifndef _LEX_IO_H_
#define _LEX_IO_H_

#define MAX_LEX_NEST 16 
/* maximal nesting depth of include files */

extern int total_lexed_lines;

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

struct lex_location 
{
  char *fname;
  int linenr, colnr;
};

struct lex_location *new_location(char *fname, int linenr, int colnr);

extern FILE *ferr, *fstatus;

void push_file(const char *fname, char *info);
int pop_file();

int curr_line();
int curr_col();
char *curr_fname();

/* lexical lookahead */

extern lex_file *CURR;
inline int LLA(int n)
{ 
  if(CURR == NULL || CURR->pos + n >= CURR->len)
    return EOF;

  return CURR->buff[CURR->pos + n];
}

int LConsume(int n);
char *LMark();

#endif

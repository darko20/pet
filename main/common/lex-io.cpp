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

/* fast memory mapped I/O for lexer - implemented with high lexer
   throughput in mind */

/* functionality provided:
   - stack of input files to handle include files transparently
   - efficient arbitrary lookahead, efficient buffer access via mark()
*/

/* for unix - essentially we just mmap(2) the whole file,
   which gives both good performance (minimizes copying) and easy use
   for windows - read the whole file
*/

#include "pet-system.h"
#include "lex-io.h"
#include "errors.h"
#include "options.h"

static lex_file file_stack[MAX_LEX_NEST];
static int file_nest = 0;
lex_file *CURR;

int total_lexed_lines = 0;

struct lex_location *new_location(char *fname, int linenr, int colnr)
{
  struct lex_location *loc = (struct lex_location *) malloc(sizeof(struct lex_location));

  loc->fname = fname;
  loc->linenr = linenr;
  loc->colnr = colnr;

  return loc;
}

void push_file(const char *fname, char *info)
{
  lex_file f;
  struct stat statbuf;

  if(file_nest >= MAX_LEX_NEST)
    throw error(string("too many nested includes (in ") + string(fname) + string(") - giving up"));

#ifndef WINDOWS
  f.fd = open(fname, O_RDONLY);
#else
  f.fd = open(fname, O_RDONLY | O_BINARY);
#endif

  if(f.fd < 0)
    throw error("error opening `" + string(fname) + "': " + string(strerror(errno)));

  if(fstat(f.fd, &statbuf) < 0)
    throw error("couldn't fstat `" + string(fname) + "': " + string(strerror(errno)));

  f.len = statbuf.st_size;

#ifdef USEMMAP
  f.buff = (char *) mmap(0, f.len, PROT_READ, MAP_SHARED, f.fd, 0);

  if(f.buff == (caddr_t) -1)
    throw error("couldn't mmap `" + string(fname) + "': " + string(strerror(errno)));

#else
  f.buff = (char *) malloc(f.len + 1);
  if(f.buff == 0)
    throw error("couldn't malloc for `" + string(fname) + "': " + string(strerror(errno)));
  
  if((size_t) read(f.fd,f.buff,f.len) != f.len)
    throw error("couldn't read from `" + string(fname) + "': " + string(strerror(errno)));

  f.buff[f.len] = '\0';
#endif

  f.fname = (char *) malloc(strlen(fname) + 1);
  strcpy(f.fname, fname);
  
  f.pos = 0;
  f.linenr = 1; f.colnr = 1;
  f.info = info;

  file_stack[file_nest++] = f;

  CURR = &(file_stack[file_nest-1]);
}

int pop_file()
{
  lex_file f;

  if(file_nest <= 0)
    {
      return 0;
    }

  f = file_stack[--file_nest];
  if(file_nest > 0)
    CURR = &(file_stack[file_nest-1]);
  else
    CURR = NULL;

#ifdef USEMMAP
  if(munmap(f.buff, f.len) != 0)
    throw error("couldn't munmap `" + string(f.fname) + "': " + string(strerror(errno)));
#else
  free(f.buff);
#endif
  
  if(close(f.fd) != 0)
    throw error("couldn't close from `" + string(f.fname) + "': " + string(strerror(errno)));
  
  return 1;
}

int curr_line()
{
  assert(file_nest > 0);
  return CURR->linenr;
}

int curr_col()
{
  assert(file_nest > 0);
  return CURR->colnr;
}

char *curr_fname()
{
  assert(file_nest > 0);
  return CURR->fname;
}

char *last_info = 0;

int LConsume(int n)
// consume lexical input
{
  int i;

  assert(n >= 0);

  if(CURR->pos + n > CURR->len)
    {
      fprintf(ferr, "nothing to consume...\n");
      return 0;
    }

  if(CURR->info)
    {
      if(opt_linebreaks)
	{
	  fprintf(fstatus, "\n%s `%s' ", CURR->info, CURR->fname);
	}
      else
	{
	  if(last_info != CURR->info)
	    fprintf(fstatus, "%s `%s'... ", CURR->info, CURR->fname);
	  else
	    fprintf(fstatus, "`%s'... ", CURR->fname);
	}

      last_info = CURR->info;
      CURR->info = NULL;
    }

  for(i = 0; i < n; i++)
    {
      CURR->colnr++;

      if(CURR->buff[CURR->pos + i] == '\n')
	{
	  CURR->colnr = 1;
	  CURR->linenr++;
	  total_lexed_lines ++;
	}
    }

  CURR->pos += n;

  return 1;
}

char *LMark()
{
  if(CURR->pos >= CURR->len)
    {
      return NULL;
    }

  return CURR->buff + CURR->pos;
}

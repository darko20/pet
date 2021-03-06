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

#include "pet-config.h"
#include "lex-io.h"
#include "errors.h"
#include "options.h"
#include "logging.h"

#include <cassert>
#include <cstring>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

using std::string;

static lex_file file_stack[MAX_LEX_NEST];
static int file_nest = 0;
lex_file *CURR;

int total_lexed_lines = 0;

struct lex_location *new_location(const char *fname, int linenr, int colnr)
{
  struct lex_location *loc = (struct lex_location *) malloc(sizeof(struct lex_location));

  loc->fname = fname;
  loc->linenr = linenr;
  loc->colnr = colnr;

  return loc;
}

void push_file(const string &fname, const char *info) {
  lex_file f;
  struct stat statbuf;

  if(file_nest >= MAX_LEX_NEST)
    throw tError(string("too many nested includes (in ") 
                 + fname + ") - giving up");

#ifndef WINDOWS
  f.fd = open(fname.c_str(), O_RDONLY);
#else
  f.fd = open(fname.c_str(), O_RDONLY | O_BINARY);
#endif

  if(f.fd < 0)
    throw tError("error opening `" + fname + "': " + string(strerror(errno)));

  if(fstat(f.fd, &statbuf) < 0)
    throw tError("couldn't fstat `" + fname + "': " + string(strerror(errno)));

  f.len = statbuf.st_size;

#ifdef HAVE_MMAP
  f.buff = (char *) mmap(0, f.len, PROT_READ, MAP_SHARED, f.fd, 0);

  if(f.buff == (caddr_t) -1)
    throw tError("couldn't mmap `" + fname + "': " + string(strerror(errno)));

#else
  f.buff = (char *) malloc(f.len + 1);
  if(f.buff == 0)
    throw tError("couldn't malloc for `" + fname + "': " 
                 + string(strerror(errno)));
  
  if((size_t) read(f.fd,f.buff,f.len) != f.len)
    throw tError("couldn't read from `" + fname + "': "
                 + string(strerror(errno)));

  f.buff[f.len] = '\0';
#endif

  f.fname = strdup(fname.c_str());
  
  f.pos = 0;
  f.linenr = 1; f.colnr = 1;
  f.info = (info != NULL ? strdup(info) : NULL);

  file_stack[file_nest++] = f;

  CURR = &(file_stack[file_nest-1]);
}

void push_string(const string &input, const char *info) {
  lex_file f;

  if(file_nest >= MAX_LEX_NEST)
    throw tError("too many nested includes (in string) - giving up");

  f.buff = strdup(input.c_str());
  if(f.buff == 0)
    throw tError("couldn't strdup for string include: " 
                 + string(strerror(errno)));
  
  f.len = strlen(f.buff);
  f.fname = NULL;
  f.pos = 0;
  f.linenr = 1; f.colnr = 1;
  f.info = (info != NULL ? strdup(info) : NULL);

  file_stack[file_nest++] = f;

  CURR = &(file_stack[file_nest-1]);
} // push_string()

int pop_file() {
  lex_file f;

  if(file_nest <= 0) return 0;

  f = file_stack[--file_nest];
  if(file_nest > 0)
    CURR = &(file_stack[file_nest-1]);
  else
    CURR = NULL;

#ifdef HAVE_MMAP
  if(f.fname) {
    if(munmap(f.buff, f.len) != 0)
      throw tError("couldn't munmap `" + string(f.fname) 
                   + "': " + string(strerror(errno)));
  } // if
  else {
    //
    // even when mmap() is in use, includes from strings were directly copied
    // into the input buffer.
    //
    free(f.buff);
  } // else
#else
  free(f.buff);
#endif
  
  if(f.fname) {
    if(close(f.fd) != 0)
      throw tError("couldn't close from `" + string(f.fname) 
                   + "': " + string(strerror(errno)));
  } // if
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
      LOG(logSyntax, ERROR, "nothing to consume...");
      return 0;
    }

  if(CURR->info)
    {
      {
        if(last_info != CURR->info) {
          LOG(logApplC, INFO, 
              CURR->info << " `" << CURR->fname << "'... ");
        }
        else {
          LOG(logApplC, INFO, "`" << CURR->fname << "'... ");
        }
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

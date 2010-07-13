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

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stack>
#include <boost/shared_ptr.hpp>

#ifdef HAVE_MMAP
#include <sys/mman.h>
#else
#include <io.h>
#endif

#ifdef WIN32
#define WINDOWS
#endif

using std::string;
using boost::shared_ptr;

static std::stack<shared_ptr<Lex_file> > file_stack;
shared_ptr<Lex_file> CURR;

int LLA(int n)
{ 
  if(CURR == NULL || CURR->pos + n >= CURR->len)
    return EOF;

  return CURR->buff[CURR->pos + n];
}

int total_lexed_lines = 0;

void push_file(const string &fname, const char *info)
{
  shared_ptr<Lex_file> f(new Lex_file());
  struct stat statbuf;


#ifndef WINDOWS
  f->fd = open(fname.c_str(), O_RDONLY);
#else
  f->fd = open(fname.c_str(), O_RDONLY | O_BINARY);
#endif

  if(f->fd < 0)
    throw tError("error opening `" + fname + "': " + string(strerror(errno)));

  if(fstat(f->fd, &statbuf) < 0)
    throw tError("couldn't fstat `" + fname + "': " + string(strerror(errno)));

  f->len = statbuf.st_size;

#ifdef HAVE_MMAP
  f->buff = (char *) mmap(0, f->len, PROT_READ, MAP_SHARED, f->fd, 0);

  if(f->buff == (caddr_t) -1)
    throw tError("couldn't mmap `" + fname + "': " + string(strerror(errno)));

#else
  f->buff = new char[f->len + 1];
  
  if((size_t) read(f->fd,f->buff,f->len) != f->len)
    throw tError("couldn't read from `" + fname + "': "
                 + string(strerror(errno)));

  f->buff[f->len] = '\0';
#endif

  f->fname = fname;
  
  if (info != NULL) {
      f->info  = info;
  }

  file_stack.push(f);

  CURR = file_stack.top();
}

void push_string(const string &input, const char *info)
{
  shared_ptr<Lex_file> f(new Lex_file());
  f->buff = new char[input.size()+1];
  strcpy(f->buff, input.c_str());
  f->len = input.size();

  if (info != NULL) {
      f->info  = info;
  }

  file_stack.push(f);

  CURR = file_stack.top();
}

Lex_file::~Lex_file()
{
#ifdef HAVE_MMAP
    if(!fname.empty()) {
        if(munmap(buff, len) != 0)
            throw tError("couldn't munmap `" + string(fname) 
            + "': " + string(strerror(errno)));
    } // if
    else {
        //
        // even when mmap() is in use, includes from strings were directly copied
        // into the input buffer.
        //
        delete[] buff;
    } // else
#else
    delete[] buff;
#endif
  if(!fname.empty()) {
    if(close(fd) != 0)
      throw tError("couldn't close from `" + fname 
                   + "': " + string(strerror(errno)));
  }
}

int pop_file()
{
    if(file_stack.empty()) return 0;

    file_stack.pop();
    if(file_stack.empty()) {
        CURR.reset();
    } else {
        CURR = file_stack.top();
    }
    return 1;
}

int curr_line()
{
  assert(!file_stack.empty());
  return CURR->linenr;
}

int curr_col()
{
  assert(!file_stack.empty());
  return CURR->colnr;
}

std::string curr_fname()
{
  assert(!file_stack.empty());
  return CURR->fname;
}

std::string last_info;

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

  if(!CURR->info.empty())
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
      CURR->info.clear();
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

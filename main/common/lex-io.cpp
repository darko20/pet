/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
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

#ifdef USEMMAP
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#else
#include <sys\stat.h>
#include <io.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>

#include "lex-io.h"
#include "options.h"

#ifdef WINDOWS
#define strcasecmp strcmpi
#define R_OK 0
#endif

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

char *find_file(char *orig, char *ext, bool ext_req)
{
  char *newn;
  struct stat sb;

  if(orig == 0)
    return 0;

  newn = (char *) malloc(strlen(orig) + strlen(ext) + 1);
  
  strcpy(newn, orig);

  if(ext_req == false && access(newn, R_OK) == 0)
    {
      stat(newn, &sb);
      if((sb.st_mode & S_IFDIR) == 0)
        return newn;
    }

  strcat(newn, ext);
      
  if(access(newn, R_OK) == 0)
    {
      stat(newn, &sb);
      if((sb.st_mode & S_IFDIR) == 0)
        return newn;
    }

  return NULL;
}

char *output_name(char *in, char *oldext, const char *newext)
{
  char *out, *ext;

  out = (char *) malloc(strlen(in) + strlen(newext) + 1);

  strcpy(out, in);

  ext = strrchr(out, '.');

  if(ext && strcasecmp(ext, oldext) == 0)
    {
      strcpy(ext, newext);
    }
  else
    {
      strcat(out, newext);
    }

  return out;
}

void push_file(char *fname, char *info)
{
  lex_file f;
  struct stat statbuf;

  if(file_nest >= MAX_LEX_NEST)
    {
      fprintf(ferr, "too many nested includes (in %s)- giving up\n", fname);
      exit(1);
    }

#ifndef WINDOWS
  f.fd = open(fname, O_RDONLY);
#else
  f.fd = open(fname, O_RDONLY | O_BINARY);
#endif

  if(f.fd < 0)
    {
      fprintf(ferr, "error opening %s: %s", fname, strerror(errno));
      exit(1);
    }

  if(fstat(f.fd, &statbuf) < 0)
    {
      fprintf(ferr, "couldn't fstat %s: %s", fname, strerror(errno));
      exit(1);
    }

  f.len = statbuf.st_size;

#ifdef USEMMAP
  f.buff = (char *) mmap(0, f.len, PROT_READ, MAP_SHARED, f.fd, 0);

  if(f.buff == (caddr_t) -1)
    {
      fprintf(ferr, "couldn't mmap %s: %s", fname, strerror(errno));
      exit(1);
    }
#else
  f.buff = (char *) malloc(f.len);
  if(f.buff == 0)
    {
      fprintf(ferr, "couldn't malloc for %s: %s", fname, strerror(errno));
      exit(1);
    }
  
  if((size_t) read(f.fd,f.buff,f.len) != f.len)
    {
      fprintf(ferr, "couldn't fread\n");
      exit(1);
    }
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
    {
      fprintf(ferr, "couldn't munmap %s: %s", f.fname, strerror(errno));
      exit(1);
    }
#else
  free(f.buff);
#endif
  
  if(close(f.fd) != 0)
    {
      fprintf(ferr, "couldn't close %s: %s", f.fname, strerror(errno));
      exit(1);
    }

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

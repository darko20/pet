/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
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

void push_file(char *fname, char *info);
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

//
//      The entire contents of this file, in printed or electronic form, are
//      (c) Copyright YY Technologies, Mountain View, CA 1991,1992,1993,1994,
//                                                       1995,1996,1997,1998,
//                                                       1999,2000,2001
//      Unpublished work -- all rights reserved -- proprietary, trade secret
//

// YY input format and server mode (oe, uc)

#ifndef _YY_H_
#define _YY_H_

#include <stdio.h>
#include <assert.h>

#include "tokenizer.h"

class yy_tokenizer : public tokenizer
{
 public:
  yy_tokenizer(string s) :
    tokenizer(s), _yyinput(strdup(_input.c_str())), _yyalloc(_yyinput) {} ;

  virtual ~yy_tokenizer()
  {
     free(_yyalloc);
  }

  virtual void add_tokens(class input_chart *i_chart);

  virtual string description() { return "YY Inc tokenization"; }

 private:
  char *_yyinput, *_yyalloc;

  bool read_int(int &);
  bool read_string(string &, string &);
  bool read_comma();
  bool read_pos(string &);
  bool read_token(int &, int &, string &, string &, set<string> &);
};

extern int cheap_server_initialize(int);
extern void cheap_server(int);
extern int cheap_server_child(int);

extern int yy_tsdb_summarize_item(class chart &, const char *,
				  int, int, char *);
extern int yy_tsdb_summarize_error(const char *, int, error &);
extern int socket_write(int, char *);
extern int socket_readline(int, char *, int);

#endif

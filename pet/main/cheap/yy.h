//
// The entire contents of this file, in printed or electronic form, are
// (c) Copyright YY Software Corporation, La Honda, CA 2000, 2001.
// Unpublished work -- all rights reserved -- proprietary, trade secret
//

// YY input format and server mode (oe, uc)

#ifndef _YY_H_
#define _YY_H_

#include <stdio.h>
#include <assert.h>

#include <set>
#include <string>
using namespace std;

#include "tokenlist.h"

class yy_tokenlist : public tokenlist
{
 public:
  yy_tokenlist(const char *);
  virtual string description() { return "YY Inc tokenization"; }

  virtual postags POS(int i);

  string surface(int i) { 
    if(i >= 0 && i < _ntokens)
      return _surface[i];
    else
      throw error("index out of bounds for tokenlist");
  } /* raw() */

 private:
  vector<string> _surface;
  vector<set<string> > _postags;
  char *_input;

  bool read_int(int &);
  bool read_string(string &, string &);
  bool read_comma();
  bool read_pos(string &);
  bool read_token(int &, int &, string &, string &, set<string> &);
};

extern int cheap_server_initialize(int);
extern void cheap_server(int);
extern int cheap_server_child(int);
extern int yy_tsdb_summarize_item(chart &, char *, int, int, char *);
extern int yy_tsdb_summarize_error(char *, int, error &);
extern int socket_write(int, char *);
extern int socket_readline(int, char *, int);

#endif

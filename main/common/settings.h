/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* a settings file */

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "lex-tdl.h"

#define SET_SUBDIRECTORY "pet"
#define SET_EXT ".settings"
#define SET_TABLE_SIZE 100

struct setting
{
  char *name;
  int n, allocated;
  char **values;
  struct lex_location* loc;
};

class settings
{
 public:
  settings(const char *name, char *base, char *message = 0);
  ~settings();
  
  setting *lookup(const char *name);
  char *value(const char *name);
  char *req_value(const char *name);
  bool member(const char *name, const char *value);
  // string equality based assoc
  char *assoc(const char *name, const char *key, int arity = 2, int nth = 1);
  // subtype based assoc
  char *sassoc(const char *name, const char *key, int arity = 2, int nth = 1);


  struct lex_location *lloc() { return _lloc; }
  
 private:
  int _n;
  setting **_set;
  char *_fname, *_prefix;
  struct lex_location *_lloc;
  
  void parse();
  void parse_one();
};

#endif

/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * 2001 Bernd Kiefer kiefer@dfki.de
 */

// input_token: the items in the token-list, basically, the "raw" input items

#ifndef _INPUTTOKEN_H_
#define _INPUTTOKEN_H_

#include "dag-common.h"
#include "grammar.h"
#include "settings.h"
#include "postags.h"
#include "paths.h"

class input_chart;

class input_token
{
 public:
  input_token(int id, int s, int e, class full_form ff, string o,
              int p, const postags &pos, const tPaths &paths,
              input_chart *cont, bool synthesized = false);

  ~input_token()
      {}

  inline bool synthesized() { return _synthesized; }

  inline int id() { return _id; }

  inline int start() { return _start; }
  inline void start(int s) { _start = s; }

  inline int end() { return _end; }
  inline void end(int e) { _end = e; }

  inline int startposition() { return _startposition; }
  inline int endposition() { return _endposition; }

  inline string orth() { return _orth; }

  inline int priority() { return _p; }

  inline const postags &get_in_postags() { return _in_pos; }
  inline const postags &get_supplied_postags() { return _supplied_pos; }

  fs instantiate();

  full_form& form() { return _form; }

  void expand(list <class lex_item *> &result);

  list<class lex_item *> generics(int discount, postags onlyfor = postags());

  void print(ostream &f);
  void print(FILE *f);

  void print_derivation(FILE *f, bool quoted, int id,
			int p, int q, list_int *l, string orth);
  void print_yield(FILE *f, list_int *l, list<string> &orth);
  string tsdb_derivation(int id, string orth);
  void getTagSequence(list_int *l, list<string> &orth,
                      list<string> &tags, list<list<string> > &words);
  
  string description();

private:
  bool _synthesized;

  int _id;

  int _start, _end;
  int _startposition, _endposition;

  string _orth;

  postags _in_pos, _supplied_pos;

  int _p;

  full_form _form;

  tPaths _paths;

  input_chart *_container;

  bool add_result(int start, int end, int ndtrs, int keydtr,
                  input_token ** dtrs, list <class lex_item *> &result);
  bool expand_rec(int arg_position, int start, int end, input_token **dtrs,
                  list <class lex_item *> &result);

};

#endif

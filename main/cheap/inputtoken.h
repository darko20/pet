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

class input_chart;

class input_token
{
 public:
  input_token(int s, int e, class full_form ff, string o,
	      int p, const postags &pos, input_chart *cont,
	      bool synthesized = false);

  ~input_token()
    {}

  bool operator==(input_token const &t) const
  {
    return (_start == t._start && _end == t._end
	    && _form == t._form && _orth == t._orth);
  }
  
  bool operator!=(input_token const &t) const
  {
    return !(*this == t);
  }

  bool operator<(input_token const &t) const
  {
    if(_start == t._start)
      {
	if(_end == t._end)
	  {
	    if(_form == t._form)
	      {
		if(_orth == t._orth)
		  return false;
		else
		  return _orth < t._orth;
	      }
	    else
	      return _form < t._form;
	  }
	else
	  return _end < t._end;
      }
    else
      return _start < t._start;
  }

  inline bool synthesized() { return _synthesized; }

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

  void print_derivation(FILE *f, bool quoted, int offset, int id,
			int p, int q, list_int *l, string orth);
  string tsdb_derivation(int offset, int id, string orth);

  string description();

private:
  bool _synthesized;

  int _start, _end;
  int _startposition, _endposition;

  string _orth;

  postags _in_pos, _supplied_pos;

  int _p;

  full_form _form;

  input_chart *_container;

  void add_result(int start, int end, int ndtrs, int keydtr,
		  input_token ** dtrs, list <class lex_item *> &result);
  void expand_rec(int arg_position, int start, int end, input_token **dtrs,
		   list <class lex_item *> &result);

};

#endif

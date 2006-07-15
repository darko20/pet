/* -*- Mode: C++ -*- */
/** \file yy-tokenizer.h
 * Reader for YY input mode strings, implemented as tokenizer input module.
 */

#ifndef _YY_TOKENIZER_H
#define _YY_TOKENIZER_H

#include "input-modules.h"

/** A tokenizer for yy input mode */
class tYYTokenizer : public tTokenizer {
public:
  /** Create a new yy mode input reader.
   * \param position_mapping if \c STANDOFF_COUNTS, an item starting, e.g., at
   *        position zero and ending at position one has length two, not one.
   * \param classchar if a stem begins with this character, it is interpreted
   *        as type name rather than as base form.
   */
  tYYTokenizer(position_map position_mapping = STANDOFF_POINTS, char classchar = '$');

  /** Produce a set of tokens from the given string. */
  virtual void tokenize(myString s, inp_list &result);
  
  /** A string to describe the module */
  virtual string description() { return "YY style tokenizer"; }

  /** Return \c true if the position in the returned tokens are counts instead
   *  of positions.
   */
  virtual position_map position_mapping() { return _position_mapping ; }

private:
  /** Is the tokenizer at the end of the file? */
  bool eos();
  /** Return the current char */
  char cur();
  /** Return the rest of the input buffer */
  const char *res();
  /** Advance the tokenizer \a n characters and return true if not past
   *  end-of-file.
   */
  bool adv(int n = 1);
  /** Read off whitespace */
  bool read_ws();
  /** Read off a special character \a c and return \c true on success. */
  bool read_special(char c);
  /** Read off an integer into \a target and return \c true on success. */
  bool read_int(int &target);
  /** Read off a double into \a target and return \c true on success. */
  bool read_double(double &target);
  /** Read off a string into \a target and return \c true on success.
   * \arg quotedp if true, assume the string is surrounded by double quotes,
   *      otherwise, read as long as all chars are \c id_char \see lex-tdl.cpp
   * \arg downcasep if true, convert string to downcase
   */
  bool read_string(string &target, bool quotedp, bool downcasep = false);
  /** Read a pair of string and double, which represents a part of speech and
   *  its probability.
   */
  bool read_pos(string &tag, double &prob);

  /** Get the next token */
  class tInputItem *read_token();

  position_map _position_mapping;
  string _yyinput;
  string::size_type _yypos;
  char _class_name_char;
};

#endif

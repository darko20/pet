/* =========================================================== */
/* ================== file is l2lib.h  ======================= */
/* =========================================================== */
#ifndef _L2LIB_H
#define _L2LIB_H

#include <string.h>
#include <vector.h>

/*	The entire contents of this file, in printed or electronic form, are
	(c) Copyright YY Technologies, Mountain View, CA 1999,2000,2001
	Unpublished work -- all rights reserved -- proprietary, trade secret
*/

/*
   Interface to the Level 2 parser library.
*/

/* Revision History:
   01/10/05	UC	created
*/

// Error conditions are communicated to the caller by throwing an exception
// of type l2_error; no other exceptions are thrown. When using any of the
// functions provided here, the caller should catch exceptions of type
// l2_error.

class l2_error
{
 public:
  l2_error(string s) : description(s) {}
  string description;
};

// forms and rules record the derivation history, e.g. one analysis for
// `singings' is:
// forms: sing                    singing                     singings
// rules:      prp_verb_infl_rule         plur_noun_infl_rule
// forms always contains one more element than rules

class l2_morph_analysis
{
 public:
  vector<string> forms;
  vector<string> rules;
};

// initialize parser with specified grammar
extern void l2_parser_init(const string& grammar_path,
                           const string& log_file_path,
                           int k2y_segregation_p);

// parse one input item; return string of K2Y's. Optionally, skip first n
// analyses.
extern string l2_parser_parse(const string& input, int nskip = 0);

// morphological analysis of one token
extern vector<l2_morph_analysis> l2_morph_analyse(const string& form);

// write tsdb profiling information, including roletable
extern void l2_tsdb_write(FILE *f_parse, FILE *f_result, FILE *f_item, const string &rt);

// is the input string enirely made up of punctuation?
extern bool l2_parser_punctuationp(const string &input);

// free resources of parser
extern void l2_parser_exit();

#endif // _L2LIB_H

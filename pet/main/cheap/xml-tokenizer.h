/* -*- Mode: C++ -*- */
/** \file xml-tokenizer.h
 * XML input mode reader for PET, similar to the YY mode tokenizer.
 */

#ifndef _XML_TOKENIZER_H
#define _XML_TOKENIZER_H

#include "input-modules.h"
#include "pic-handler.h"

/** Tokenizer similar to the yy mode tokenizer, using a custom XML DTD giving
 *  more flexibility for the input stage, and also maybe more clarity.
 */
class tXMLTokenizer : public tTokenizer {
public:
  /** Create a new XML input reader.
   *  \param positions_are_counts if \c true, an item starting, e.g., at
   *  position zero and ending at position one has length two, not one.
   */
  tXMLTokenizer(bool positions_are_counts = false)
    : tTokenizer(), _positions_are_counts(positions_are_counts) { }
  
  virtual ~tXMLTokenizer() {}

  /** Produce a set of tokens from the given string. */
  virtual void tokenize(myString inputfile, inp_list &result) {
      PICHandler picreader;
      if (parse_file(inputfile.c_str(), &picreader)
          && (! picreader.error())) {
        result.splice(result.begin(), picreader.items());
      }
  }
  
  /** A string to describe the module */
  virtual string description() { return "XML input chart reader"; }

  /** Return \c true if the position in the returned tokens are counts instead
   *  of positions.
   */
  virtual bool positions_are_counts() { return _positions_are_counts ; }

private:
  bool _positions_are_counts;
};

#endif

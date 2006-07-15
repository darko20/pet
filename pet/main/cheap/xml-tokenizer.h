/* -*- Mode: C++ -*- */
/** \file xml-tokenizer.h
 * XML input mode reader for PET, similar to the YY mode tokenizer.
 */

#ifndef _XML_TOKENIZER_H
#define _XML_TOKENIZER_H

#include "input-modules.h"
#include "pic-handler.h"
#include "xercesc/framework/MemBufInputSource.hpp"
#include "xercesc/framework/LocalFileInputSource.hpp"
#include "xercesc/util/XMLString.hpp"

/** Tokenizer similar to the yy mode tokenizer, using a custom XML DTD giving
 *  more flexibility for the input stage, and also maybe more clarity.
 */
class tXMLTokenizer : public tTokenizer {
public:
  /** Create a new XML input reader.
   *  \param position_map if \c STANDOFF_COUNTS, an item starting, e.g., at
   *  position zero and ending at position one has length two, not one.
   */
  tXMLTokenizer(position_map position_mapping = STANDOFF_POINTS)
    : tTokenizer(), _position_mapping(position_mapping) { }
  
  virtual ~tXMLTokenizer() {}

  /** Produce a set of tokens from the given XML input. */
  virtual void tokenize(string input, inp_list &result);
  
  /** A string to describe the module */
  virtual string description() { return "XML input chart reader"; }

  /** Return \c true if the position in the returned tokens are counts instead
   *  of positions.
   */
  virtual position_map position_mapping() { return _position_mapping ; }
  void set_position_mapping(position_map position_mapping) { 
    _position_mapping = position_mapping ; }

private:
  /** Produce a set of tInputItem tokens from the given XML input on stdin. */
  void tokenize_from_stream(string input, inp_list &result);

  /** Produce a set of tokens from the given XML file. */
  void tokenize_from_file(string filename, inp_list &result);

  position_map _position_mapping;
};

#endif

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
   *  \param positions_are_counts if \c true, an item starting, e.g., at
   *  position zero and ending at position one has length two, not one.
   */
  tXMLTokenizer(bool positions_are_counts = false)
    : tTokenizer(), _positions_are_counts(positions_are_counts) { }
  
  virtual ~tXMLTokenizer() {}

  /** Produce a set of tokens from the given XML input. */
  virtual void tokenize(string input, inp_list &result) {
    if (input.compare(0, 2, "<?") == 0)
      tokenize_from_stream(input, result);
    else
      tokenize_from_file(input, result);
  }

  
  /** A string to describe the module */
  virtual string description() { return "XML input chart reader"; }

  /** Return \c true if the position in the returned tokens are counts instead
   *  of positions.
   */
  virtual bool positions_are_counts() { return _positions_are_counts ; }

private:
  /** Produce a set of tokens from the given XML input on stdin. */
  void tokenize_from_stream(string input, inp_list &result) {
    string buffer = input;
    if(verbosity > 4)
      {
	//cerr << "received from :pic preprocessor:" << endl << buffer << endl << endl;
	fprintf(ferr, "[processing PIC XML input]\n");
      };
    
    PICHandler picreader(true, _translate_iso_chars);
    MemBufInputSource xmlinput((const XMLByte *) buffer.c_str()
                               , buffer.length(), "STDIN");
    if (parse_file(xmlinput, &picreader)
        && (! picreader.error())) {
      result.splice(result.begin(), picreader.items());
    }
  }

  /** Produce a set of tokens from the given XML file. */
  void tokenize_from_file(string filename, inp_list &result) {
    PICHandler picreader(true, _translate_iso_chars);
    XMLCh * XMLFilename = XMLString::transcode(filename.c_str());
    LocalFileInputSource inputfile(XMLFilename);
    if (parse_file(inputfile, &picreader)
        && (! picreader.error())) {
      result.splice(result.begin(), picreader.items());
    }
    XMLString::release(&XMLFilename);
  }


  bool _positions_are_counts;
};

#endif

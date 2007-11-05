/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 *   2004 Bernd Kiefer bk@dfki.de
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "pic-tokenizer.h"
#include "cheap.h"

using std::string;
using std::list;

XERCES_CPP_NAMESPACE_USE

bool tPICTokenizer::next_input(std::istream &in, std::string &input) {
  input.clear();
  string nextline;
  do {
    std::getline(in, nextline);
    input += nextline;
  } while (! nextline.empty());
  return !input.empty();
}

/** Produce a set of tokens from the given XML input. */
void tPICTokenizer::tokenize(string input, inp_list &result) {

  if (input.compare(0, 2, "<?") == 0)
    tokenize_from_stream(input, result);
  else {
    string::size_type len = input.length();
    string::size_type pos = input.find_last_of('\n');
    // trim trailing newlines
    while (pos != string::npos && pos == len - 1) {
      input.erase(pos);
      len--;
      pos = input.find_last_of('\n');
    }
    tokenize_from_file(input, result);
  }
}

/** Produce a set of tInputItem tokens from the given XML input on stdin. */
void tPICTokenizer::tokenize_from_stream(string input, inp_list &result) {
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
    result = picreader.items();
    //    result.splice(result.begin(), picreader.items());
  }
}

/** Produce a set of tokens from the given XML file. */
void tPICTokenizer::tokenize_from_file(string filename, inp_list &result) {
  PICHandler picreader(true, _translate_iso_chars);
  XMLCh * XMLFilename = XMLString::transcode(filename.c_str());
  LocalFileInputSource inputfile(XMLFilename);
  if (parse_file(inputfile, &picreader)
      && (! picreader.error())) {
    result.splice(result.begin(), picreader.items());
  }
  XMLString::release(&XMLFilename);
}


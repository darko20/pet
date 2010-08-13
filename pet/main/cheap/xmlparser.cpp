/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
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

#include "xmlparser.h"

#include "cheap.h"
#include "logging.h"

#include <boost/scoped_array.hpp>

#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/sax/SAXException.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>

using namespace XERCES_CPP_NAMESPACE;
using std::basic_string;

/******************************************************************************
 String Encoding/Decoding Helpers
 *****************************************************************************/

// global objects used in XMLCh --> UTF-8 String transformation
MemBufFormatTarget *membuf;
XMLFormatter *utf8_formatter;
XMLFormatter *latin_formatter;

/** Convert an XMLCh string into UTF8.
 *
 * \attn be aware that this function always uses the same buffer, so you can
 * not, for example, use it twice in one \c printf call because one result will
 * be overwritten by the other. To be save, you either have to copy the result
 * or do two \c printf calls with two calls to this function
 */
const char * XMLCh2UTF8(const XMLCh *in) {
  membuf->reset();
  (*utf8_formatter) << in;
  return (const char *) membuf->getRawBuffer();
}

std::string XMLCh2Native(const XMLCh * const str) {
  boost::scoped_array<char> ptr(xercesc::XMLString::transcode(str));
  return (ptr != 0) ? std::string(ptr.get()) : "";
}

std::string XMLCh2Native(basic_string<XMLCh> str) {
  return XMLCh2Native(str.c_str());
}

basic_string<XMLCh> Native2XMLCh(const char *str) {
  boost::scoped_array<XMLCh> ptr(xercesc::XMLString::transcode(str));
  return (ptr != 0) ? basic_string<XMLCh>(ptr.get()) : basic_string<XMLCh>();
}

basic_string<XMLCh> Native2XMLCh(const std::string &str) {
  return Native2XMLCh(str.c_str());
}


bool parse_file(InputSource &inp, HandlerBase *docHandler){
  SAXParser parser;

  parser.setValidationScheme(SAXParser::Val_Auto);
  parser.setDoNamespaces(true);    // optional
  parser.setDoSchema(true);
  parser.setValidationSchemaFullChecking(true);
  parser.setExitOnFirstFatalError(true);

  parser.setDocumentHandler(docHandler);
  parser.setErrorHandler(docHandler);

  try {
    parser.parse(inp);
  }
  catch (const XMLException& toCatch) {
    LOG(logXML, WARN, "unknown:" << toCatch.getSrcLine()
        << ": error: SAX: " << XMLCh2Native(toCatch.getMessage()));
    return false;
  }
  catch (const SAXParseException& toCatch) {
    docHandler->error(toCatch);
    return false;
  }
  // an XMLAttributeError is catched in startElement
  return true;
}

bool xml_initialize() {
  try {
    XMLPlatformUtils::Initialize();
    membuf = new MemBufFormatTarget();
    latin_formatter = new XMLFormatter("iso-8859-1", "1.0", membuf);
    utf8_formatter = new XMLFormatter("UTF-8", "1.0", membuf);
  }
  catch (const XMLException& toCatch) {
    // we don't know if we have an encoder available
    LOG(logXML, ERROR, "SAX: Error during XML initialization!");
    return false;
  }
  return true;
}

bool xml_finalize() {
  delete utf8_formatter;
  delete latin_formatter;
  delete membuf;
  XMLPlatformUtils::Terminate();
  return true;
}

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/sax/SAXException.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include "xmlparser.h"
#include "cheap.h"
#include "logging.h"

using namespace XERCES_CPP_NAMESPACE;

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

/** Convert an XMLCh string into the latin codepage.
 *
 * \attn be aware that this function always uses the same buffer, so you can
 * not, for example, use it twice in one \c printf call because one result will
 * be overwritten by the other. To be save, you either have to copy the result
 * or do two \c printf calls with two calls to this function
 */
const char * XMLCh2Latin(const XMLCh *in) {
  membuf->reset();
  (*latin_formatter) << in;
  return (const char *) membuf->getRawBuffer();
}



bool parse_file(InputSource &inp, HandlerBase *docHandler){
  SAXParser parser;

  parser.setValidationScheme(SAXParser::Val_Auto);
  //parser.setDoValidation(true); // deprecated, replaced by above fn
  parser.setDoNamespaces(true);    // optional
  parser.setDoSchema(true);
  parser.setValidationSchemaFullChecking(true);
  // parser.setValidationConstraintFatal(true);
  parser.setExitOnFirstFatalError(true);

  parser.setDocumentHandler(docHandler);
  parser.setErrorHandler(docHandler);

  try {
    parser.parse(inp);
  }
  catch (const XMLException& toCatch) {
    LOG_ERROR(loggerXml, "SAX: XMLException line %d: %s",
              toCatch.getSrcLine(), XMLCh2Latin(toCatch.getMessage()));
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
    LOG_ERROR(loggerXml, "SAX: Error during XML initialization!");
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

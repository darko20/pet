#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/sax/SAXException.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include "xmlparser.h"
#include "cheap.h"

using namespace XERCES_CPP_NAMESPACE;

/******************************************************************************
 String Encoding/Decoding Helpers
 *****************************************************************************/

// global objects used in XMLCh --> UTF-8 String transformation
MemBufFormatTarget *membuf;
XMLFormatter *utf8_formatter;
XMLFormatter *latin_formatter;

const char * XMLCh2UTF8(const XMLCh *in) {
  membuf->reset();
  (*utf8_formatter) << in;
  return (const char *) membuf->getRawBuffer();
}

const char * XMLCh2Latin(const XMLCh *in) {
  membuf->reset();
  (*latin_formatter) << in;
  return (const char *) membuf->getRawBuffer();
}

/******************************************************************************
 Attribute helper functions
 *****************************************************************************/

const XMLCh yes_str[] = { chLatin_y,  chLatin_e,  chLatin_s, chNull };

bool req_bool_attr(AttributeList& attr, char *aname) {
  const XMLCh *str;
  if ((str = attr.getValue(aname)) == NULL)
    throw(XMLAttributeError("missing attribute", aname));
  return (XMLString::compareIString(str, yes_str) == 0);
}

bool opt_bool_attr(AttributeList& attr, char *aname, bool def) {
  const XMLCh *str;
  if ((str = attr.getValue(aname)) == NULL) return def;
  return (XMLString::compareIString(str, yes_str) == 0);
}

int req_int_attr(AttributeList& attr, char *aname) {
  const XMLCh *val;
  if ((val = attr.getValue(aname)) == NULL)
    throw(XMLAttributeError("missing attribute", aname));
  const char *str = XMLCh2UTF8(val);
  char *end;
  int res = strtol(str, &end, 10);
  if ((*end != '\0') || (str == end))
    throw(XMLAttributeError("wrong int value", aname));
  return res;
}

int opt_int_attr(AttributeList& attr, char *aname, int def) {
  const XMLCh *val;
  if ((val = attr.getValue(aname)) == NULL) return def;
  const char *str = XMLCh2UTF8(val);
  char *end;
  int res = strtol(str, &end, 10);
  if ((*end != '\0') || (str == end))
    throw(XMLAttributeError("wrong int value", aname));
  return res;
}

double req_double_attr(AttributeList& attr, char *aname) {
  const XMLCh *val;
  if ((val = attr.getValue(aname)) == NULL) 
    throw(XMLAttributeError("missing attribute", aname));
  const char *str = XMLCh2UTF8(val);
  char *end;
  double res = strtod(str, &end);
  if ((*end != '\0') || (str == end))
    throw(XMLAttributeError("wrong double value", aname));
  return res;
}

double opt_double_attr(AttributeList& attr, char *aname, double def) {
  const XMLCh *val;
  if ((val = attr.getValue(aname)) == NULL) return def;
  const char *str = XMLCh2UTF8(val);
  char *end;
  double res = strtod(str, &end);
  if ((*end != '\0') || (str == end))
    throw(XMLAttributeError("wrong double value", aname));
  return res;
}

const char *req_string_attr(AttributeList &attr, char *aname) {
  const XMLCh *val = attr.getValue(aname);
  if (val == NULL) throw(XMLAttributeError("missing attribute", aname));
  return XMLCh2UTF8(val);
}

const char *opt_string_attr(AttributeList &attr, char *aname) {
  const XMLCh *val = attr.getValue(aname);
  return (val == NULL) ? "" : XMLCh2UTF8(val);
}



void print_sax_exception(const char * errtype, const SAXParseException& e) {
  fprintf(ferr, "\n%s at file %s, line %d, column %d\n Message: %s\n"
          , errtype, XMLCh2Latin(e.getSystemId())
          , (int) e.getLineNumber(), (int) e.getColumnNumber()
          , XMLCh2Latin(e.getMessage()));
}

void XMLAttributeError::print(FILE *f) {
  fprintf(f, "%s : \"%s\"\n", _msg
          , (_arg_is_char_ptr ? (char *) _arg : XMLCh2Latin(_arg)));
}

bool parse_file(const char *xmlFile, HandlerBase *docHandler){
  SAXParser parser;

  parser.setValidationScheme(SAXParser::Val_Auto);
  parser.setDoValidation(true);    // optional.
  parser.setDoNamespaces(true);    // optional
  parser.setDoSchema(true);
  parser.setValidationSchemaFullChecking(true);
  parser.setExitOnFirstFatalError(true);

  parser.setDocumentHandler(docHandler);
  parser.setErrorHandler(docHandler);

  try {
    parser.parse(xmlFile);
  }
  catch (const XMLException& toCatch) {
    fprintf(ferr, "Exception message is: %s\n"
            , XMLCh2Latin(toCatch.getMessage()));
    return false;
  }
  catch (const SAXParseException& toCatch) {
    fprintf(ferr, "Exception message is: %s\n"
            , XMLCh2Latin(toCatch.getMessage()));
    return false;
  }
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
    fprintf(ferr, "Error during XML initialization!\n");
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

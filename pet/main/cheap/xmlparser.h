/* -*- Mode: C++ -*- */
/** \file xmlparser.h
 * Facilities to parse XML files containing grammars, options or wordlists,
 * etc. etc.
 */
#ifndef _XMLPARSER_H
#define _XMLPARSER_H

#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <xercesc/util/XMLString.hpp>
#include <string>
#include <cstdio>

/** An error for unknown or missing XML attributes */
class Error {
  XMLCh *_msg;

public:
  Error(std::string msg) {
    _msg = XERCES_CPP_NAMESPACE_QUALIFIER XMLString::transcode(msg.c_str());
  }

  Error(const char * const msg) {
    _msg = XERCES_CPP_NAMESPACE_QUALIFIER XMLString::transcode(msg);
  }

  Error(const Error &e) {
    _msg = XERCES_CPP_NAMESPACE_QUALIFIER XMLString::replicate(e._msg);
  }

  ~Error() {
    XERCES_CPP_NAMESPACE_QUALIFIER XMLString::release(&_msg);
  }

  const XMLCh *getMessage() const { return _msg; }
};

using XERCES_CPP_NAMESPACE_QUALIFIER AttributeList;

/** Parse an XML file using the given SAX Document Handler.
 * \param inp The XML input source (stdin or file input)
 * \param dochandler The document handler to use (for different file contents).
 * \returns \c true, if no fatal error occurred, \c false otherwise.
*/
bool parse_file(XERCES_CPP_NAMESPACE_QUALIFIER InputSource &inp
                , XERCES_CPP_NAMESPACE_QUALIFIER HandlerBase *dochandler);

/** Initialize XML parsing services */
bool xml_initialize();
/** Finalize XML parsing services */
bool xml_finalize();

/** Convert the XMLCh string \a in into a UTF-8 encoded string */
const char *XMLCh2UTF8(const XMLCh *in);

/** Convert the XMLCh string \a in into a latin-1 encoded string */
const char *XMLCh2Latin(const XMLCh *in);

#endif

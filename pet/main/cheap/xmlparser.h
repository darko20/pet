/* -*- Mode: C++ -*- */
/** \file xmlparser.h
 * Facilities to parse XML files containing grammars, options or wordlists,
 * etc. etc.
 */
#ifndef _XMLPARSER_H
#define _XMLPARSER_H

#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/util/XMLString.hpp>
#include <string>
#include <cstdio>

/** An error for unknown or missing XML attributes */
class XMLAttributeError {
  const char *_msg;
  const XMLCh *_arg;
  bool _arg_is_char_ptr;
  
public:
  XMLAttributeError(const char *message, const XMLCh *arg)
    : _msg(message), _arg(arg), _arg_is_char_ptr(false) {}
  XMLAttributeError(const char *message, std::string arg) 
    : _msg(message), _arg((const XMLCh *) arg.c_str())
    , _arg_is_char_ptr(true) {}

  void print(FILE *f);
};

using XERCES_CPP_NAMESPACE_QUALIFIER AttributeList;

/** @name Attribute Helper Functions
 * Helper Functions for different types of attributes with error handling.
 */
/*@{*/
bool req_bool_attr(AttributeList& attr, char *aname);
bool opt_bool_attr(AttributeList& attr, char *aname, bool def);
int req_int_attr(AttributeList& attr, char *aname);
int opt_int_attr(AttributeList& attr, char *aname, int def);
double req_double_attr(AttributeList& attr, char *aname);
double opt_double_attr(AttributeList& attr, char *aname, double def);
const char *req_string_attr(AttributeList &attr, char *aname);
const char *opt_string_attr(AttributeList &attr, char *aname);
/*@}*/

/** Parse an XML file using the given SAX Document Handler.
 * \param xmlFile The name of the XML file.
 * \param dochandler The document handler to use (for different file contents).
 * \returns \c true, if no fatal error occurred, \c false otherwise.
*/
//bool parse_file(const XMLCh *xmlFile
//                , XERCES_CPP_NAMESPACE_QUALIFIER HandlerBase *dochandler);
bool parse_file(const char *xmlFile
                , XERCES_CPP_NAMESPACE_QUALIFIER HandlerBase *dochandler);

/** Initialize XML parsing services */
bool xml_initialize();
/** Finalize XML parsing services */
bool xml_finalize();

/** Convert the XMLCh string \a in into a UTF-8 encoded string */
const char *XMLCh2UTF8(const XMLCh *in);

/** Convert the XMLCh string \a in into a latin-1 encoded string */
const char *XMLCh2Latin(const XMLCh *in);

/** Print a SAX parse exception to the error log */
void print_sax_exception(const char * errtype
                         , const XERCES_CPP_NAMESPACE_QUALIFIER
                                 SAXParseException& e);

#endif

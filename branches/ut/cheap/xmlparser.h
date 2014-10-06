/* -*- Mode: C++ -*- */
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

/** \file xmlparser.h
 * Facilities to parse XML files containing grammars, options or wordlists,
 * etc. etc.
 */
#ifndef _XMLPARSER_H
#define _XMLPARSER_H

#include "pet-config.h"

#include <string>
#include <cstdio>
#include <iostream>

#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <xercesc/util/XMLString.hpp>

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

/** Parse an XML file using the given SAX Document Handler.
 * \param inp The XML input source (stdin or file input)
 * \param dochandler The document handler to use (for different file contents).
 * \return \c true, if no fatal error occurred, \c false otherwise.
 */
bool parse_file(XERCES_CPP_NAMESPACE_QUALIFIER InputSource &inp
                , XERCES_CPP_NAMESPACE_QUALIFIER HandlerBase *dochandler);

/** Initialize XML parsing services */
bool xml_initialize();
/** Finalize XML parsing services */
bool xml_finalize();

/** Convert the XMLCh string \a in into a UTF-8 encoded string */
const char *XMLCh2UTF8(const XMLCh *in);

/** Converts from a native string to an XMLCh string. */
std::string XMLCh2Native(const XMLCh *str);

/** Converts from a native string to an XMLCh string. */
std::string XMLCh2Native(std::basic_string<XMLCh> str);

/** Converts from an XMLCh string to a native string. */
std::basic_string<XMLCh> Native2XMLCh(const char *str);

/** Converts from an XMLCh string to a native string. */
std::basic_string<XMLCh> Native2XMLCh(const std::string &str);

#endif

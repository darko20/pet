/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
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

#include "mrs-handler.h"
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include "unicode.h"
#include <stack>
#include "mrs-states.h"

using namespace std;
using namespace XERCES_CPP_NAMESPACE;
using XERCES_CPP_NAMESPACE_QUALIFIER AttributeList;

extern FILE *ferr;

MRSHandler::MRSHandler(bool downcase) {
  _state_factory = new mrs::mrs_state_factory(this);
  _state_stack.push(new mrs::TOP_state(this));
  _err = 0;
  _error_occurred = false;
  _downcase_strings = downcase;
  _errbuf = NULL;
}

MRSHandler::~MRSHandler() {
  while (!_state_stack.empty()) {
    delete _state_stack.top();
    _state_stack.pop();
  }
  delete _state_factory;
  if (_errbuf != NULL) XMLString::release(&_errbuf);
}

void MRSHandler::
startElement(const XMLCh* const tag, AttributeList& attrs) {
  if (_err > 0) {
    _err ++;
  } else {
    mrs::mrs_base_state *oldState = _state_stack.top();
    mrs::mrs_base_state *newState = _state_factory->get_state(tag);
    if (newState != NULL) {
      try {
        oldState->changeTo(newState, attrs);
        _state_stack.push(newState);
      } catch (const Error &e) {
        delete newState;
        _error_occurred = true;
        _err ++;
        throw SAXParseException(e.getMessage(), *_loc);
      }
    } else {
      _error_occurred = true;
      throw SAXParseException(errmsg((std::string) "unknown tag '" + XMLCh2Latin(tag) + "'"), *_loc);
    }
  }
}

void MRSHandler::
endElement(const XMLCh* const tag) {
  if (_err == 0) {
    mrs::mrs_base_state *oldState = _state_stack.top();
    _state_stack.pop();
    mrs::mrs_base_state *newState = _state_stack.top();
    try {
      newState->changeFrom(oldState);
    } catch (const Error &e) {
      throw SAXParseException(e.getMessage(), *_loc);
    }
    delete oldState;
  } else {
    _err --;
  }
}

void MRSHandler::
characters(const XMLCh *const chars, const unsigned int len) {
  _state_stack.top()->characters(chars, len);
}

void MRSHandler::
print_sax_exception(const char * errtype, const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e) {
  fprintf(ferr, "SAX: %s %s:%d:%d: "
          , errtype, XMLCh2Latin(e.getSystemId())
          , (int) e.getLineNumber(), (int) e.getColumnNumber()) ;
  fprintf(ferr, "%s\n", XMLCh2Latin(e.getMessage()));
}

string 
MRSHandler::surface_string(const XMLCh *chars, const unsigned int len) const {
  XMLCh* res;
  res = new XMLCh[len + 1];
  XMLString::copyNString(res, chars, len);
  res[len] = chNull;
  XMLString::trim(res);   // strip blanks
  if (_downcase_strings) { XMLString::lowerCase(res); }
  // transfer to unicode UTF-8 string
  string result;
#ifdef HAVE_ICU
  result = Conv->convert((UChar *) res, XMLString::stringLen(res));
#else
  result = XMLCh2Latin(res);
#endif
  delete[] res;
  return result;
}


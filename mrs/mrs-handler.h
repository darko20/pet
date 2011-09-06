/* -*- Mode: C++ -*- */

#ifndef _MRS_HANDLER_H_
#define _MRS_HANDLER_H_

#include "mrs.h"
#include "xmlparser.h"
#include "hash.h"
#include "hashing.h"
#include <stack>
#include <list>

using XERCES_CPP_NAMESPACE_QUALIFIER AttributeList;

namespace mrs {
  class mrs_base_state;
  class mrs_state_factory;
}

/** A SAX parser handler class to read MRS XML (mrx)
 *
 * MrsHandler maintains a stack of states that represent the XML
 * nodestack. The states communicated with each other to pass
 * subresults up or down.
 *  
 * For a detailed description, see the Xerces C++ API.
 */
class MrsHandler : public XERCES_CPP_NAMESPACE_QUALIFIER HandlerBase {
public:
  /** Constructor: Make a new MrsHandler */
  MrsHandler(bool downcase);
  /** Destructor */
  ~MrsHandler();

  /** Start XML element with tag name \a tag and attribute list \a attrs */
  virtual void startElement(const XMLCh* const tag, AttributeList& attrs);
  /** End XML element with tag name \a tag */
  virtual void endElement(const XMLCh* const tag);
  /** The characters event */
  virtual void characters (const XMLCh *const chars, const unsigned int len);
  /** ErrorHandler Interface */
  /*@{*/
  /** An XML error occured */
  virtual void error(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e){
    print_sax_exception("Error", e);
    _error_occurred = true;
  }
  /** A fatal XML error occured */
  virtual void fatalError(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e){
    print_sax_exception("Fatal Error", e);
    _error_occurred = true;
  }
  /** An XML warning should be issued */
  virtual void warning(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e){
    print_sax_exception("Warning", e);
    _error_occurred = true;
  }
  /*@}*/

  /** Did an error occur during processing? */
  bool error() const { return _error_occurred; }

  /** Transform the XMLCh string into the internally used representation.
   *
   * The string is translated into UTF-8 and all leading and trailing
   * whitespace is removed. if \c _downcase_strings is \c true, the string is
   * converted to lower case.
   */
  std::string
  surface_string(const XMLCh *chars, const unsigned int len) const;


  std::list<mrs::tMrs*>  &mrss() {return _mrss;}
  
private:
  /** Copy construction is disallowed */
  MrsHandler(const MrsHandler &x) {}

  /** Print a SAX exception in a convenient form */
  void print_sax_exception(const char * errtype, 
                           const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e);

  /** The state factory that produces the appropriate state for each XML tag */
  class mrs::mrs_state_factory *_state_factory;

  const XERCES_CPP_NAMESPACE_QUALIFIER Locator *_loc;
  
  XMLCh *_errbuf;

  const XMLCh * errmsg(std::string msg) {
    if (_errbuf != NULL) XERCES_CPP_NAMESPACE_QUALIFIER XMLString::release(&_errbuf) ;
    _errbuf = XERCES_CPP_NAMESPACE_QUALIFIER XMLString::transcode(msg.c_str());
    return _errbuf;
  }

  /** The embedding level of erroneous subunits to skip */
  int _err;

  /** A flag to register that an error occured during processing */
  bool _error_occurred;

  
  /** The current stack of XML states (nodes) */
  std::stack<class mrs::mrs_base_state *> _state_stack;

  /** A flag to indicate that all strings should be lower case (or not) */
  bool _downcase_strings;

  struct string_hash
  {
    inline size_t operator()(const std::string &key) const
    {
      return ::bjhash((const ub1 *) key.data(), key.size(), 0);
    }
  };

  std::list<mrs::tMrs*> _mrss;
};


#endif

/* -*- Mode: C++ -*- */
/** \file pic-handler.h
 * A SAX parser handler class to read xtdl grammar definitions.
 */

#ifndef _GRAMMAR_HANDLER_H
#define _GRAMMAR_HANDLER_H

#include "hash.h"
#include "hashing.h"
#include "xmlparser.h"
#include <stack>
#include <list>
//#include <string>

XERCES_CPP_NAMESPACE_USE

/** Namespace for pet input chart xml parser */
namespace pic {
  class pic_base_state;
  class pic_state_factory;
}

/** A SAX parser handler class to read xtdl grammar definitions 
 *
 * PICHandler maintains a stack of states that represent the XML nodes
 * stack. The states communicate with each other to pass subresults up or down,
 * share coreference bindings and the like. For more information about this
 * interaction, see sax_grammar_state
 *
 * For a detailed description, of HandlerBase see the Xerces C++ API.
 */
class PICHandler : public HandlerBase {
public:
  /** Constructor: Make a new PICHandler */
  PICHandler(bool downcase, bool translate_iso);
  /** Destructor */
  ~PICHandler();
  
  /** Start XML element with tag name \a tag and attribute list \a attrs */
  virtual void startElement(const XMLCh* const tag, AttributeList& attrs);
  /** End XML element with tag name \a tag */
  virtual void endElement(const XMLCh* const);
  /** The characters event */
  virtual void characters (const XMLCh *const chars, const unsigned int len);

  /** ErrorHandler Interface */
  /*@{*/
  /** An XML error occured */
  virtual void error(const SAXParseException& e){
    print_sax_exception("Error", e);
    _error_occurred = true;
  }
  /** A fatal XML error occured */
  virtual void fatalError(const SAXParseException& e){
    print_sax_exception("Fatal Error", e);
    _error_occurred = true;
  }
  /** An XML warning should be issued */
  virtual void warning(const SAXParseException& e){
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
   * converted to lower case, if \c _translate_iso_chars is \c true, german
   * umlaut characters are converted to their isomorphix counterpart (ae, ue,
   * ss, etc.)
   */
  string
  surface_string(const XMLCh *chars, const unsigned int len) const;

  /** The list of all input items produced during processing */
  std::list<class tInputItem *> &items() { return _items; }

  /** Try to map \a id to an already produced input item.
   * \return NULL if no item could be found, the pointer to the item otherwise.
   */
  tInputItem *get_item(const std::string &id) const {
    tItemMap::const_iterator it = _item_table.find(id.c_str());
    if (it == _item_table.end()) 
      return NULL;
    else
      return it->second;
  }

  /** Add \a new_item to the list of already produced input items */
  void add_item(class tInputItem *new_item);

  void setDocumentLocator(const Locator* const locator) { 
    _loc = locator;
  }

private:
  /** The copy constructor is disallowed */
  PICHandler(const PICHandler &x) {}

  /** Print a SAX exception in a convenient form */
  void print_sax_exception(const char * errtype, const SAXParseException& e);

  /** The state factory that produces the appropriate state for each XML tag */
  class pic::pic_state_factory *_state_factory;

  const Locator *_loc;

  XMLCh *_errbuf;

  const XMLCh * errmsg(std::string msg) {
    if (_errbuf != NULL) XMLString::release(&_errbuf) ;
    _errbuf = XMLString::transcode(msg.c_str());
    return _errbuf;
  }

  /** The embedding level of erroneous subunits to skip */
  int _err;

  /** A flag to register that an error occured during processing */
  bool _error_occurred;
  
  /** The current stack of XML states (nodes) */
  std::stack<class pic::pic_base_state *> _state_stack;

  /** A flag to indicate that all strings should be lower case (or not) */
  bool _downcase_strings;

  /** If \c true, translate all german umlaut characters to isomorphix (ae, ue,
   *  etc.)
   */
  bool _translate_iso_chars;

  struct string_hash
  {
    inline size_t operator()(const std::string &key) const
    {
      return ::bjhash((const ub1 *) key.data(), key.size(), 0);
    }
  };
  
  typedef hash_map< std::string, class tInputItem *, string_hash > tItemMap;
  tItemMap _item_table;
  std::list<class tInputItem *> _items;
};


#endif

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

XERCES_CPP_NAMESPACE_USE

/** Namespace for pet input chart xml parser */
namespace pic {
  class pic_base_state;
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
  PICHandler();
  /** Destructor */
  ~PICHandler();
  
  /** Start XML element with tag name \a tag and attribute list \a attrs */
  virtual void startElement(const XMLCh* const tag, AttributeList& attrs);
  /** End XML element with tag name \a tag */
  virtual void endElement(const XMLCh* const);
  /** The characters event */
  virtual void characters (const XMLCh *const chars, const unsigned int len);

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
  
  /** Did an error occur during processing? */
  bool error() const { return _error_occurred; }

  /** Should all strings be made lowercase? */
  bool downcase_strings() const { return _downcase_strings; }

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

private:
  /** The copy constructor is disallowed */
  PICHandler(const PICHandler &x) {}

  /** The current state (XML node) */
  class pic::pic_base_state *_state;
  
  /** The embedding level of erroneous subunits to skip */
  int _err;

  /** A flag to register that an error occured during processing */
  bool _error_occurred;
  
  /** The current stack of XML states (nodes) */
  std::stack<class pic::pic_base_state *> _state_stack;

  /** A flag to indicate that all strings should be lower case (or not) */
  bool _downcase_strings;

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

/* -*- Mode: C++ -*- */
/** \file pic-states.h
 * The superclass of all states used to parse the pic.dtd . This file can also
 * be used as a template for other SAX parsers.
 */

#ifndef _PIC_STATE_H
#define _PIC_STATE_H

#include "fs.h"
#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/util/XMLString.hpp>
#include <iostream>
#include <cstdio>
#include "xmlparser.h"
#include "pic-handler.h"
#include "item.h"

namespace pic {

class error {
  const char *_msg;
  const XMLCh *_arg;
  bool _arg_is_char_ptr;

public:
  error(const char *message, const XMLCh *arg)
    : _msg(message), _arg(arg), _arg_is_char_ptr(false)  {}
  error(const char *message, string arg) 
    : _msg(message), _arg((const XMLCh *) arg.c_str())
    , _arg_is_char_ptr(true)  {}

  void print(FILE *f);
};

XERCES_CPP_NAMESPACE_USE
using namespace std;

/** Double Dispatch Functions.
 * These two functions have to be implemented in every subclass of
 * pic_base_state in the same way. They only exist to implement the double
 * dispatch. Use this macro for clarity and to avoid typos.
 */
#define STATE_DOUBLE_DISPATCH \
  virtual void changeTo(pic_base_state* target, AttributeList& attr) \
    { target->enterState(this, attr); } \
  virtual void changeFrom(pic_base_state* curr) { curr->leaveState(this); }

/**
 * The base class to parse PetInputChart XML files.
 *
 * This solution is based on a double dispatch strategy. The changeTo method,
 * which has to be implemented in all subclasses (using the same simple code
 * everywhere), is selected based on the source state, while the enterState
 * method is selected based on the target state and has to be implemented only
 * for existing state transitions.
 * 
 * This implementation requires two virtual function calls per state
 * transition, but keeps the code compact.
 */
class pic_base_state {
public:
  pic_base_state(PICHandler *handler) : _reader(handler) {}

  /** @name Double Dispatch Functions
   * The next two functions have to be implemented in every subclass in the
   * same way. They only exist to implement the double dispatch. Use the macro
   * \c STATE_DOUBLE_DISPATCH for clarity and to avoid typos.
   */
  /*@{*/  
  /* Change into a new state. This function is called in the startElement
   * method of the document handler and dispatches on the basis of the source
   * state.
   */
  virtual void changeTo(pic_base_state* target, AttributeList& attr) = 0;
  /* { target->enterState(this, attr); } */

  /* Change back into a previous state. This function is called in the
   * endElement method of the document handler and dispatches on the basis of
   * the current state.
   */  
  virtual void changeFrom(pic_base_state* curr) = 0;
  /* { curr->leaveState(this); } */
  /*@}*/
  
  /** @name Enter State
   * Enter a state from another state.
   *
   * There is one such method for each state pair, eventually implemented in
   * this superclass. The transport of information is handled between the two
   * states, and the object that calls this method is always the new state.
   * The default implementation rejects the transition from one state into 
   * another, because the transition table is sparse. This is just a safety net
   * that might be removed if everything works OK.
   *
   * \param state The current state.
   * \param attr  The list of XML attributes of the node associated with the
   *              new state 
   */
  /*@{*/
  virtual void enterState(class pic_base_state* state, AttributeList& attr);
  virtual void enterState(class TOP_state* state, AttributeList& attr);
  virtual void enterState(class pet_input_chart* state, AttributeList& attr);
  virtual void enterState(class w* state, AttributeList& attr);
  virtual void enterState(class surface* state, AttributeList& attr);
  virtual void enterState(class path* state, AttributeList& attr);
  virtual void enterState(class typeinfo* state, AttributeList& attr);
  virtual void enterState(class fsmod* state, AttributeList& attr);
  virtual void enterState(class pos* state, AttributeList& attr);
  virtual void enterState(class ne* state, AttributeList& attr);
  virtual void enterState(class ref* state, AttributeList& attr);
  /*@}*/

  /** @name Leave State
   * Leave a state and return to the previous state.
   *
   * There is one such method for each state pair, eventually implemented in
   * this superclass. The transport of information is handled between the two
   * states, and the object that calls this method is always the current state,
   * while the \a previous is the the state coming from the state stack, to
   * which processing will return now.  The default implementation is a no-op.
   *
   * \param previous The old state to change back to.
   */
  /*@{*/
  virtual void leaveState(class pic_base_state* previous){}
  virtual void leaveState(class TOP* previous){}
  virtual void leaveState(class pet_input_chart* previous){}
  virtual void leaveState(class w* previous){}
  virtual void leaveState(class surface* previous){}
  virtual void leaveState(class path* previous){}
  virtual void leaveState(class typeinfo* previous){}
  virtual void leaveState(class fsmod* previous){}
  virtual void leaveState(class pos* previous){}
  virtual void leaveState(class ne* previous){}
  virtual void leaveState(class ref* previous){}
  /*@}*/

  /** the characters event */
  virtual void
  characters(const XMLCh *const chars, const unsigned int length) { }

  /** Diagnostic printing */
  virtual void print(std::ostream &out) const = 0 ;

protected:
  /** The XML SAX parser handler this state belongs to */
  PICHandler *_reader;
  
};

/** virtual top state not representing a token in the XML file. The parsing of
 *  the XML file starts with this state as current state.
 */
class TOP_state : public pic_base_state {
public:
  TOP_state(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~TOP_state() {}

  STATE_DOUBLE_DISPATCH

  /* The top state can not be entered or left. It is created at the beginning
     of the XML parser and the topmost element on the stack. This state has no
     valid events because it does not correspond to a XML tag. */

  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << "top state"; }
};

/** pic state representing the pet_input_chart token: the top level token */
class pet_input_chart : public pic_base_state {
public:
  pet_input_chart(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~pet_input_chart() {}

  STATE_DOUBLE_DISPATCH

  /**
   * TOP_state --> pet_input_chart state.
   */
  virtual void
  enterState(TOP_state* state, AttributeList& attr) { }

  /**
   * TOP state <-- pet_input_chart state.
   */
  //virtual void
  //leaveState(TOP_state* previous) { }

  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << "pic state"; }

private:
};

/** pic state representing the w token: a "word" */
class w : public pic_base_state {
public:
  w(PICHandler *picreader) : pic_base_state(picreader), _items(0) {} ;
  virtual ~w() {}

  STATE_DOUBLE_DISPATCH
  
  /**
   * pet_input_chart state --> w state.
   * Get the basic attributes of the w node.
   */
  virtual void
  enterState(pet_input_chart* state, AttributeList& attr) {
    _id = req_string_attr(attr, "id");
    _cstart = req_int_attr(attr, "cstart");
    _cend = req_int_attr(attr, "cend");
    _constant = req_bool_attr(attr, "constant");
    _prio = opt_double_attr(attr, "prio", 0.0);
  }
  /**
   * pet_input_start state <-- w state.
   * Add the items produced in this w node to the list of all input items.
   */
  virtual void
  leaveState(pet_input_chart* previous) {
    // If no add_item has been called and the _item_list is empty, we create a
    // WORD_TOKEN_CLASS input item that has to be analyzed internally
    if (_items == 0) {
      tInputItem *new_item 
        = new tInputItem(_id, _cstart, _cend, _surface, ""
                         , (_paths.empty() ? tPaths() : tPaths(_paths))
                         , _constant ? SKIP_TOKEN_CLASS : WORD_TOKEN_CLASS);
      new_item->set_in_postags(_pos);
      _reader->add_item(new_item);
    }
    // previous->splice(_items);
  }

  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << "w state"; }

  /** Add path number \a pathnum to our valid paths */
  void add_path(int pathnum) { _paths.push_back(pathnum); }

  /** Add POS \a tag with weight \a prio to the current list */
  void add_pos(string tag, double prio) {
    _pos.add(tag, prio);
  }

  /** Create a new unstructured tInputItem */
  void add_item(string id, bool base, double prio, string stem
                , list<int> &infls, ::modlist &mods) {
    // in principle, we had to check that this w node is not "constant". Those
    // nodes should not contain typeinfos
    int tokenclass = STEM_TOKEN_CLASS;
    if (! base) {
      // The "stem" is not a stem but a type name. Look up its code.
      if ((tokenclass = lookup_type(stem.c_str())) == T_BOTTOM)
        throw(error("Unknown Type", stem));
    }
    tInputItem *new_item 
      = new tInputItem(id, _cstart, _cend, _surface, stem
                       , (_paths.empty() ? tPaths() : tPaths(_paths))
                       , tokenclass, mods);
    new_item->score(prio);
    new_item->set_inflrs(infls);
    new_item->set_in_postags(_pos);
    //_items.push_back(new_item);
    _items++;
    _reader->add_item(new_item);
  }

  /** Store the surface string */
  void set_surface(string &s) { _surface = s; }

private:
  list<int> _paths;

  string _id, _surface;
  int _cstart, _cend;
  double _prio;
  bool _constant;
  postags _pos;

  int _items;
  // list< tInputItem * > _items;
};

/** pic state representing the ne token: a named entity (consisting of w
 *  tokens)
 */
class ne : public pic_base_state {
public:
  ne(PICHandler *picreader) : pic_base_state(picreader), _items(0) {} ;
  virtual ~ne() {}

  STATE_DOUBLE_DISPATCH
  
  /** pet_input_chart state --> ne state. */
  virtual void
  enterState(pet_input_chart* state, AttributeList& attr) {
    _id = req_string_attr(attr, "id");
    _prio = opt_double_attr(attr, "prio", 0.0);
    // Remember enclosing node to resolve the id references to the base input
    // items 
    //_pic_state = state;
  }

  /** pet_input_start state <-- ne state. */
  virtual void
  leaveState(pet_input_chart* previous) {
    if (_items == 0) throw(error("No valid typeinfo in ne tag", ""));
    // Add the new items to the enclosing state
    // previous->splice(_items);
  }

  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << "ne state"; }

  /** Add POS \a tag with weight \a prio to the current list */
  void add_pos(string tag, double prio) {
    _pos.add(tag, prio);
  }
  
  /** Append the next given daughter to the daughters list */
  void append_dtr(const string &id) { 
    //tInputItem *item = _pic_state->get_item(id);
    tInputItem *item = _reader->get_item(id);
    if (item == NULL) throw( error("unknown item" , id.c_str()) );
    _dtrs.push_back(item);
  }

  /** Add a structured input item to the list of input items */
  void add_item(string id, bool base, double prio, string stem
                , list<int> &infls, ::modlist &mods) {
    // OK we have to have a new tInputItem constructor here that allows to
    // create structured items. This constructor must take care of start and
    // end position.
    // The rest is quite the same. Again the question, which id counts as the
    // "right one"
    int tokenclass = STEM_TOKEN_CLASS;
    if (! base) {
      // The "stem" is not a stem but a type name. Look up its code.
      if ((tokenclass = lookup_type(stem.c_str())) == T_BOTTOM)
        throw(error("Unknown Type", stem));
    }

    tInputItem *new_item = new tInputItem(id, _dtrs, stem, tokenclass, mods);
    new_item->score(prio);
    new_item->set_inflrs(infls);
    new_item->set_in_postags(_pos);
    // _items.push_back(new_item);
    _items++;
    _reader->add_item(new_item);
    _dtrs.clear();
  }

private:
  string _id;
  double _prio;
  postags _pos;
  tPaths _paths;

  int _items;
  //pet_input_chart* _pic_state;
  list< tInputItem * > _dtrs; // , _items;
};

/** pic state representing the path token */
class path : public pic_base_state {
public:
  path(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~path() {}
  
  STATE_DOUBLE_DISPATCH
  
  /** w state --> path state: Add the new path to the enclosing w item */
  virtual void enterState(w* state, AttributeList& attr) {
    state->add_path(req_int_attr(attr, "num")); 
  }

  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << "path state"; }

private:

};

/** pic state representing the typeinfo token */
class typeinfo : public pic_base_state {
private:
  
  inline void get_attributes(AttributeList& attr) {
    _id = req_string_attr(attr, "id");
    _base = req_bool_attr(attr, "baseform");
    _prio = opt_double_attr(attr, "prio", 0.0);
  }

public:
  typeinfo(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~typeinfo() {}
  
  STATE_DOUBLE_DISPATCH

  /** @name w state <--> typeinfo state. */
  /*@{*/  
  /** Get base attributes*/
  virtual void
  enterState(w* state, AttributeList& attr) { get_attributes(attr); }

  /** Build a new item from this typeinfo */
  virtual void
  leaveState(w* prev) {
    prev->add_item(_id, _base, _prio, _stem, _infls, _mods);
  }
  /*@}*/

  /** @name ne state <--> typeinfo state. */
  /*@{*/  
  /** Get base attributes*/
  virtual void
  enterState(ne* state, AttributeList& attr) { get_attributes(attr); }

  /** Build a new item from this typeinfo */
  virtual void
  leaveState(ne* prev) {
    prev->add_item(_id, _base, _prio, _stem, _infls, _mods);
  }
  /*@}*/

  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << "typeinfo state"; }

  /** set the stem for this typeinfo */
  void set_stem(string str){
    _stem = str;
  }

  /** Append the inflection rule \a name to the current list. */
  void append_infl(string name){
    type_t infl_rule_type;
    if((infl_rule_type = lookup_type(name.c_str())) == T_BOTTOM)
      throw(error("unknown type", name));
    _infls.push_back(infl_rule_type);
  }

  /** Add the feature structure modification with \a path and \a val to the
   *  current list.
   */
  void add_fsmod(string path, string val) {
    type_t value = lookup_type(val.c_str());
    if (value == T_BOTTOM) throw(error("unknown type", val));
    _mods.push_back(pair<string, type_t>(path, value));
  }

private:

  string _id, _stem;
  list<int> _infls;
  ::modlist _mods;
  bool _base;
  double _prio;
};


/** pic state representing the infl token (an inflection rule) */
class infl : public pic_base_state {
public:
  infl(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~infl() {}

  STATE_DOUBLE_DISPATCH

  /** typeinfo state --> infl state: Add a new inflection rule to the
   * enclosing node.
   */
  virtual void enterState(typeinfo* state, AttributeList& attr) {
    state->append_infl(req_string_attr(attr, "name"));
  }


  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << "infl state"; }

};

/** pic state representing the fsmod token (a feature structure modification) 
 */
class fsmod : public pic_base_state {
public:
  fsmod(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~fsmod() {}
 
  STATE_DOUBLE_DISPATCH
  
  /** typeinfostate --> fsmod state: Add a new feature structure modification
   *  to the enclosing node.
   */
  virtual void enterState(typeinfo* state, AttributeList& attr) {
    state->add_fsmod( req_string_attr(attr, "path")
                      , req_string_attr(attr, "value"));
  }

  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << "fsmod state"; }

};

/** pic state representing the pos token (a part of speech with score) */
class pos : public pic_base_state {
public:
  pos(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~pos() {}
  
  STATE_DOUBLE_DISPATCH
  
  /** w state --> pos state. */
  virtual void enterState(w* state, AttributeList& attr) {
    state->add_pos(req_string_attr(attr, "tag")
                   , req_double_attr(attr, "prio"));
  }
  
  /** ne state --> pos state. */
  virtual void enterState(ne* state, AttributeList& attr) {
    state->add_pos(req_string_attr(attr, "tag")
                   , req_double_attr(attr, "prio"));
  }
  
  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << "pos state"; }

};

/** pic state representing a surface token (a string) */
class surface : public pic_base_state {
public:
  surface(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~surface() {}

  STATE_DOUBLE_DISPATCH

  /** @name w state <--> surface state. */
  /*@{*/
  /** A do nothing */
  virtual void enterState(w* state, AttributeList& attr) {} 

  /** set the surface string of the w node */
  virtual void leaveState(w* prev) { prev->set_surface(_surface); }
  /*@}*/

  /** @name typeinfo state <--> surface state. */
  /*@{*/
  /** A do nothing */
  virtual void enterState(typeinfo* state, AttributeList& attr) {} 

  /** set the stem string of the typedef node */
  virtual void leaveState(typeinfo* prev) { prev->set_stem(_surface); }
  /*@}*/

  /** the characters event */
  virtual void
  characters(const XMLCh *const chars, const unsigned int length) {
    XMLCh *res = XMLString::replicate(chars);
    XMLString::trim(res);   // strip blanks
    if (_reader->downcase_strings()) { XMLString::lowerCase(res); }
    // transfer to unicode UTF-8 string
    _surface = XMLCh2UTF8(res) ;
    XMLString::release(&res);
  }

  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << "surface state"; }

private:
  string _surface;
};

/** pic state representing the ref xml token (reference to a w or ne token) */
class ref : public pic_base_state {
public:
  ref(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~ref() {}
  
  STATE_DOUBLE_DISPATCH

  /** ne state --> ref state: Add the next daughter to the enclosing ne node */
  virtual void enterState(ne* state, AttributeList& attr) {
    state->append_dtr(req_string_attr(attr, "dtr"));
  }

  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << "ref state"; }

};

}

/** State printing for debugging */
std::ostream &
operator <<(std::ostream &out, const pic::pic_base_state &s);

#endif
#if 0
class : public pic_base_state {
public:
  virtual ~() {}

  STATE_DOUBLE_DISPATCH
  
  /** state <--> state. */
  /*@{*/  
  /** */
  virtual void enterState(* state, AttributeList& attr) { }

  /** */
  virtual void leaveState(* prev) {}
  /*@}*/

  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << " state"; }

private:

};

class : public pic_base_state {
public:
  virtual ~() {}

  STATE_DOUBLE_DISPATCH
  
  /** state <--> state. */
  /*@{*/  
  /** */
  virtual void enterState(* state, AttributeList& attr) { }
  /*@}*/

  /** Diagnostic printing */
  virtual void print(std::ostream &out) const { out << " state"; }

};
#endif

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
#include <xercesc/util/XMLChar.hpp>
#include <iostream>
#include <cstdio>
#include "hashing.h"
#include "hash.h"
#include "xmlparser.h"
#include "pic-handler.h"
#include "item.h"

namespace pic {

XERCES_CPP_NAMESPACE_USE
using namespace std;

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
   * \c STATE_COMMON_CODE for clarity and to avoid typos.
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
  
  /** Virtual clone function that mimics a virtual copy constructor */
  virtual pic_base_state *clone() = 0;

  /** Return the tag name this state corresponds to */
  virtual const XMLCh * const tag() const = 0;

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
  virtual void enterState(class stem* state, AttributeList& attr);
  virtual void enterState(class infl* state, AttributeList& attr);
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
  virtual void leaveState(class TOP_state* previous){}
  virtual void leaveState(class pet_input_chart* previous){}
  virtual void leaveState(class w* previous){}
  virtual void leaveState(class surface* previous){}
  virtual void leaveState(class path* previous){}
  virtual void leaveState(class typeinfo* previous){}
  virtual void leaveState(class stem* previous){}
  virtual void leaveState(class infl* previous){}
  virtual void leaveState(class fsmod* previous){}
  virtual void leaveState(class pos* previous){}
  virtual void leaveState(class ne* previous){}
  virtual void leaveState(class ref* previous){}
  /*@}*/

  /** the characters event */
  virtual void
  characters(const XMLCh *const chars, const unsigned int length) { }

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
string req_string_attr(AttributeList &attr, char *aname);
string opt_string_attr(AttributeList &attr, char *aname);
/*@}*/

protected:
  /** The XML SAX parser handler this state belongs to */
  PICHandler *_reader;
  
};


/** Code common to all classes that may not be put into the base class
 * 
 * Double Dispatch Functions:
 * These two functions have to be implemented in every subclass of
 * pic_base_state in the same way. They only exist to implement the double
 * dispatch. Use this macro for clarity and to avoid typos.
 *
 * Naming Function:
 * This function returns the tag name that corresponds to the state. Because it
 * accesses the static tag name in every class, it can not be put into the base
 * class.
 *
 * Virtual Clone Function:
 * Needed for the state factory to be able to construct the right state on
 * demand.
 */
#define STATE_COMMON_CODE(CLASSNAME) \
private: \
  static const XMLCh tagname[]; \
public: \
  virtual void changeTo(pic_base_state* target, AttributeList& attr) \
    { target->enterState(this, attr); } \
  virtual void changeFrom(pic_base_state* curr) { curr->leaveState(this); } \
  virtual const XMLCh * const tag() const { return tagname ; } \
  virtual CLASSNAME *clone() { return new CLASSNAME(*this) ; }


/** virtual top state not representing a token in the XML file. The parsing of
 *  the XML file starts with this state as current state.
 */
class TOP_state : public pic_base_state {
  STATE_COMMON_CODE(TOP_state)
public:
  TOP_state(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~TOP_state() {}


  /* The top state can not be entered or left. It is created at the beginning
     of the XML parser and the topmost element on the stack. This state has no
     valid events because it does not correspond to a XML tag. */

};

/** pic state representing the pet_input_chart token: the top level token */
class pet_input_chart : public pic_base_state {
  STATE_COMMON_CODE(pet_input_chart)
public:
  pet_input_chart(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~pet_input_chart() {}

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

};

/** pic state representing the w token: a "word" */
class w : public pic_base_state {
  STATE_COMMON_CODE(w)
public:
  w(PICHandler *picreader) : pic_base_state(picreader), _items(0) {} ;
  virtual ~w() {}

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
        throw(Error((string) "unknown type in w tag '" + stem + "'"));
    }
    tInputItem *new_item 
      = new tInputItem(id, _cstart, _cend, _surface, stem
                       , (_paths.empty() ? tPaths() : tPaths(_paths))
                       , tokenclass, mods);
    new_item->score(prio);
    new_item->set_inflrs(infls);
    new_item->set_in_postags(_pos);
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
  STATE_COMMON_CODE(ne)
public:
  ne(PICHandler *picreader) : pic_base_state(picreader), _items(0) {} ;
  virtual ~ne() {}
  
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
    if (_items == 0) throw(Error("No valid typeinfo in ne tag"));
    // Add the new items to the enclosing state
    // previous->splice(_items);
  }

  /** Add POS \a tag with weight \a prio to the current list */
  void add_pos(string tag, double prio) {
    _pos.add(tag, prio);
  }
  
  /** Append the next given daughter to the daughters list */
  void append_dtr(const string &id) { 
    //tInputItem *item = _pic_state->get_item(id);
    tInputItem *item = _reader->get_item(id);
    if (item == NULL) throw( Error((string) "unknown item '" + id + "'") );
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
        throw(Error((string) "unknown type in ne tag '" + stem + "'"));
    }

    tInputItem *new_item = new tInputItem(id, _dtrs, stem, tokenclass, mods);
    new_item->score(prio);
    new_item->set_inflrs(infls);
    new_item->set_in_postags(_pos);
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
  STATE_COMMON_CODE(path)
public:
  path(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~path() {}
  
  /** w state --> path state: Add the new path to the enclosing w item */
  virtual void enterState(w* state, AttributeList& attr) {
    state->add_path(req_int_attr(attr, "num")); 
  }

};

/** pic state representing the typeinfo token */
class typeinfo : public pic_base_state {
  STATE_COMMON_CODE(typeinfo)
private:
  inline void get_attributes(AttributeList& attr) {
    _id = req_string_attr(attr, "id");
    // baseform == yes means: this is a stem (base form), not a type name
    _base = req_bool_attr(attr, "baseform");
    _prio = opt_double_attr(attr, "prio", 0.0);
  }

public:
  typeinfo(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~typeinfo() {}
  
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

  /** set the stem for this typeinfo */
  void set_stem(string str){
    _stem = str;
  }

  /** Append the inflection rule \a name to the current list. */
  void append_infl(string name){
    type_t infl_rule_type = lookup_type(name.c_str());
    if(infl_rule_type == T_BOTTOM)
      throw(Error((string) "unknown infl type '" + name + "'"));
    _infls.push_back(infl_rule_type);
  }

  /** Add the feature structure modification with \a path and \a val to the
   *  current list.
   */
  void add_fsmod(string path, string val) {
    _mods.push_back(pair<string, type_t>(path, lookup_symbol(val.c_str())));
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
  STATE_COMMON_CODE(infl)
public:
  infl(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~infl() {}

  /** typeinfo state --> infl state: Add a new inflection rule to the
   * enclosing node.
   */
  virtual void enterState(typeinfo* state, AttributeList& attr) {
    state->append_infl(req_string_attr(attr, "name"));
  }

};

/** pic state representing the fsmod token (a feature structure modification) 
 */
class fsmod : public pic_base_state {
  STATE_COMMON_CODE(fsmod)
public:
  fsmod(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~fsmod() {}

  /** typeinfostate --> fsmod state: Add a new feature structure modification
   *  to the enclosing node.
   */
  virtual void enterState(typeinfo* state, AttributeList& attr) {
    state->add_fsmod( req_string_attr(attr, "path")
                      , req_string_attr(attr, "value"));
  }

};

/** pic state representing the pos token (a part of speech with score) */
class pos : public pic_base_state {
  STATE_COMMON_CODE(pos)
public:
  pos(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~pos() {}
  
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
  
};

/** pic state representing a surface token (a string) */
class surface : public pic_base_state {
  STATE_COMMON_CODE(surface)
private:
  string _surface;

public:
  surface(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~surface() {}

  /** @name w state <--> surface state. */
  /*@{*/
  /** A do nothing */
  virtual void enterState(w* state, AttributeList& attr) {} 

  /** set the surface string of the w node */
  virtual void leaveState(w* prev) { prev->set_surface(_surface); }
  /*@}*/

  /** the characters event */
  virtual void
  characters(const XMLCh *const chars, const unsigned int length) {
    // translate the chars into a surface string feasible for the system
    _surface += _reader->surface_string(chars, length) ;
  }

};

/** pic state representing a stem token (a string) */
class stem : public pic_base_state {
  STATE_COMMON_CODE(stem)
private:
  string _surface;

public:
  stem(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~stem() {}

  /** @name typeinfo state <--> stem state. */
  /*@{*/
  /** A do nothing */
  virtual void enterState(typeinfo* state, AttributeList& attr) {} 

  /** set the stem string of the typedef node */
  virtual void leaveState(typeinfo* prev) { prev->set_stem(_surface); }
  /*@}*/

  /** the characters event */
  virtual void
  characters(const XMLCh *const chars, const unsigned int length) {
    // translate the chars into a surface string feasible for the system
    _surface += _reader->surface_string(chars, length);
  }

};

/** pic state representing the ref xml token (reference to a w or ne token) */
class ref : public pic_base_state {
  STATE_COMMON_CODE(ref)
public:
  ref(PICHandler *picreader) : pic_base_state(picreader) {} ;
  virtual ~ref() {}
  
  /** ne state --> ref state: Add the next daughter to the enclosing ne node */
  virtual void enterState(ne* state, AttributeList& attr) {
    state->append_dtr(req_string_attr(attr, "dtr"));
  }

};

class pic_state_factory {
  struct hash_xmlstring {
    unsigned int operator()(const XMLCh *s) const {
      return bjhash((const ub1 *) s
                    , XMLString::stringLen(s) * sizeof(XMLCh), 0);
    }
  };

  struct xmlstring_equal {
    bool operator()(const XMLCh *s1, const XMLCh *s2) {
      return (XMLString::compareString(s1, s2) == 0);
    }
  };

  typedef hash_map< const XMLCh *, pic_base_state *, struct hash_xmlstring
                    , struct xmlstring_equal > xs_hash_map;

  xs_hash_map _state_table;

  void register_state(pic_base_state *state) {
    _state_table[state->tag()] = state;
  }

public:
  pic_state_factory(class PICHandler *handler) {
    register_state(new pet_input_chart(handler));
    register_state(new w(handler));
    register_state(new surface(handler));
    register_state(new path(handler));
    register_state(new typeinfo(handler));
    register_state(new stem(handler));
    register_state(new infl(handler));
    register_state(new fsmod(handler));
    register_state(new pos(handler));
    register_state(new ne(handler));
    register_state(new ref(handler));
  }

  pic_base_state *get_state(const XMLCh *tag) {
    xs_hash_map::iterator it = _state_table.find(tag);
    return (it == _state_table.end()) ? NULL : (it->second)->clone();
  }
};

}

/** State printing for debugging */
std::ostream &
operator <<(std::ostream &out, const pic::pic_base_state &s);

#endif
#if 0
class STATE_CLASS : public pic_base_state {
public:
  virtual ~STATE_CLASS() {}

  STATE_COMMON_CODE(STATE_CLASS)
  
  /** state <--> state. */
  /*@{*/  
  /** */
  virtual void enterState(* state, AttributeList& attr) { }

  /** */
  virtual void leaveState(* prev) {}
  /*@}*/

private:
  static const XMLCh tagname[] = {
    chLatin_t, chLatin_a, chLatin_g, chNull
  };
};

class STATE_CLASS : public pic_base_state {
  static const XMLCh tagname[] = {
    chLatin_t, chLatin_a, chLatin_g, chNull
  };

public:
  virtual ~STATE_CLASS() {}

  STATE_COMMON_CODE(STATE_CLASS)
  
  /** state <--> state. */
  /*@{*/  
  /** */
  virtual void enterState(* state, AttributeList& attr) { }
  /*@}*/

};
#endif

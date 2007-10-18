/* -*- Mode: C++ -*- */
/* This file is partially copied/adapted from pic-states.h for SAX
   parsing of XML MRS inputs. */

#ifndef _MRS_STATES_H_
#define _MRS_STATES_H_

#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLChar.hpp>
#include "xmlparser.h"
#include "mrs-handler.h"
#include <iostream>

namespace mrs {

class mrs_base_state {
public:
  mrs_base_state(MRSHandler *handler) : _reader(handler) {
  }
  
  virtual ~mrs_base_state() {
  }

  virtual void changeTo(mrs_base_state* target, AttributeList& attr) = 0;
  
  virtual void changeFrom(mrs_base_state* curr) = 0;

  virtual mrs_base_state *clone() = 0;
  
  virtual const XMLCh * const tag() const = 0;

  virtual void enterState(class mrs_base_state* state, AttributeList& attr);
  virtual void enterState(class TOP_state* state, AttributeList& attr);
  virtual void enterState(class mrs_list_state* state, AttributeList& attr);
  virtual void enterState(class mrs_state* state, AttributeList& attr);
  virtual void enterState(class ep_state* state, AttributeList& attr);
  virtual void enterState(class pred_state* state, AttributeList& attr);
  virtual void enterState(class spred_state* state, AttributeList& attr);
  virtual void enterState(class realpred_state* state, AttributeList& attr);
  virtual void enterState(class label_state* state, AttributeList& attr);
  virtual void enterState(class var_state* state, AttributeList& attr);
  virtual void enterState(class extrapair_state* state, AttributeList& attr);
  virtual void enterState(class path_state* state, AttributeList& attr);
  virtual void enterState(class value_state* state, AttributeList& attr);
  virtual void enterState(class fvpair_state* state, AttributeList& attr);
  virtual void enterState(class rargname_state* state, AttributeList& attr);
  virtual void enterState(class constant_state* state, AttributeList& attr);
  virtual void enterState(class hcons_state* state, AttributeList& attr);
  virtual void enterState(class hi_state* state, AttributeList& attr);
  virtual void enterState(class lo_state* state, AttributeList& attr);

  virtual void leaveState(class mrs_base_state* state) {}
  virtual void leaveState(class TOP_state* state) {}
  virtual void leaveState(class mrs_list_state* state) {}
  virtual void leaveState(class mrs_state* state) {}
  virtual void leaveState(class ep_state* state) {}
  virtual void leaveState(class pred_state* state) {}
  virtual void leaveState(class spred_state* state) {}
  virtual void leaveState(class realpred_state* state) {}
  virtual void leaveState(class label_state* state) {}
  virtual void leaveState(class var_state* state) {}
  virtual void leaveState(class extrapair_state* state) {}
  virtual void leaveState(class path_state* state) {}
  virtual void leaveState(class value_state* state) {}
  virtual void leaveState(class fvpair_state* state) {}
  virtual void leaveState(class rargname_state* state) {}
  virtual void leaveState(class constant_state* state) {}
  virtual void leaveState(class hcons_state* state) {}
  virtual void leaveState(class hi_state* state) {}
  virtual void leaveState(class lo_state* state) {}
  
  virtual void characters(const XMLCh* chars, const unsigned int length) { }
  
  /** @name Attribute Helper Functions
   * Helper Functions for different types of attributes with error handling.
   */
  /*@{*/
  bool req_bool_attr(AttributeList& attr, char *aname);
  bool opt_bool_attr(AttributeList& attr, char *aname, bool def);
  int req_int_attr(AttributeList& attr, char *aname);
  int opt_int_attr(AttributeList& attr, char *aname, int def);
  char req_char_attr(AttributeList& attr, char *aname);
  char opt_char_attr(AttributeList& attr, char *aname, char def);
  double req_double_attr(AttributeList& attr, char *aname);
  double opt_double_attr(AttributeList& attr, char *aname, double def);
  std::string req_string_attr(AttributeList &attr, char *aname);
  std::string opt_string_attr(AttributeList &attr, char *aname);
  /*@}*/
  
protected:
  /** The XML SAX parser handler this state belongs to */
  MRSHandler *_reader;
  
};


/** Code common to all classes that may not be put into the base class
 * 
 * Double Dispatch Functions:
 * These two functions have to be implemented in every subclass of
 * mrs_base_state in the same way. They only exist to implement the double
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
  virtual void changeTo(mrs_base_state* target, AttributeList& attr) \
    { target->enterState(this, attr); } \
  virtual void changeFrom(mrs_base_state* curr) { curr->leaveState(this); } \
  virtual const XMLCh * const tag() const { return tagname ; } \
  virtual CLASSNAME *clone() { return new CLASSNAME(*this) ; }


class TOP_state : public mrs_base_state {
  STATE_COMMON_CODE(TOP_state)
public:
  TOP_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~TOP_state() {}
};


class mrs_list_state : public mrs_base_state {
  STATE_COMMON_CODE(mrs_list_state)
public:
  mrs_list_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~mrs_list_state() {}

  /**
   * TOP_state --> mrs_list_state
   */
  virtual void enterState(class TOP_state* state, AttributeList& attr);
  
  /**
   * TOP_state <-- mrs_list_state
   */
  virtual void leaveState(class TOP_state *state);

};
  
class mrs_state : public mrs_base_state {
  STATE_COMMON_CODE(mrs_state)
public:
  mrs_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~mrs_state() {}

  /**
   * mrs_list_state --> mrs_state
   */
  virtual void enterState(class mrs_list_state* state, AttributeList& attr);

  /**
   * mrs_list_state <-- mrs_state
   */
  virtual void leaveState(class mrs_list_state* state);

  tPSOA* _mrs;
  
};

class ep_state : public mrs_base_state {
  STATE_COMMON_CODE(ep_state)
public:
  ep_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~ep_state() {}

  /** 
   * mrs_state --> ep_state
   */
  virtual void enterState(mrs_state* state, AttributeList& attr);

  /**
   * mrs_state <-- ep_state
   */
  virtual void leaveState(mrs_state* state);
  
  tRel* _rel;

};

class pred_state : public mrs_base_state {
  STATE_COMMON_CODE(pred_state)
public:
  pred_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~pred_state() {}
  
  /**
   * ep_state --> pred_state
   */
  virtual void enterState(class ep_state* state, AttributeList& attr);

  /**
   * ep_state <-- pred_state
   */
  virtual void leaveState(class ep_state* state);

  /** the characters event */
  virtual void characters(const XMLCh *const chars, const unsigned int length);

  std::string _predname; 
};

class spred_state : public mrs_base_state {
  STATE_COMMON_CODE(spred_state)
public:
  spred_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~spred_state() {}

  /**
   * ep_state --> spred_state
   */
  virtual void enterState(class ep_state* state, AttributeList& attr);

  /**
   * ep_state <-- spred_state
   */
  virtual void leaveState(class ep_state* state);

  /** the characters event */
  virtual void characters(const XMLCh *const chars, const unsigned length);
  
  std::string _predname;
};

// realpred is not yet supported, here only for later compatibility
class realpred_state : public mrs_base_state {
  STATE_COMMON_CODE(realpred_state)
public:
  realpred_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~realpred_state() {}

};

class label_state : public mrs_base_state {
  STATE_COMMON_CODE(label_state)
public:
  label_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~label_state() {}
  
  /**
   * mrs_state --> label_state
   */
  virtual void enterState(class mrs_state* state, AttributeList& attr);

  /**
   * mrs_state <-- label_state
   */
  virtual void leaveState(class mrs_state* state);

  /**
   * ep_state --> label_state
   */
  virtual void enterState(class ep_state* state, AttributeList& attr);

  /**
   * ep_state <-- label_state
   */
  virtual void leaveState(class ep_state* state);

  tVar* _label;
};

class extrapair_state : public mrs_base_state {
  STATE_COMMON_CODE(extrapair_state)
public:
  extrapair_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~extrapair_state() {}

  /**
   * label_state --> extrapair_state
   */
  virtual void enterState(class label_state* state, AttributeList& attr);

  /**
   * label_state <-- extrapair_state
   */
  virtual void leaveState(class label_state* state);

  /**
   * var_state --> extrapair_state
   */
  virtual void enterState(class var_state* state, AttributeList& attr);

  /**
   * var_state <-- extrapair_state
   */
  virtual void leaveState(class var_state* state);

  tVar* _var;
  std::string _path;
  std::string _value;
};

class path_state : public mrs_base_state {
  STATE_COMMON_CODE(path_state)
public:
  path_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~path_state() {}

  /**
   * extrapair_state --> path_state
   */
  virtual void enterState(class extrapair_state* state, AttributeList& attr);

  /**
   * extrapair_state <-- path_state
   */
  virtual void leaveState(class extrapair_state* state);

  /**
   * the characters event
   */
  virtual void characters(const XMLCh *const chars, const unsigned int length);
  
  tVar* _var;
  std::string _path;
};

class value_state : public mrs_base_state {
  STATE_COMMON_CODE(value_state)
public:
  value_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~value_state() {}

  /**
   * extrapair_state --> value_state
   */
  virtual void enterState(class extrapair_state* state, AttributeList& attr);

  /**
   * extrapair_state <-- value_state
   */
  virtual void leaveState(class extrapair_state* state);

  /**
   * the characters event
   */
  virtual void characters(const XMLCh *const chars, const unsigned int length);
  
  tVar* _var;
  std::string _value;
};

class var_state : public mrs_base_state {
  STATE_COMMON_CODE(var_state)
public:
  var_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~var_state() {}

  /**
   * mrs_state --> var_state
   */
  virtual void enterState(class mrs_state* state, AttributeList& attr);

  /**
   * mrs_state <-- var_state
   */
  virtual void leaveState(class mrs_state* state);

  /**
   * fvpair_state --> var_state
   */
  virtual void enterState(class fvpair_state* state, AttributeList& attr);

  /**
   * fvpair_state <-- var_state
   */
  virtual void leaveState(class fvpair_state* state);

  /**
   * hi_state --> var_state
   */
  virtual void enterState(class hi_state* state, AttributeList& attr);

  /**
   * hi_state <-- var_state
   */
  virtual void leaveState(class hi_state* state);

  /**
   * lo_state --> var_state
   */
  virtual void enterState(class lo_state* state, AttributeList& attr);

  /**
   * lo_state <-- var_state
   */
  virtual void leaveState(class lo_state* state);

  tVar* _var;
};

class fvpair_state : public mrs_base_state {
  STATE_COMMON_CODE(fvpair_state)
public:
  fvpair_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~fvpair_state() {}

  /**
   * ep_state --> fvpair_state
   */
  virtual void enterState(class ep_state* state, AttributeList& attr);

  /**
   * ep_state <-- fvpair_state
   */
  virtual void leaveState(class ep_state* state);

  tRel* _rel;
  std::string _feature;
  tValue* _value;
};

class rargname_state : public mrs_base_state {
  STATE_COMMON_CODE(rargname_state)
public:
  rargname_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~rargname_state() {}
  
  /**
   * fvpair_state --> rargname_state
   */
  virtual void enterState(class fvpair_state* state, AttributeList& attr);

  /**
   * fvpair_state <-- rargname_state
   */
  virtual void leaveState(class fvpair_state* state);

  /**
   * the characters event
   */
  virtual void characters(const XMLCh *const chars, const unsigned int length);

  std::string _feature;
};

class constant_state : public mrs_base_state {
  STATE_COMMON_CODE(constant_state)
public:
  constant_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~constant_state() {}
  
  /**
   * fvpair_state --> constant_state
   */
  virtual void enterState(class fvpair_state* state, AttributeList& attr);

  /**
   * fvpair_state <-- constant_state
   */
  virtual void leaveState(class fvpair_state* state);

  /**
   * the characters event
   */
  virtual void characters(const XMLCh *const chars, const unsigned int length);
  
  tRel* _rel; 
  std::string _value;
};

class hcons_state : public mrs_base_state {
  STATE_COMMON_CODE(hcons_state)
public:
  hcons_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~hcons_state() {}
  
  /**
   * mrs_state --> hcons_state
   */
  virtual void enterState(class mrs_state* state, AttributeList& attr);
  
  /**
   * mrs_state <-- hcons_state
   */
  virtual void leaveState(class mrs_state* state);

  tHCons* _hcons;
};

class hi_state : public mrs_base_state {
  STATE_COMMON_CODE(hi_state)
public:
  hi_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~hi_state() {}

  /**
   * hcons_state --> hi_state
   */
  virtual void enterState(class hcons_state* state, AttributeList& attr);

  /**
   * hcons_state <-- hi_state
   */
  virtual void leaveState(class hcons_state* state);

  tHCons* _hcons;
  tVar* _var;
};

class lo_state : public mrs_base_state {
  STATE_COMMON_CODE(lo_state)
public:
  lo_state(MRSHandler *mrsreader) : mrs_base_state(mrsreader) {}
  virtual ~lo_state() {}

  /**
   * hcons_state --> lo_state
   */
  virtual void enterState(class hcons_state* state, AttributeList& attr);

  /**
   * hcons_state <-- lo_state
   */
  virtual void leaveState(class hcons_state* state);

  tHCons* _hcons;
  tVar* _var;
};

class mrs_state_factory {
  struct hash_xmlstring {
    unsigned int operator()(const XMLCh*s) const {
      return bjhash((const ub1 *) s
                    , XERCES_CPP_NAMESPACE_QUALIFIER XMLString::stringLen(s) * sizeof(XMLCh), 0);
    }
  };

  struct xmlstring_equal {
    bool operator()(const XMLCh *s1, const XMLCh *s2) {
      return (XERCES_CPP_NAMESPACE_QUALIFIER XMLString::compareString(s1, s2) == 0);
    }
  };

  typedef hash_map<const XMLCh *, mrs_base_state *, struct hash_xmlstring
		   , struct xmlstring_equal > xs_hash_map;
  xs_hash_map _state_table;
  void register_state(mrs_base_state *state) {
    _state_table[state->tag()] = state;
  }

public:
  mrs_state_factory(class MRSHandler *handler) {
    register_state(new mrs_list_state(handler));
    register_state(new mrs_state(handler));
    register_state(new ep_state(handler));    
    register_state(new pred_state(handler));    
    register_state(new spred_state(handler));
    register_state(new realpred_state(handler));
    register_state(new label_state(handler));
    register_state(new var_state(handler));
    register_state(new extrapair_state(handler));
    register_state(new path_state(handler));
    register_state(new value_state(handler));
    register_state(new fvpair_state(handler));
    register_state(new rargname_state(handler));
    register_state(new constant_state(handler));
    register_state(new hcons_state(handler));
    register_state(new hi_state(handler));
    register_state(new lo_state(handler));
  }

  mrs_base_state *get_state(const XMLCh *tag) {
    xs_hash_map::iterator it = _state_table.find(tag);
    return (it == _state_table.end()) ? NULL : (it->second)->clone();
  }
};

} // namespace mrs

#endif

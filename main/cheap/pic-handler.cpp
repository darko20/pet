#include "pic-handler.h"
#include "pic-states.h"
#include "fs.h"
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/util/XMLChar.hpp>

extern FILE *ferr;

std::ostream &operator <<(std::ostream &out, const pic::pic_base_state *s) {
  s->print(out); return out;
}

std::ostream &operator <<(std::ostream &out, const pic::pic_base_state &s) {
  s.print(out); return out;
}

const XMLCh pet_input_chart_str[] = {
  chLatin_p, chLatin_e, chLatin_t, chDash,
  chLatin_i, chLatin_n, chLatin_p, chLatin_u, chLatin_t, chDash,
  chLatin_c, chLatin_h, chLatin_a, chLatin_r, chLatin_t, chNull
};

const XMLCh w_str[] = {
  chLatin_w, chNull
};

const XMLCh surface_str[] = {
  chLatin_s, chLatin_u, chLatin_r, chLatin_f, chLatin_a, chLatin_c, chLatin_e,
  chNull
};

const XMLCh path_str[] = {
  chLatin_p, chLatin_a, chLatin_t, chLatin_h, chNull
};

const XMLCh typeinfo_str[] = {
  chLatin_t, chLatin_y, chLatin_p, chLatin_e, 
  chLatin_i, chLatin_n, chLatin_f, chLatin_o,
  chNull
};

const XMLCh stem_str[] = {
  chLatin_s, chLatin_t, chLatin_e, chLatin_m, chNull
};

const XMLCh fsmod_str[] = {
  chLatin_f, chLatin_s, chLatin_m, chLatin_o, chLatin_d, chNull
};

const XMLCh pos_str[] = {
  chLatin_p, chLatin_o, chLatin_s, chNull
};

const XMLCh ne_str[] = {
  chLatin_n, chLatin_e, chNull
};

const XMLCh ref_str[] = {
  chLatin_r, chLatin_e, chLatin_f, chNull
};


// ---------------------------------------------------------------------------
//  SAXPrintHandlers: Overrides of the SAX ErrorHandler interface
// ---------------------------------------------------------------------------

PICHandler::PICHandler() {
  _state = new pic::TOP_state(this);
  _err = 0;
  _error_occurred = false;
  _downcase_strings = true;
}

PICHandler::~PICHandler() {
  while (! _state_stack.empty()) {
    // *errlog << *_state_stack.top() ;
    delete _state_stack.top();
    _state_stack.pop();
  }
  delete _state;  // delete the top state
}

// _fix_me implement this function better!
pic::pic_base_state *
getNewState(const XMLCh* const name, AttributeList& attribs, PICHandler *ph) {
  // Get the NewState function from the dispatch table
  if(XMLString::compareString(w_str, name) == 0)
    return new pic::w(ph);
  else if (XMLString::compareString(pet_input_chart_str, name) == 0)
    return new pic::pet_input_chart(ph);
  else if (XMLString::compareString(typeinfo_str, name) == 0)
    return new pic::typeinfo(ph);
  else if (XMLString::compareString(path_str, name) == 0)
    return new pic::path(ph);
  else if (XMLString::compareString(ref_str, name) == 0)
    return new pic::ref(ph);
  else if (XMLString::compareString(ne_str, name) == 0)
    return new pic::ne(ph);
  else if (XMLString::compareString(surface_str, name) == 0)
    return new pic::surface(ph);
  else if (XMLString::compareString(stem_str, name) == 0)
    return new pic::surface(ph);
  else if (XMLString::compareString(pos_str, name) == 0)
    return new pic::pos(ph);
  else if (XMLString::compareString(fsmod_str, name) == 0)
    return new pic::fsmod(ph);
  
  return NULL;
}

void 
PICHandler::startElement(const XMLCh* const name, AttributeList& attribs)
{
  // cerr << "I saw Startelement: "<< XMLCh2Latin(name) << endl;

  if (_err > 0) {
    // the enclosing state is in error, we postpone processing until we leave
    // the erroneous state
    _err++;
  } else {
    pic::pic_base_state *newState = getNewState(name, attribs, this);
    
    if (newState != NULL) {
      // eventually change from current state into new state on tag NAME  
      cerr << *_state << "<" << *newState;
      
      try {
        _state->changeTo(newState, attribs);
        _state_stack.push(_state);
        _state = newState;
      }
      catch(pic::error e) {
        // log error 
        e.print(ferr);
        delete newState;
        _error_occurred = true;
        _err++;
      }
      catch(::XMLAttributeError e) {
        // log error 
        e.print(ferr);
        delete newState;
        _error_occurred = true;
        _err++;
      }
    } else {
      // unknown tag error: This is a fatal XML error because all tags in the
      // DTD should be covered
      cerr << "Unknown Tag:" << XMLCh2Latin(name);
      _error_occurred = true;
    }
  }
}

void
PICHandler::endElement(const XMLCh* const name)
{
  //cerr << "I saw Endelement: "<< XMLCh2Latin(name) << endl;
  // if we are not currently skipping an error, return to a state on the stack
  if (_err == 0) {
    pic::pic_base_state *oldstate = _state;
    _state = _state_stack.top();
    _state_stack.pop();
    
    cerr << " " << *oldstate << ">" << *_state;
    // do anything necessary to finish the old state, eventually get results
    // from oldstate into new current state 
    try {
      _state->changeFrom(oldstate);
    }
    catch (pic::error e) {
      //  log error 
      e.print(ferr);
    }
    delete oldstate;
  } else {
    // processing of erroneous subunits is avoided
    _err--;
  }
}

void
PICHandler::characters(const XMLCh *const chars, const unsigned int len){
  _state->characters(chars, len);
}

void PICHandler::add_item(tInputItem *new_item) {
  const char *ext_id = new_item->external_id().c_str();
  if (_item_table.find(ext_id) != _item_table.end())
    throw(pic::error("Same ID used twice", ext_id));
  
  _item_table[ext_id] = new_item;
  _items.push_back(new_item);
}

/*
int call_sax_parser(int argc, char **argv) {
  xml_initialize();
  PICHandler ghandler;
  bool res = (parse_file(XStr(argv[1]).XMLForm()
                         , &gram_handler)
              && (! gram_handler.error()));
  xml_finalize();
  return res ? 0 : 1;
}
*/


namespace pic {

void error::print(FILE *f) {
  fprintf(f, "%s : %s\n", _msg
          , (_arg_is_char_ptr ? (char *) _arg : XMLCh2Latin(_arg)));
}

/** Reject state transition \a from --> \a to */
void reject(pic_base_state *to, pic_base_state *from) {
  ostringstream ostr;
  ostr << "Can not change from " << from << " into " << to << endl;
  throw(error(ostr.str().c_str(), (const char *) ""));
}

void pic_base_state::enterState(class pic_base_state* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(class TOP_state* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(class pet_input_chart* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(class w* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(class surface* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(class path* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(class typeinfo* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(class fsmod* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(class pos* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(class ne* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(class ref* s, AttributeList& attr)
  { reject(this, s); }

}

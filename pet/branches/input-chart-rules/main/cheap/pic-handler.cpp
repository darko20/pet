#include "pic-states.h"
#include "fs.h"
#include <xercesc/framework/MemBufFormatTarget.hpp>
#ifdef HAVE_ICU
#include "unicode.h"
#endif
#include <sstream>

using namespace std;
using namespace XERCES_CPP_NAMESPACE;
using XERCES_CPP_NAMESPACE_QUALIFIER AttributeList;

extern FILE *ferr;

namespace pic {

/******************************************************************************
 Attribute helper functions
 *****************************************************************************/

const XMLCh yes_str[] = { chLatin_y,  chLatin_e,  chLatin_s, chNull };

bool pic_base_state::req_bool_attr(AttributeList& attr, char *aname) {
  const XMLCh *str;
  if ((str = attr.getValue(aname)) == NULL)
    throw Error((string) "required attribute '" + aname + "' missing");
  return (XMLString::compareIString(str, yes_str) == 0);
}

bool pic_base_state::opt_bool_attr(AttributeList& attr, char *aname, bool def){
  const XMLCh *str;
  if ((str = attr.getValue(aname)) == NULL) return def;
  return (XMLString::compareIString(str, yes_str) == 0);
}

int pic_base_state::req_int_attr(AttributeList& attr, char *aname) {
  const XMLCh *val;
  if ((val = attr.getValue(aname)) == NULL)
    throw Error((string) "required attribute '" + aname + "' missing");
  const char *str = XMLCh2UTF8(val);
  char *end;
  int res = strtol(str, &end, 10);
  if ((*end != '\0') || (str == end))
    throw Error((string) "wrong int value '" + str + "' for attribute '"
                + aname + "'");
  return res;
}

int pic_base_state::opt_int_attr(AttributeList& attr, char *aname, int def) {
  const XMLCh *val;
  if ((val = attr.getValue(aname)) == NULL) return def;
  const char *str = XMLCh2UTF8(val);
  char *end;
  int res = strtol(str, &end, 10);
  if ((*end != '\0') || (str == end))
    throw Error((string) "wrong int value '" + str + "' for attribute '"
                + aname + "'");
  return res;
}

double pic_base_state::req_double_attr(AttributeList& attr, char *aname) {
  const XMLCh *val;
  if ((val = attr.getValue(aname)) == NULL) 
    throw Error((string) "required attribute '" + aname + "' missing");
  const char *str = XMLCh2UTF8(val);
  char *end;
  double res = strtod(str, &end);
  if ((*end != '\0') || (str == end))
    throw Error((string) "wrong double value '" + str + "' for attribute '"
                + aname + "'");
  return res;
}

double
pic_base_state::opt_double_attr(AttributeList& attr, char *aname, double def) {
  const XMLCh *val;
  if ((val = attr.getValue(aname)) == NULL) return def;
  const char *str = XMLCh2UTF8(val);
  char *end;
  double res = strtod(str, &end);
  if ((*end != '\0') || (str == end))
    throw Error((string) "wrong double value '" + str + "' for attribute '"
                + aname + "'");
  return res;
}

string pic_base_state::
req_string_attr(AttributeList &attr, char *aname) {
  const XMLCh *val = attr.getValue(aname);
  if (val == NULL)
    throw Error((string) "required attribute '" + aname + "' missing");
#ifdef HAVE_ICU
  // TODO: [bmw] is it not necessary to explicitly XMLString::release the
  // XMLCh* val ???
  return Conv->convert((UChar *)val, XMLString::stringLen(val));
#else
  return XMLCh2Latin(val);
#endif
}

string pic_base_state::
opt_string_attr(AttributeList &attr, char *aname) {
  const XMLCh *val = attr.getValue(aname);
  if (val == NULL) return "";
#ifdef HAVE_ICU
  return Conv->convert((UChar *)val, XMLString::stringLen(val));
#else
  return XMLCh2Latin(val);
#endif
}


std::ostream &operator <<(std::ostream &out, const pic::pic_base_state *s) {
  out << XMLCh2Latin(s->tag()); return out;
}

std::ostream &operator <<(std::ostream &out, const pic::pic_base_state &s) {
  out << XMLCh2Latin(s.tag()); return out;
}

/** Reject state transition \a from --> \a to */
void reject(pic_base_state *to, pic_base_state *from) {
  std::ostringstream ostr;
  ostr << "Can not change from " << from << " into " << to << endl;
  throw Error(ostr.str());
}

void pic_base_state::enterState(pic_base_state* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(TOP_state* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(pet_input_chart* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(w* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(surface* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(path* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(typeinfo* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(stem* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(infl* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(fsmod* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(pos* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(ne* s, AttributeList& attr)
  { reject(this, s); }
void pic_base_state::enterState(ref* s, AttributeList& attr)
  { reject(this, s); }

const XMLCh TOP_state::tagname[] = { chNull } ;

const XMLCh pet_input_chart::tagname[] = {
  chLatin_p, chLatin_e, chLatin_t, chDash,
  chLatin_i, chLatin_n, chLatin_p, chLatin_u, chLatin_t, chDash,
  chLatin_c, chLatin_h, chLatin_a, chLatin_r, chLatin_t, chNull
};

const XMLCh w::tagname[] = {
  chLatin_w, chNull
};

const XMLCh surface::tagname[] = {
  chLatin_s, chLatin_u, chLatin_r, chLatin_f, chLatin_a, chLatin_c, chLatin_e,
  chNull
};

const XMLCh path::tagname[] = {
  chLatin_p, chLatin_a, chLatin_t, chLatin_h, chNull
};

const XMLCh typeinfo::tagname[] = {
  chLatin_t, chLatin_y, chLatin_p, chLatin_e, 
  chLatin_i, chLatin_n, chLatin_f, chLatin_o,
  chNull
};

const XMLCh stem::tagname[] = {
  chLatin_s, chLatin_t, chLatin_e, chLatin_m, chNull
};

const XMLCh infl::tagname[] = {
  chLatin_i, chLatin_n, chLatin_f, chLatin_l, chNull
};

const XMLCh fsmod::tagname[] = {
  chLatin_f, chLatin_s, chLatin_m, chLatin_o, chLatin_d, chNull
};

const XMLCh pos::tagname[] = {
  chLatin_p, chLatin_o, chLatin_s, chNull
};

const XMLCh ne::tagname[] = {
  chLatin_n, chLatin_e, chNull
};

const XMLCh ref::tagname[] = {
  chLatin_r, chLatin_e, chLatin_f, chNull
};

}

// ---------------------------------------------------------------------------
//  PICHandler: Overrides of the SAX ErrorHandler interface
// ---------------------------------------------------------------------------

PICHandler::PICHandler(bool downcase, bool translate_iso) {
  _state_factory = new pic::pic_state_factory(this);
  _state_stack.push(new pic::TOP_state(this));
  _err = 0;
  _error_occurred = false;
  _downcase_strings = downcase;
  _translate_iso_chars = translate_iso;
  _errbuf = NULL;
}

PICHandler::~PICHandler() {
  while (! _state_stack.empty()) {
    // *errlog << *_state_stack.top() ;
    delete _state_stack.top();
    _state_stack.pop();
  }
  delete _state_factory;   // and the state factory
  if (_errbuf != NULL) XMLString::release(&_errbuf) ;
}

void PICHandler::
print_sax_exception(const char * errtype, const SAXParseException& e) {
  fprintf(ferr, "SAX: %s %s:%d:%d: "
          , errtype, XMLCh2Latin(e.getSystemId())
          , (int) e.getLineNumber(), (int) e.getColumnNumber()) ;
  fprintf(ferr, "%s\n", XMLCh2Latin(e.getMessage()));
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
    pic::pic_base_state *oldState = _state_stack.top();
    // get the new state from the state factory
    pic::pic_base_state *newState = _state_factory->get_state(name);
    
    if (newState != NULL) {
      // eventually change from current state into new state on tag NAME  
      //cerr << *_state << "<" << *newState;
      try {
        oldState->changeTo(newState, attribs);
        _state_stack.push(newState);
      }
      catch(const Error &e) {
        delete newState;
        _error_occurred = true;
        _err++;
        // rethrow to get file location properly
        throw SAXParseException(e.getMessage(), *_loc);
      }
    } else {
      // unknown tag error: This is a fatal XML error because all tags in the
      // DTD should be covered
      _error_occurred = true;
      throw SAXParseException(errmsg((string) "unknown tag '"
                                     + XMLCh2Latin(name) + "'"), *_loc);
    }
  }
}

void
PICHandler::endElement(const XMLCh* const name)
{
  //cerr << "I saw Endelement: "<< XMLCh2Latin(name) << endl;
  // if we are not currently skipping an error, return to a state on the stack
  if (_err == 0) {
    pic::pic_base_state *oldState = _state_stack.top();
    _state_stack.pop();
    pic::pic_base_state *newState = _state_stack.top();

    //cerr << " " << *oldstate << ">" << *_state;
    // do anything necessary to finish the old state, eventually get results
    // from oldstate into new current state 
    try {
      newState->changeFrom(oldState);
    }
    catch (const Error &e) {
      // rethrow to get error location
      throw SAXParseException(e.getMessage(), *_loc);
    }
    delete oldState;
  } else {
    // processing of erroneous subunits is avoided
    _err--;
  }
}

void
PICHandler::characters(const XMLCh *const chars, const unsigned int len){
  _state_stack.top()->characters(chars, len);
}

XMLCh _umlaute[] = { 0xE4, 0xFC, 0xF6, 0xC4, 0xDC, 0xD6, 0xDF, chNull} ;

XMLCh isomorphix_umlaut[] = { 
  chLatin_a, chLatin_e, chLatin_u, chLatin_e, chLatin_o, chLatin_e,
  chLatin_A, chLatin_e, chLatin_U, chLatin_e, chLatin_O, chLatin_e,
  chLatin_s, chLatin_s, chNull
} ;

int german_umlaut_p(XMLCh c) { return XMLString::indexOf(_umlaute, c); }

string 
PICHandler::surface_string(const XMLCh *chars, const unsigned int len) const {
  XMLCh* res;
  if (_translate_iso_chars) {
    res = new XMLCh[2 * len + 1];
    unsigned int j = 0;
    int gindex = 0;
    for (unsigned int i = 0; i < len; i++) {
      gindex = german_umlaut_p(chars[i]);
      if (gindex >= 0) {
        gindex = 2 * gindex;
        res[j++] = isomorphix_umlaut[gindex];
        res[j++] = isomorphix_umlaut[gindex + 1];
      } else {
        res[j++] = chars[i];
      }
    }
    res[j] = chNull;
  } else {
    res = new XMLCh[len + 1];
    XMLString::copyNString(res, chars, len);
    res[len] = chNull;
  }
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


void PICHandler::add_item(tInputItem *new_item) {
  const char *ext_id = new_item->external_id().c_str();
  if (_item_table.find(ext_id) != _item_table.end())
    throw SAXParseException(errmsg((string) "ID '" + ext_id + "' used twice")
                            , *_loc);
  
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


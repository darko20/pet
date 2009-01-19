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

#include "mrs-states.h"
#include "unicode.h"
#include <sstream>

using namespace std;
using namespace XERCES_CPP_NAMESPACE;

namespace mrs {

/******************************************************************************
 Attribute helper functions
 *****************************************************************************/

const XMLCh yes_str[] = { chLatin_y,  chLatin_e,  chLatin_s, chNull };

bool mrs_base_state::
req_bool_attr(AttributeList& attr, char *aname) {
  const XMLCh *str;
  if ((str = attr.getValue(aname)) == NULL)
    throw Error((std::string) "required attribute '" + aname + "' missing");
  return (XMLString::compareIString(str, yes_str) == 0);
}

bool mrs_base_state::
opt_bool_attr(AttributeList& attr, char *aname, bool def){
  const XMLCh *str;
  if ((str = attr.getValue(aname)) == NULL) return def;
  return (XMLString::compareIString(str, yes_str) == 0);
}

int mrs_base_state::
req_int_attr(AttributeList& attr, char *aname) {
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

int mrs_base_state::
opt_int_attr(AttributeList& attr, char *aname, int def) {
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

char mrs_base_state::
req_char_attr(AttributeList& attr, char *aname) {
  const XMLCh *val;
  if ((val = attr.getValue(aname)) == NULL)
    throw Error((string) "required attribute '" + aname + "' missing");
  const char *str = XMLCh2UTF8(val);
  if (strlen(str) != 1)
    throw Error((string) "wrong char value '" + str + "' for attribute '"
                + aname + "'");
  return str[0];
}

char mrs_base_state::
opt_char_attr(AttributeList& attr, char *aname, char def) {
  const XMLCh *val;
  if ((val = attr.getValue(aname)) == NULL) return def;
  const char *str = XMLCh2UTF8(val);
  if (strlen(str) != 1)
    throw Error((string) "wrong char value '" + str + "' for attribute '"
                + aname + "'");
  return str[0];
}

double mrs_base_state::
req_double_attr(AttributeList& attr, char *aname) {
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

double mrs_base_state::
opt_double_attr(AttributeList& attr, char *aname, double def) {
  const XMLCh *val;
  if ((val = attr.getValue(aname)) == NULL) return def;
  const char *str = XMLCh2UTF8(val);
  char *end;
  double res = strtod(str, &end);
  if ((*end != '\0') || (str == end))
    throw Error((std::string) "wrong double value '" + str + "' for attribute '"
                + aname + "'");
  return res;
}

string mrs_base_state::
req_string_attr(AttributeList &attr, char *aname) {
  const XMLCh *val = attr.getValue(aname);
  if (val == NULL)
    throw Error((std::string) "required attribute '" + aname + "' missing");
#ifdef HAVE_ICU
  // TODO: [bmw] is it not necessary to explicitly XMLString::release the
  // XMLCh* val ???
  return Conv->convert((UChar *)val, XMLString::stringLen(val));
#else
  return XMLCh2Latin(val);
#endif
}

string mrs_base_state::
opt_string_attr(AttributeList &attr, char *aname) {
  const XMLCh *val = attr.getValue(aname);
  if (val == NULL) return "";
#ifdef HAVE_ICU
  return Conv->convert((UChar *)val, XMLString::stringLen(val));
#else
  return XMLCh2Latin(val);
#endif
}


/** Reject state transition \a from --> \a to */
void reject(mrs_base_state *to, mrs_base_state *from) {
  std::ostringstream ostr;
  ostr << "Can not change from " << from << " into " << to << endl;
  throw Error(ostr.str());
}

void mrs_base_state::enterState(mrs_base_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(TOP_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(mrs_list_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(mrs_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(ep_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(pred_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(spred_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(realpred_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(label_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(var_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(extrapair_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(path_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(value_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(fvpair_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(rargname_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(constant_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(hcons_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(hi_state* state, AttributeList& attr) {
  reject(this, state);
}
void mrs_base_state::enterState(lo_state* state, AttributeList& attr) {
  reject(this, state);
}

/* tag names */

const XMLCh TOP_state::tagname[] = { chNull };

const XMLCh mrs_list_state::tagname[] = { 
  chLatin_m, chLatin_r, chLatin_s, chDash,
  chLatin_l, chLatin_i, chLatin_s, chLatin_t, chNull
};

const XMLCh mrs_state::tagname[] = {
  chLatin_m, chLatin_r, chLatin_s, chNull
};

const XMLCh ep_state::tagname[] = {
  chLatin_e, chLatin_p, chNull
};

const XMLCh pred_state::tagname[] = {
  chLatin_p, chLatin_r, chLatin_e, chLatin_d, chNull
};

const XMLCh spred_state::tagname[] = {
  chLatin_s, chLatin_p, chLatin_r, chLatin_e, chLatin_d, chNull
};

const XMLCh realpred_state::tagname[] = {
  chLatin_r, chLatin_e, chLatin_a, chLatin_l,
  chLatin_p, chLatin_r, chLatin_e, chLatin_d, chNull
};

const XMLCh label_state::tagname[] = {
  chLatin_l, chLatin_a, chLatin_b, chLatin_e, chLatin_l, chNull
};

const XMLCh var_state::tagname[] = {
  chLatin_v, chLatin_a, chLatin_r, chNull
};

const XMLCh extrapair_state::tagname[] = {
  chLatin_e, chLatin_x, chLatin_t, chLatin_r, chLatin_a,
  chLatin_p, chLatin_a, chLatin_i, chLatin_r, chNull
};

const XMLCh path_state::tagname[] = {
  chLatin_p, chLatin_a, chLatin_t, chLatin_h, chNull
};

const XMLCh value_state::tagname[] = {
  chLatin_v, chLatin_a, chLatin_l, chLatin_u, chLatin_e, chNull
};

const XMLCh fvpair_state::tagname[] = {
  chLatin_f, chLatin_v, chLatin_p, chLatin_a, chLatin_i, chLatin_r, chNull
};

const XMLCh rargname_state::tagname[] = {
  chLatin_r, chLatin_a, chLatin_r, chLatin_g,
  chLatin_n, chLatin_a, chLatin_m, chLatin_e, chNull
};

const XMLCh constant_state::tagname[] = {
  chLatin_c, chLatin_o, chLatin_n, chLatin_s,
  chLatin_t, chLatin_a, chLatin_n, chLatin_t, chNull
};

const XMLCh hcons_state::tagname[] = {
  chLatin_h, chLatin_c, chLatin_o, chLatin_n, chLatin_s, chNull
};

const XMLCh hi_state::tagname[] = {
  chLatin_h, chLatin_i, chNull
};

const XMLCh lo_state::tagname[] = {
  chLatin_l, chLatin_o, chNull
};



/**
 * TOP_state --> mrs_list_state
 */
void mrs_list_state::
enterState(class TOP_state* state, AttributeList& attr) {
  _reader->mrss().clear();
}

/**
 * TOP_state <-- mrs_list_state
 */
void mrs_list_state::
leaveState(class TOP_state *state) {
}

/**
 * mrs_list_state --> mrs_state
 */
void mrs_state::
enterState(class mrs_list_state* state, AttributeList& attr) {
  _mrs = new tPSOA();
}

/**
 * mrs_list_state <-- mrs_state
 */
void mrs_state::
leaveState(class mrs_list_state* state) {
  _reader->mrss().push_back(_mrs); 
}

/** 
 * mrs_state --> ep_state
 */
void ep_state::
enterState(mrs_state* state, AttributeList& attr) {
  _rel = new tRel(state->_mrs);
}

/**
 * mrs_state <-- ep_state
 */
void ep_state::
leaveState(mrs_state* state) {
  state->_mrs->liszt.push_back(_rel);
}

/**
 * ep_state --> pred_state
 */
void pred_state::
enterState(class ep_state* state, AttributeList& attr) {
}

/**
 * ep_state <-- pred_state
 */
void pred_state::
leaveState(class ep_state* state) {
  state->_rel->pred = _predname;
}

/** the characters event */
void pred_state::
characters(const XMLCh *const chars, const unsigned int length) {
  _predname = _reader->surface_string(chars, length);
}

/**
 * ep_state --> spred_state
 */
void spred_state::
enterState(class ep_state* state, AttributeList& attr) {
}

/**
 * ep_state <-- spred_state
 */
void spred_state::
leaveState(class ep_state* state) {
  state->_rel->pred = "\"" + _predname + "\"";
}

/** the characters event */
void spred_state::
characters(const XMLCh *const chars, const unsigned length) {
  _predname = _reader->surface_string(chars, length);
}

/**
 * mrs_state --> label_state
 */
void label_state::
enterState(class mrs_state* state, AttributeList& attr) {
  int vid = req_int_attr(attr, "vid");
  _label = state->_mrs->request_var(vid, 'h');
}

/**
 * mrs_state <-- label_state
 */
void label_state::
leaveState(class mrs_state* state) {
  state->_mrs->top_h = _label;
}

/**
 * ep_state --> label_state
 */
void label_state::
enterState(class ep_state* state, AttributeList& attr) {
  int vid = req_int_attr(attr, "vid");
  _label = state->_rel->_mrs->request_var(vid, 'h');
}

/**
 * ep_state <-- label_state
 */
void label_state::
leaveState(class ep_state* state) {
  state->_rel->handel = _label;
}

/**
 * label_state --> extrapair_state
 */
void extrapair_state::
enterState(class label_state* state, AttributeList& attr) {
  _var = state->_label;
}

/**
 * label_state <-- extrapair_state
 */
void extrapair_state::
leaveState(class label_state* state) {
  _var->extra[_path] = _value;
}

/**
 * var_state --> extrapair_state
 */
void extrapair_state::
enterState(class var_state* state, AttributeList& attr) {
  _var = state->_var;
}

/**
 * var_state <-- extrapair_state
 */
void extrapair_state::
leaveState(class var_state* state) {
  _var->extra[_path] = _value;
}

/**
 * extrapair_state --> path_state
 */
void path_state::
enterState(class extrapair_state* state, AttributeList& attr) {
  _var = state->_var;
}

/**
 * extrapair_state <-- path_state
 */
void path_state::
leaveState(class extrapair_state* state) {
  state->_path = _path;
}

/**
 * the characters event
 */
void path_state::
characters(const XMLCh *const chars, const unsigned int length) {
  _path = _reader->surface_string(chars, length);
}

/**
 * extrapair_state --> value_state
 */
void value_state::
enterState(class extrapair_state* state, AttributeList& attr) {
  _var = state->_var;
}

/**
 * extrapair_state <-- value_state
 */
void value_state::
leaveState(class extrapair_state* state) {
  state->_value = _value;
}

/**
 * the characters event
 */
void value_state::
characters(const XMLCh *const chars, const unsigned int length) {
  _value = _reader->surface_string(chars, length);
}


/**
 * mrs_state --> var_state
 */
void var_state::
enterState(class mrs_state* state, AttributeList& attr) {
  int vid = req_int_attr(attr, "vid");
  char sort = opt_char_attr(attr, "sort", 'u');
  _var = state->_mrs->request_var(vid, sort);
}

/**
 * mrs_state <-- var_state
 */
void var_state::
leaveState(class mrs_state* state) {
  state->_mrs->index = _var;
}

/**
 * fvpair_state --> var_state
 */
void var_state::
enterState(class fvpair_state* state, AttributeList& attr) {
  int vid = req_int_attr(attr, "vid");
  char sort = opt_char_attr(attr, "sort", 'u');
  _var = state->_rel->_mrs->request_var(vid, sort);
}

/**
 * fvpair_state <-- var_state
 */
void var_state::
leaveState(class fvpair_state* state) {
  state->_value = _var;
}

/**
 * hi_state --> var_state
 */
void var_state::
enterState(class hi_state* state, AttributeList& attr) {
  int vid = req_int_attr(attr, "vid");
  char sort = opt_char_attr(attr, "sort", 'u');
  _var = state->_hcons->_mrs->request_var(vid, sort);
}

/**
 * hi_state <-- var_state
 */
void var_state::
leaveState(class hi_state* state) {
  state->_var = _var;
}

/**
 * lo_state --> var_state
 */
void var_state::
enterState(class lo_state* state, AttributeList& attr) {
  int vid = req_int_attr(attr, "vid");
  char sort = opt_char_attr(attr, "sort", 'u');
  _var = state->_hcons->_mrs->request_var(vid, sort);
}

/**
 * lo_state <-- var_state
 */
void var_state::
leaveState(class lo_state* state) {
  state->_var = _var;
}

/**
 * ep_state --> fvpair_state
 */
void fvpair_state::
enterState(class ep_state* state, AttributeList& attr) {
  _rel = state->_rel;
}

/**
 * ep_state <-- fvpair_state
 */
void fvpair_state::
leaveState(class ep_state* state) {
  _rel->flist[_feature] = _value;
}

/**
 * fvpair_state --> rargname_state
 */
void rargname_state::
enterState(class fvpair_state* state, AttributeList& attr) {
}

/**
 * fvpair_state <-- rargname_state
 */
void rargname_state::
leaveState(class fvpair_state* state) {
  state->_feature = _feature;
}

/**
 * the characters event
 */
void rargname_state::
characters(const XMLCh *const chars, const unsigned int length) {
  _feature = _reader->surface_string(chars, length);
}

/**
 * fvpair_state --> constant_state
 */
void constant_state::
enterState(class fvpair_state* state, AttributeList& attr) {
  _rel = state->_rel;
}

/**
 * fvpair_state <-- constant_state
 */
void constant_state::
leaveState(class fvpair_state* state) {
  state->_value = _rel->_mrs->request_constant(_value);
}

/**
 * the characters event
 */
void constant_state::
characters(const XMLCh *const chars, const unsigned int length) {
  _value = _reader->surface_string(chars, length);
}

/**
 * mrs_state --> hcons_state
 */
void hcons_state::
enterState(class mrs_state* state, AttributeList& attr) {
  std::string hreln = req_string_attr(attr, "hreln");
  _hcons = new tHCons(hreln, state->_mrs);
}

/**
 * mrs_state <-- hcons_state
 */
void hcons_state::
leaveState(class mrs_state* state) {
  state->_mrs->h_cons.push_back(_hcons);
}


/**
 * hcons_state --> hi_state
 */
void hi_state::
enterState(class hcons_state* state, AttributeList& attr) {
  _hcons = state->_hcons;
}

/**
 * hcons_state <-- hi_state
 */
void hi_state::
leaveState(class hcons_state* state) {
  _hcons->scarg = _var;
}


/**
 * hcons_state --> lo_state
 */
void lo_state::
enterState(class hcons_state* state, AttributeList& attr) {
  _hcons = state->_hcons;
}

/**
 * hcons_state <-- lo_state
 */
void lo_state::
leaveState(class hcons_state* state) {
  _hcons->outscpd = _var;
}

}

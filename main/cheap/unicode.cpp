/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

// 
// This module provides 8 bit encoding - Unicode string conversion
// facilities. The ICU library implements the required functionality.
//

#include "pet-system.h"
#include "../common/errors.h"
#include "unicode.h"

EncodingConverter *Conv = 0;
EncodingConverter *ConvUTF8 = 0;

EncodingConverter::EncodingConverter(string encodingname) :
  _status(U_ZERO_ERROR), _conv(encodingname.c_str(), _status),
  _encoding(encodingname)
{
  if(U_FAILURE(_status))
    throw error("Couldn't open " + encodingname + " converter");
}

string EncodingConverter::convert(const UnicodeString from)
{
  int32_t sz = from.length() * _conv.getMaxBytesPerChar() + 1;
  char *s = New char [sz];

  _conv.fromUnicodeString(s, sz, from, _status);
  if(U_FAILURE(_status))
    throw error("Couldn't convert to " + _encoding);

  s[sz] = '\0';

  string to(s);
  delete[] s;
  
  return to;
};

UnicodeString EncodingConverter::convert(const string from)
{
  UnicodeString to;
  
  if(from.length() == 0) return to;
  _conv.toUnicodeString(to, from.c_str(), from.length(), _status);
  if(U_FAILURE(_status))
    throw error("Couldn't convert from " + _encoding + " (" + from + ")");
  
  return to;
};

void initialize_encoding_converter(string encoding)
{
  if(Conv) delete Conv;
  Conv = new EncodingConverter(encoding);
  if(ConvUTF8 == 0)
    ConvUTF8 = new EncodingConverter("utf8");
}

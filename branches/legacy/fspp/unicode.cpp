/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
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

// 
// This module provides 8 bit encoding - Unicode string conversion
// facilities. The ICU library implements the required functionality.
//

using namespace std;

//#include "pet-system.h"
#include "errors.h"
#include "unicode.h"

EncodingConverter::EncodingConverter(string encodingname) :
  _status(U_ZERO_ERROR), _conv(0),
  _encoding(encodingname)
{
  _conv = ucnv_open(encodingname.c_str(), &_status);
  if(U_FAILURE(_status))
    throw tError("Couldn't open " + encodingname + " converter");
}

EncodingConverter::~EncodingConverter()
{
  ucnv_close(_conv);
}


string EncodingConverter::encodingName()
{
  return ucnv_getName(_conv, &_status);
};

string EncodingConverter::convert(const UnicodeString from)
{
  int32_t sz = from.length() * ucnv_getMaxCharSize(_conv) + 1;
  char *s = new char [sz];

  sz = ucnv_fromUChars(_conv, s, sz, from.getBuffer(), from.length(), &_status);
  if(U_FAILURE(_status))
    throw tError("Couldn't convert to " + _encoding);

  s[sz] = '\0';

  string to(s);
  delete[] s;
  
  return to;
};

string EncodingConverter::convert(const UChar* from, int32_t length)
{
  int32_t sz = length * ucnv_getMaxCharSize(_conv) + 1;
  char *s = new char [sz];

  sz = ucnv_fromUChars(_conv, s, sz, from, length, &_status);
  if(U_FAILURE(_status))
    throw tError("Couldn't convert to " + _encoding);

  s[sz] = '\0';

  string to(s);
  delete[] s;
  
  return to;
};

UnicodeString EncodingConverter::convert(const string from)
{
  UnicodeString to;
  if(from.length() == 0) return to;

  int32_t sz = 2 * from.length(); 
  UChar * toBuffer = to.getBuffer(sz);

  sz = ucnv_toUChars(_conv, toBuffer, sz, from.c_str(), from.length(), &_status); 
  if(U_FAILURE(_status))
    throw tError("Couldn't convert from " + _encoding + " (" + from + ")");

  to.releaseBuffer(sz);
  
  return to;
};


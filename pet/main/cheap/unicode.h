/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

// 
// This module provides 8 bit encodings - Unicode strings conversion
// facilities. The ICU library provides the required functionality.
//

#ifndef _UNICODE_H_
#define _UNICODE_H_

extern class EncodingConverter *Conv;
extern class EncodingConverter *ConvUTF8;

//
// Encoding conversion utility class
// This class implements UnicodeString <-> STL String
// converters using a specified encoding for the STL strings (e.g. UTF8)
//

class EncodingConverter
{
public:
  EncodingConverter(string encodingname);

  string convert(const UnicodeString from);
  UnicodeString convert(const string from);

private:
  UErrorCode _status;
  UnicodeConverter _conv;
  string _encoding;
};

void initialize_encoding_converter(string encoding);

#endif

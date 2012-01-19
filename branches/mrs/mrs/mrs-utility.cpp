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

/* general helper functions and classes */

#include "mrs-utility.h"

#include <boost/algorithm/string.hpp>

using std::string;

/** Convert all characters in \a s to lower case */
void strtolower(char *s)
{
  if(s == NULL) return;
  while(*s)
  {
    *s = tolower(*s);
    s++;
  }
}

/** Convert all characters in \a s to upper case */
void strtoupper(char *s)
{
  if(s == NULL) return;
  while(*s)
  {
    *s = toupper(*s);
    s++;
  }
}

/** Convert all characters in \a src to lower case and copy to \a dest */
void strtolower(char *dest, const char *src) {
  if (src == NULL || dest == NULL) return;
  while (*src) {
    *dest = tolower(*src);
    src++, dest++;
  }
  *dest = *src;
}

/** Convert all characters in \a src to upper case and copy to \a dest */
void strtoupper(char *dest, const char *src) {
  if (src == NULL || dest == NULL) return;
  while (*src) {
    *dest = toupper(*src);
    src++, dest++;
  }
  *dest = *src;
}

/**
 * Escape XML delimiters.
 */
std::string
xml_escape(std::string s) {
  boost::replace_all(s, "<", "&lt;");
  boost::replace_all(s, ">", "&gt;");
  boost::replace_all(s, "&", "&amp;");
  boost::replace_all(s, "'", "&apos;");
  boost::replace_all(s, "\"", "&quot;");
  return s;
}

/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 2007 Peter Adolphs
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

/**
 * \file types-test.cpp
 * Unit tests for types.
 */ 

#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>
#include "types.h"
#include "grammar.h"

using namespace std;

class tTypesTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(tTypesTest);
  CPPUNIT_TEST(test_dyntypes);
  CPPUNIT_TEST_SUITE_END();
  
private:
  
public:
  /**
   * Inherited from CppUnit::TestFixture .
   * Automatically started before each test.
   */
  void setUp()
  {
  }
  
  /**
   * Inherited from CppUnit::TestFixture .
   * Automatically started after each test.
   */    
  void tearDown() 
  {
  }
  
  void test_dyntypes()
  {
    const char *s1 = "\"supercalifragilistic\"";
    const char *s2 = "\"expialidocious\"";
    CPPUNIT_ASSERT(lookup_type(s1) == -1);
    type_t t1 = lookup_symbol(s1);
    type_t t2 = lookup_symbol(s2);
    // TODO: for a registered dynamic type t, is_type(t) == true but 
    // lookup_type(t) == -1. bug or feature??
    //CPPUNIT_ASSERT(lookup_type(s1) == t1);
    
    CPPUNIT_ASSERT(t1 > 0);
    CPPUNIT_ASSERT(is_type(t1));
    CPPUNIT_ASSERT(!is_proper_type(t1));
    CPPUNIT_ASSERT(is_dynamic_type(t1));
    CPPUNIT_ASSERT(!is_static_type(t1));
    CPPUNIT_ASSERT(is_leaftype(t1));
    
    CPPUNIT_ASSERT(strcmp(type_name(t1), s1) == 0);
    CPPUNIT_ASSERT(strcmp(print_name(t1), s1) == 0);
    
    std::list<type_t> supertypes = all_supertypes(t1);
    std::list<type_t> expected_supertypes;
    expected_supertypes.push_back(BI_STRING);
    expected_supertypes.push_back(t1);
    CPPUNIT_ASSERT(supertypes == expected_supertypes);
    
    CPPUNIT_ASSERT(subtype(t1, BI_STRING));
    CPPUNIT_ASSERT(!subtype(BI_STRING, t1));
    CPPUNIT_ASSERT(!subtype(t1, t2));
    CPPUNIT_ASSERT(!subtype(t2, t1));
    
    bool forward, backward;
    subtype_bidir(t1, BI_TOP, forward, backward);
    CPPUNIT_ASSERT(forward && !backward);
    subtype_bidir(BI_TOP, t1, forward, backward);
    CPPUNIT_ASSERT(!forward && backward);
    subtype_bidir(t1, BI_STRING, forward, backward);
    CPPUNIT_ASSERT(forward && !backward);
    subtype_bidir(BI_STRING, t1, forward, backward);
    CPPUNIT_ASSERT(!forward && backward);
    subtype_bidir(t1, t2, forward, backward);
    CPPUNIT_ASSERT(!forward && !backward);
    
    CPPUNIT_ASSERT(glb(t1, t1) == t1);
    CPPUNIT_ASSERT(glb(t1, BI_STRING) == t1);
    CPPUNIT_ASSERT(glb(BI_STRING, t1) == t1);
    
    CPPUNIT_ASSERT(leaftype_parent(t1) == BI_STRING);
  }
    
};

CPPUNIT_TEST_SUITE_REGISTRATION(tTypesTest);

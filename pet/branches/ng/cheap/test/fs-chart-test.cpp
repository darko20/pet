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
 * \file fs-chart-test.cpp
 * Unit tests for tChart and related classes.
 */

#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>

#include "errors.h"
#include "fs.h"
#include "fs-chart.h"
#include "grammar.h"
#include "item.h"

using namespace std;

extern tGrammar* Grammar;

class tChartTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(tChartTest);
  CPPUNIT_TEST(test_initialization);
  CPPUNIT_TEST(test_lattice);
  CPPUNIT_TEST_SUITE_END();

private:
  tChart _chart;
  tChartVertex *_v0;
  tChartVertex *_v1;
  tChartVertex *_v2;
  tChartVertex *_v3;
  tItem *_i0;
  tItem *_i1;
  tItem *_i2;
  tItem *_i3;
  tItem *_i4;

public:

  /**
   * Inherited from CppUnit::TestFixture .
   * Automatically started before each test.
   */
  void setUp()
  {
    _v0 = _chart.add_vertex(tChartVertex::create());
    _v1 = _chart.add_vertex(tChartVertex::create());
    _v2 = _chart.add_vertex(tChartVertex::create());
    _v3 = _chart.add_vertex(tChartVertex::create());
    _i0 = _chart.add_item(new tInputItem("id",-1,-1,-1,-1,"f","s"), _v0, _v1);
    _i1 = _chart.add_item(new tInputItem("id",-1,-1,-1,-1,"f","s"), _v1, _v2);
    _i2 = _chart.add_item(new tInputItem("id",-1,-1,-1,-1,"f","s"), _v2, _v3);
    _i3 = _chart.add_item(new tInputItem("id",-1,-1,-1,-1,"f","s"), _v2, _v3);
    _i4 = _chart.add_item(new tInputItem("id",-1,-1,-1,-1,"f","s"), _v1, _v3);
  }

  /**
   * Inherited from CppUnit::TestFixture .
   * Automatically started after each test.
   */
  void tearDown()
  {
    _chart.clear();
  }

  void test_initialization()
  {
    CPPUNIT_ASSERT(_chart.vertices().size() > 0);
    CPPUNIT_ASSERT(_chart.items().size() > 0);
    _chart.clear();
    CPPUNIT_ASSERT(_chart.vertices().size() == 0);
    CPPUNIT_ASSERT(_chart.items().size() == 0);
    _chart.clear(); // clearing twice should not change anything
    CPPUNIT_ASSERT(_chart.vertices().size() == 0);
    CPPUNIT_ASSERT(_chart.items().size() == 0);
    _v1 = _chart.add_vertex(tChartVertex::create());
    _v2 = _chart.add_vertex(tChartVertex::create());
    _v3 = _chart.add_vertex(tChartVertex::create());
    _chart.add_item(new tInputItem("id",-1,-1,-1,-1, "form", "stem"), _v1, _v2);
    _chart.add_item(new tInputItem("id",-1,-1,-1,-1, "form", "stem"), _v2, _v3);
    _chart.add_item(new tInputItem("id",-1,-1,-1,-1, "form", "stem"), _v2, _v3);
    CPPUNIT_ASSERT(_chart.vertices().size() == 3);
    CPPUNIT_ASSERT(_chart.items().size() == 3);
  }

  void test_lattice()
  {
    CPPUNIT_ASSERT(_i0->get_prec_vertex() == _v0);
    CPPUNIT_ASSERT(_i0->get_succ_vertex() == _v1);
    CPPUNIT_ASSERT(_i1->get_prec_vertex() == _v1);
    CPPUNIT_ASSERT(_i1->get_succ_vertex() == _v2);
    CPPUNIT_ASSERT(_i2->get_prec_vertex() == _v2);
    CPPUNIT_ASSERT(_i2->get_succ_vertex() == _v3);
    CPPUNIT_ASSERT(_i3->get_prec_vertex() == _v2);
    CPPUNIT_ASSERT(_i3->get_succ_vertex() == _v3);
    CPPUNIT_ASSERT(_i4->get_prec_vertex() == _v1);
    CPPUNIT_ASSERT(_i4->get_succ_vertex() == _v3);

    std::list<tItem*> expected_items;

    expected_items.clear();
    CPPUNIT_ASSERT(_v0->ending_items() == expected_items);

    expected_items.clear();
    expected_items.push_back(_i0);
    CPPUNIT_ASSERT(_v1->ending_items() == expected_items);

    expected_items.clear();
    expected_items.push_back(_i1);
    expected_items.push_back(_i4);
    CPPUNIT_ASSERT(_v1->starting_items() == expected_items);

    expected_items.clear();
    expected_items.push_back(_i1);
    CPPUNIT_ASSERT(_v2->ending_items() == expected_items);

    expected_items.clear();
    expected_items.push_back(_i2);
    expected_items.push_back(_i3);
    CPPUNIT_ASSERT(_v2->starting_items() == expected_items);

    expected_items.clear();
    expected_items.push_back(_i2);
    expected_items.push_back(_i3);
    expected_items.push_back(_i4);
    CPPUNIT_ASSERT(_v3->ending_items() == expected_items);

    expected_items.clear();
    CPPUNIT_ASSERT(_v3->starting_items() == expected_items);

    expected_items.clear();
    expected_items.push_back(_i0);
    expected_items.push_back(_i1);
    expected_items.push_back(_i2);
    expected_items.push_back(_i3);
    expected_items.push_back(_i4);
    CPPUNIT_ASSERT(_chart.items() == expected_items);

    expected_items.clear();
    expected_items.push_back(_i2);
    expected_items.push_back(_i3);
    CPPUNIT_ASSERT(_chart.succeeding_items(_v2, 0, 0) == expected_items);
    CPPUNIT_ASSERT(_chart.succeeding_items(_v2, 0, 42) == expected_items);

    expected_items.clear();
    CPPUNIT_ASSERT(_chart.succeeding_items(_v3, 0, 42) == expected_items);

    expected_items.clear();
    expected_items.push_back(_i1);
    expected_items.push_back(_i4);
    expected_items.push_back(_i2);
    expected_items.push_back(_i3);
    CPPUNIT_ASSERT(_chart.succeeding_items(_v1, 0, 42) == expected_items);
  }

  /* TODO
  void test_item_removal()
  {
  }

  void test_connectivity()
  {
  }
  */
};

CPPUNIT_TEST_SUITE_REGISTRATION(tChartTest);

/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 2002 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* unit tests for tPaths class */

#include <cstdlib>
#include <cppunit/extensions/HelperMacros.h>

using namespace std;

#include "paths.h"

class tPathsTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(tPathsTest);
    CPPUNIT_TEST(testDefault);
    CPPUNIT_TEST(testEmpty);
    CPPUNIT_TEST(testCompatible);
    CPPUNIT_TEST_SUITE_END();

private:
    tPaths *_pDefault, *_pEmpty, *_pa, *_pb, *_pc, *_pab, *_pac, *_pbc, *_pabc,
        *_pd, *_pmany;
    
public:    
    void setUp()
    {
        const int a = 1, b = 8, c = 33, d = 15;

        _pDefault = new tPaths();

        list<int> v;
        _pEmpty = new tPaths(v);
        
        v.push_back(a);
        _pa = new tPaths(v);
        
        v.push_back(b);
        _pab = new tPaths(v);
        
        v.push_back(c);
        _pabc = new tPaths(v);
        
        v.clear();
        v.push_back(b);
        _pb = new tPaths(v);
        
        v.clear();
        v.push_back(c);
        _pc = new tPaths(v);
        
        v.push_back(b);
        _pbc = new tPaths(v);
        
        v.clear();
        v.push_back(a);
        v.push_back(c);
        _pac = new tPaths(v);
        
        v.clear();
        v.push_back(d);
        _pd = new tPaths(v);
        
        for(int i = 0; i < 99; ++i)
            v.push_back(rand() % 511);
        
        _pmany = new tPaths(v);
    }

    void tearDown() 
    {
        delete _pDefault;
        delete _pEmpty;
        delete _pabc;
        delete _pac;
        delete _pbc;
        delete _pab;
        delete _pa;
        delete _pb;
        delete _pc;
        delete _pd;
        delete _pmany;
    }

    // Basic assumptions about the wildcard set (default).
    void testDefault()
    {
        CPPUNIT_ASSERT(_pDefault->compatible(*_pDefault));
        CPPUNIT_ASSERT(_pDefault->compatible(*_pabc));
        CPPUNIT_ASSERT(_pDefault->compatible(*_pac));
        CPPUNIT_ASSERT(_pDefault->compatible(*_pd));
        list<int> p = _pDefault->get();
        CPPUNIT_ASSERT(p.size() == 0);
    }
    
    // Basic assumptions about the empty set.
    void testEmpty()
    {
        CPPUNIT_ASSERT(!_pEmpty->compatible(*_pEmpty));
        CPPUNIT_ASSERT(!_pEmpty->compatible(*_pDefault));
        CPPUNIT_ASSERT(!_pEmpty->compatible(*_pabc));
        CPPUNIT_ASSERT(!_pEmpty->compatible(*_pac));
        CPPUNIT_ASSERT(!_pEmpty->compatible(*_pd));
        list<int> p = _pEmpty->get();
        CPPUNIT_ASSERT(p.size() == 1);
        CPPUNIT_ASSERT(p.front() == -1);
    }

    // The 'compatible' function.
    void testCompatible()
    {
        CPPUNIT_ASSERT(_pabc->compatible(*_pabc));
        CPPUNIT_ASSERT(_pabc->compatible(*_pac));
        CPPUNIT_ASSERT(_pac->compatible(*_pabc));
        CPPUNIT_ASSERT(_pab->compatible(*_pbc));
        CPPUNIT_ASSERT(!_pabc->compatible(*_pd));
    }

    // The 'common' function.
    void testCommon()
    {
        CPPUNIT_ASSERT(_pab->common(*_pbc).compatible(*_pb));
    }
    
};

CPPUNIT_TEST_SUITE_REGISTRATION(tPathsTest);

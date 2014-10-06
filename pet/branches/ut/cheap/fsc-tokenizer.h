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

/** \file fsc-tokenizer.h
 * Tokenizer for the feature-structure chart XML format.
 */

#ifndef _FSC_TOKENIZER_H
#define _FSC_TOKENIZER_H

#include "pet-config.h"

#include "fs.h"
#include "fs-chart.h"
#include "hashing.h"
#include "input-modules.h"
#include "types.h"

#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/Locator.hpp>
#include <xercesc/util/XMLString.hpp>

#include <list>
#include <map>
#include <stack>
#include <string>

// forward declaration:
namespace fsc
{
  class tFSCState;
  class tFSCBottomState;
  class tFSCState_fsc;
  class tFSCState_chart;
  class tFSCState_text;
  class tFSCState_lattice;
  class tFSCState_edge;
  class tFSCState_fs;
  class tFSCState_f;
  class tFSCState_str;
}



/**
 * Tokenizer reading the XML-based feature-structure-chart format. Activates
 * chart mapping rules.
 */
class tFSCTokenizer : public tTokenizer {
public:

  /** Constructor. */
  tFSCTokenizer();

  /** Destructor. */
  virtual ~tFSCTokenizer();

  // comment provided by super-class
  virtual bool next_input(std::istream &in, std::string &result);

  // comment provided by super-class
  virtual void tokenize(std::string s, inp_list &result);

  /**
   * Produce a feature structure chart from the given string for the
   * application of chart mapping rules.
   */
  virtual void tokenize(std::string s, tChart &result);

  // comment provided by super-class
  virtual std::string description();

  // comment provided by super-class
  virtual position_map position_mapping();

};



/**
 * Factory creating states for parsing FSC files.
 */
class tFSCStateFactory {

private:

  /**
   * A map mapping state names to state prototypes. Needed for creating
   * new states by their name. See Prototype pattern.
   */
  std::map< std::string, const fsc::tFSCState* > _state_map;

  /**
   * Registers a prototype for a certain state \a state under its
   * name \a state->name().
   */
  void register_state(const fsc::tFSCState *state);

public:

  /** Constructor. */
  tFSCStateFactory();

  /** Destructor. */
  virtual ~tFSCStateFactory();

  /**
   * Creates a new state for the XML element with name \a name. The caller
   * of this function is responsible for deleting the returned state object.
   */
  fsc::tFSCState* create_state(const std::string &name);

};



/**
 * A SAX parser handler class to read exactly one input chart from an
 * XML stream in the FSC format.
 */
class tFSCHandler : public XERCES_CPP_NAMESPACE_QUALIFIER HandlerBase
{
private:
  /** The current stack of XML states (nodes) */
  std::stack<fsc::tFSCState*> _state_stack;

  /** A factory producing states for XML element names. */
  tFSCStateFactory _factory;

  /**
   * The document locator allows us to associate SAX events with
   * the document position where the event occurred.
   */
  const XERCES_CPP_NAMESPACE_QUALIFIER Locator *_loc;

  /**
   * Build an error message string, using the specified \a exception category
   * and \a message, as well as the registered document locator.
   */
  std::string
  errmsg(std::string category, std::string message);

  /** The copy constructor is disallowed */
  tFSCHandler(const tFSCHandler &x);

public:

  /** Constructor: Make a new tFSCHandler */
  tFSCHandler(tChart &chart);
  /** Destructor */
  ~tFSCHandler();

  /** \name Xerces DocumentHandler interface */
  /*@{*/
  /** Receive notification of the beginning of an element. */
  virtual void
  startElement(const XMLCh* const xml_name,
      XERCES_CPP_NAMESPACE_QUALIFIER AttributeList& xml_attrs);
  /** Receive notification of the end of an element. */
  virtual void
  endElement(const XMLCh* const xml_name);
  /** Receive notification of character data. Can be called multiple times. */
  virtual void
  characters(const XMLCh *const xml_chars, const
#if (XERCES_VERSION_MAJOR < 3)
             unsigned int
#else
             XMLSize_t
#endif
             len);
  /**
   * Sets the document locator which allows us to associate SAX events with
   * the document position where the event occurred.
   */
  virtual void
  setDocumentLocator(const XERCES_CPP_NAMESPACE_QUALIFIER Locator* const loc);
  /*@}*/

  /** \name Xerces ErrorHandler interface */
  /*@{*/
  /** Receive notification of a warning. */
  virtual void
  warning(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e);
  /** Receive notification of a recoverable error. */
  virtual void
  error(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e);
  /** Receive notification of a non-recoverable error. */
  virtual void
  fatalError(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e);
  /*@}*/

};



/**
 * A namespace for the states used while parsing FSC files.
 */
namespace fsc
{

  /**
   * An (abstract) state for parsing FSC XML files.
   */
  class tFSCState {
  private:

    /** The name of this state. */
    std::string _name;

    /** Reject state transition from state \a source to \a target */
    static void reject(tFSCState *source, tFSCState *target);

  protected:

    tFSCState(std::string name);

  public:

    tFSCState();

    virtual ~tFSCState();

    virtual tFSCState* clone() const = 0;

    std::string name() const;

    /**
     * Changes from this state to the \a target state. This in turn calls
     * \a target->enter(this, attrs). (Double Dispatch pattern)
     */
    virtual void change_to(tFSCState *target,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs) = 0;

    /**
     * Changes from this state to the \a source state. This in turn calls
     * \a source->leave(this). (Double Dispatch pattern)
     */
    virtual void change_from(tFSCState *source) = 0;

    virtual void enter(tFSCBottomState *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void enter(tFSCState_fsc *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void enter(tFSCState_chart *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void enter(tFSCState_text *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void enter(tFSCState_lattice *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void enter(tFSCState_edge *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void enter(tFSCState_fs *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void enter(tFSCState_f *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void enter(tFSCState_str *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);

    virtual void leave(tFSCBottomState *target);
    virtual void leave(tFSCState_fsc *target);
    virtual void leave(tFSCState_chart *target);
    virtual void leave(tFSCState_text *target);
    virtual void leave(tFSCState_lattice *target);
    virtual void leave(tFSCState_edge *target);
    virtual void leave(tFSCState_fs *target);
    virtual void leave(tFSCState_f *target);
    virtual void leave(tFSCState_str *target);

    virtual void characters(std::string chars);

  };

  class tFSCBottomState : public tFSCState
  {
  private:
    tChart *_chart;
  public:
    tFSCBottomState();
    virtual tFSCBottomState* clone() const;
    virtual void change_to(tFSCState *target,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void change_from(tFSCState *source);
    void set_chart(tChart *chart);
    tChart *get_chart();
  };

  class tFSCState_fsc : public tFSCState
  {
  private:
    tChart *_chart;
  public:
    tFSCState_fsc();
    virtual tFSCState_fsc* clone() const;
    virtual void change_to(tFSCState *target,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void change_from(tFSCState *source);
    virtual void enter(tFSCBottomState *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void leave(tFSCBottomState *target);
    tChart *get_chart();
  };

  class tFSCState_chart : public tFSCState
  {
  private:
    tChart *_chart;
  public:
    tFSCState_chart();
    virtual tFSCState_chart* clone() const;
    virtual void change_to(tFSCState *target,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void change_from(tFSCState *source);
    virtual void enter(tFSCState_fsc *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void leave(tFSCState_fsc *target);
    tChart *get_chart();
  };

  class tFSCState_text : public tFSCState
  {
  public:
    tFSCState_text();
    virtual tFSCState_text* clone() const;
    virtual void change_to(tFSCState *target,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void change_from(tFSCState *source);
    virtual void enter(tFSCState_chart *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void leave(tFSCState_chart *target);
    virtual void characters(std::string chars);
  };

  class tFSCState_lattice : public tFSCState
  {
  private:
    HASH_SPACE::hash_map<std::string, tChartVertex*, standard_string_hash>
      _vertex_map;
    tChart *_chart;
  public:
    tFSCState_lattice();
    virtual tFSCState_lattice* clone() const;
    virtual void change_to(tFSCState *target,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void change_from(tFSCState *source);
    virtual void enter(tFSCState_chart *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void leave(tFSCState_chart *target);
    void add_item(fs f, std::string vfrom_name, std::string vto_name);
  };

  class tFSCState_edge : public tFSCState
  {
  private:
    /** feature structure associated with this state */
    fs _fs;
    /** name for the source vertex (vfrom) */
    std::string _vfrom_name;
    /** name for the target vertex (vto) */
    std::string _vto_name;
  public:
    tFSCState_edge();
    virtual tFSCState_edge* clone() const;
    virtual void change_to(tFSCState *target,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void change_from(tFSCState *source);
    virtual void enter(tFSCState_lattice *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void leave(tFSCState_lattice *target);
    void set_fs(fs f);
  };

  class tFSCState_fs : public tFSCState
  {
  private:
    /** feature structure represented by this state */
    fs _fs;
  public:
    tFSCState_fs();
    virtual tFSCState_fs* clone() const;
    virtual void change_to(tFSCState *target,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void change_from(tFSCState *source);
    virtual void enter(tFSCState_edge *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void leave(tFSCState_edge *target);
    virtual void enter(tFSCState_f *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void leave(tFSCState_f *target);
    void set_fs(fs f);
    fs get_fs();
  };

  class tFSCState_f : public tFSCState
  {
  private:
    /** object holding the feature structure to which this feature belongs to */
    tFSCState_fs *_parent;
    /** attribute code for the current feature state */
    attr_t _attr;
    /** \c true if the value of this feature is a list */
    bool _is_list;
    /** values to be set in this feature */
    std::list<fs> _values;
  public:
    tFSCState_f();
    virtual tFSCState_f* clone() const;
    virtual void change_to(tFSCState *target,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void change_from(tFSCState *source);
    virtual void enter(tFSCState_fs *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void leave(tFSCState_fs *target);
    void add_value(fs value);
  };

  class tFSCState_str : public tFSCState
  {
  private:
    std::string _chars;
  public:
    tFSCState_str();
    virtual tFSCState_str* clone() const;
    virtual void change_to(tFSCState *target,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void change_from(tFSCState *source);
    virtual void enter(tFSCState_f *source,
        XERCES_CPP_NAMESPACE_QUALIFIER AttributeList &attrs);
    virtual void leave(tFSCState_f *target);
    virtual void characters(std::string chars);
  };

} // namespace fsc




// ========================================================================
// INLINE DEFINITIONS
// ========================================================================

// =====================================================
// class tFSCTokenizer
// =====================================================

inline
tFSCTokenizer::tFSCTokenizer()
: tTokenizer()
{
  // done
}

inline
tFSCTokenizer::~tFSCTokenizer()
{
  // done
}

inline std::string
tFSCTokenizer::description()
{
  return "FSC tokenization";
}

inline position_map
tFSCTokenizer::position_mapping()
{
  return NO_POSITION_MAP;
}

#endif

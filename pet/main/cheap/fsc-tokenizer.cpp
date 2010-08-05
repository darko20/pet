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

/* tokenizer base class and simple lingo tokenizer */

#include "fsc-tokenizer.h"

#include "chart-mapping.h"
#include "fs-chart.h"
#include "grammar.h"
#include "list-int.h"
#include "xmlparser.h"

#include <iostream>
#include <fstream>
#include <list>
#include <memory>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

XERCES_CPP_NAMESPACE_USE

using namespace std;



// =====================================================
// class tFSCTokenizer
// =====================================================

bool
tFSCTokenizer::next_input(std::istream &in, std::string &input)
{
  input.clear();

  string nextline;
  do {
    if (in.eof())
      return false;
    std::getline(in, nextline);
    boost::trim(nextline);
    if (input.empty() && (! nextline.empty())) {
      if (nextline.at(0) != '<') {
        input = nextline;
        return true;
      }
    }
    input.append(nextline);
  } while ((nextline.find("</fsc>") == string::npos));

  // stuff string after end tag back into stream
  size_t end = input.find("</fsc>") + 6;
  size_t real_end = input.length();
  for (size_t i = real_end - 1; i >= end; i--) {
    in.putback(input.at(i));
  }
  input.erase(end, real_end - end);

  return !input.empty();
}

void
tFSCTokenizer::tokenize(std::string s, inp_list &result)
{
  tChart chart;
  tokenize(s, chart);
  tChartUtil::map_chart(chart, result);
}

void
tFSCTokenizer::tokenize(std::string s, tChart &result)
{
  tFSCHandler fsc_handler(result);
  InputSource *i;
  if (s.at(0) == '<') {
    MemBufInputSource xml_input((const XMLByte *) s.c_str(), s.length(),
                                "STDIN");
    parse_file(xml_input, &fsc_handler);
  }
  else {
    XMLCh * XMLFilename = XMLString::transcode(s.c_str());
    LocalFileInputSource inputfile(XMLFilename);
    parse_file(inputfile, &fsc_handler);
  }
}



// =====================================================
// class tFSCHandler
// =====================================================

tFSCHandler::tFSCHandler(tChart &chart)
{
  _state_stack.push(_factory.create_state("*bottom*"));
  dynamic_cast<fsc::tFSCBottomState*>(_state_stack.top())->set_chart(&chart);
}

tFSCHandler::~tFSCHandler()
{
  while (!_state_stack.empty()) {
    fsc::tFSCState *state = _state_stack.top();
    _state_stack.pop();
    delete state;
  }
}

void
tFSCHandler::startElement(const XMLCh* const xml_name,
      XERCES_CPP_NAMESPACE_QUALIFIER AttributeList& xml_attrs)
{
  try {
    std::string tgtname = XMLCh2Native(xml_name);
    fsc::tFSCState *source = _state_stack.top();
    _state_stack.push(_factory.create_state(tgtname));
    fsc::tFSCState *target = _state_stack.top();
    source->change_to(target, xml_attrs);
  } catch (tError e) {
      throw tError(errmsg("Error", e.getMessage()));
  }
}

void
tFSCHandler::endElement(const XMLCh* const xml_name)
{
  try {
    fsc::tFSCState *source = _state_stack.top();
    _state_stack.pop();
    fsc::tFSCState *target = _state_stack.top();
    target->change_from(source);
    delete source;
  } catch (tError e) {
      throw tError(errmsg("Error", e.getMessage()));
  }
}

void
tFSCHandler::characters(const XMLCh *const xml_chars, const unsigned int len)
{
  try {
    std::string chars = XMLCh2Native(xml_chars);
    fsc::tFSCState *current = _state_stack.top();
    if (chars.find_first_not_of(" \t\n\r") != std::string::npos)
      current->characters(chars); // only called if chars is not only whitespace
  } catch (tError e) {
      throw tError(errmsg("Error", e.getMessage()));
  }
}

void
tFSCHandler::setDocumentLocator(const Locator* const loc)
{
  _loc = loc;
}

std::string
tFSCHandler::errmsg(std::string category, std::string message)
{
  return "[FSC tokenizer] " + category + " in "
    + XMLCh2Native(_loc->getSystemId())
    + ":" + boost::lexical_cast<std::string>(_loc->getLineNumber())
    + ":" + boost::lexical_cast<std::string>(_loc->getColumnNumber())
    + ": " + message;
}

void
tFSCHandler::warning(const SAXParseException& e)
{
  cerr << errmsg("Warning", XMLCh2Native(e.getMessage()));
}

void
tFSCHandler::error(const SAXParseException& e)
{
  cerr << errmsg("Error", XMLCh2Native(e.getMessage()));
}

void
tFSCHandler::fatalError(const SAXParseException& e)
{
  throw tError(errmsg("Fatal error", XMLCh2Native(e.getMessage())));
}



// =====================================================
// class tFSCStateFactory
// =====================================================

tFSCStateFactory::tFSCStateFactory()
{
  register_state(new fsc::tFSCBottomState());
  register_state(new fsc::tFSCState_fsc());
  register_state(new fsc::tFSCState_chart());
  register_state(new fsc::tFSCState_text());
  register_state(new fsc::tFSCState_lattice());
  register_state(new fsc::tFSCState_edge());
  register_state(new fsc::tFSCState_fs());
  register_state(new fsc::tFSCState_f());
  register_state(new fsc::tFSCState_str());
}

tFSCStateFactory::~tFSCStateFactory()
{
  // delete registered state prototypes:
  std::map< std::string, const fsc::tFSCState* >::iterator it;
  for (it = _state_map.begin(); it != _state_map.end(); it++) {
    delete it->second;
  }
}

void
tFSCStateFactory::register_state(const fsc::tFSCState *state)
{
  _state_map[state->name()] = state;
}

fsc::tFSCState*
tFSCStateFactory::create_state(const std::string &name)
{
  if (_state_map.find(name) == _state_map.end())
    throw tError("unknown FSC state \"" + name + "\" requested");
  return _state_map[name]->clone();
}



// =====================================================
// FSC states
// =====================================================

namespace fsc {

  // =====================================================
  // abstract state for parsing FSC files
  // =====================================================

  tFSCState::tFSCState()
  : _name("")
  { }

  tFSCState::tFSCState(std::string name)
  : _name(name)
  { }

  tFSCState::~tFSCState()
  { }

  std::string
  tFSCState::name() const
  {
    return _name;
  }

  void
  tFSCState::reject(tFSCState *s, tFSCState *t)
  {
    throw tError((std::string)"Cannot change from element " +
        "\"" + s->name() + "\" to element \"" + t->name() + "\".");
  }

  void tFSCState::enter(tFSCBottomState *source, AttributeList &attrs)
  { tFSCState::reject(source, this); }
  void tFSCState::enter(tFSCState_fsc *source, AttributeList &attrs)
  { tFSCState::reject(source, this); }
  void tFSCState::enter(tFSCState_chart *source, AttributeList &attrs)
  { tFSCState::reject(source, this); }
  void tFSCState::enter(tFSCState_text *source, AttributeList &attrs)
  { tFSCState::reject(source, this); }
  void tFSCState::enter(tFSCState_lattice *source, AttributeList &attrs)
  { tFSCState::reject(source, this); }
  void tFSCState::enter(tFSCState_edge *source, AttributeList &attrs)
  { tFSCState::reject(source, this); }
  void tFSCState::enter(tFSCState_fs *source, AttributeList &attrs)
  { tFSCState::reject(source, this); }
  void tFSCState::enter(tFSCState_f *source, AttributeList &attrs)
  { tFSCState::reject(source, this); }
  void tFSCState::enter(tFSCState_str *source, AttributeList &attrs)
  { tFSCState::reject(source, this); }

  void tFSCState::leave(tFSCBottomState *target)
  { tFSCState::reject(this, target); }
  void tFSCState::leave(tFSCState_fsc *target)
  { tFSCState::reject(this, target); }
  void tFSCState::leave(tFSCState_chart *target)
  { tFSCState::reject(this, target); }
  void tFSCState::leave(tFSCState_text *target)
  { tFSCState::reject(this, target); }
  void tFSCState::leave(tFSCState_lattice *target)
  { tFSCState::reject(this, target); }
  void tFSCState::leave(tFSCState_edge *target)
  { tFSCState::reject(this, target); }
  void tFSCState::leave(tFSCState_fs *target)
  { tFSCState::reject(this, target); }
  void tFSCState::leave(tFSCState_f *target)
  { tFSCState::reject(this, target); }
  void tFSCState::leave(tFSCState_str *target)
  { tFSCState::reject(this, target); }

  void
  tFSCState::characters(std::string chars)
  {
    throw tError("Characters are not allowed at this place.");
  }

  // =====================================================
  // bottom state
  // =====================================================

  tFSCBottomState::tFSCBottomState()
  : tFSCState("*bottom*")
  { }

  tFSCBottomState*
  tFSCBottomState::clone() const
  { return new tFSCBottomState(*this); }

  void
  tFSCBottomState::change_to(tFSCState *target, AttributeList &attrs)
  { target->enter(this, attrs); }

  void
  tFSCBottomState::change_from(tFSCState *source)
  { source->leave(this); }

  void
  tFSCBottomState::set_chart(tChart *chart)
  {
    _chart = chart;
  }

  tChart*
  tFSCBottomState::get_chart()
  {
    return _chart;
  }

  // =====================================================
  // state "fsc"
  // =====================================================

  tFSCState_fsc::tFSCState_fsc()
  : tFSCState("fsc")
  { }

  tFSCState_fsc*
  tFSCState_fsc::clone() const
  { return new tFSCState_fsc(*this); }

  void
  tFSCState_fsc::change_to(tFSCState *target, AttributeList &attrs)
  { target->enter(this, attrs); }

  void
  tFSCState_fsc::change_from(tFSCState *source)
  { source->leave(this); }

  void
  tFSCState_fsc::enter(tFSCBottomState *source, AttributeList &attrs)
  {
    // cerr << "<" + name() + ">\n";
    _chart = source->get_chart();
  }

  void
  tFSCState_fsc::leave(tFSCBottomState *target)
  {
    // cerr << "</" + name()  + ">\n";
  }

  tChart*
  tFSCState_fsc::get_chart()
  {
    return _chart;
  }

  // =====================================================
  // state "chart"
  // =====================================================

  tFSCState_chart::tFSCState_chart()
  : tFSCState("chart")
  { }

  tFSCState_chart*
  tFSCState_chart::clone() const
  { return new tFSCState_chart(*this); }

  void
  tFSCState_chart::change_to(tFSCState *target, AttributeList &attrs)
  { target->enter(this, attrs); }

  void
  tFSCState_chart::change_from(tFSCState *source)
  { source->leave(this); }

  void
  tFSCState_chart::enter(tFSCState_fsc *source, AttributeList &attrs)
  {
    // cerr << "<" + name() + ">\n";
    _chart = source->get_chart();
  }

  void
  tFSCState_chart::leave(tFSCState_fsc *target)
  {
    // cerr << "</" + name()  + ">\n";
  }

  tChart*
  tFSCState_chart::get_chart()
  {
    return _chart;
  }

  // =====================================================
  // state "text"
  // =====================================================

  tFSCState_text::tFSCState_text()
  : tFSCState("text")
  { }

  tFSCState_text*
  tFSCState_text::clone() const
  { return new tFSCState_text(*this); }

  void
  tFSCState_text::change_to(tFSCState *target, AttributeList &attrs)
  { target->enter(this, attrs); }

  void
  tFSCState_text::change_from(tFSCState *source)
  { source->leave(this); }

  void
  tFSCState_text::enter(tFSCState_chart *source, AttributeList &attrs)
  {
    // cerr << "<" + name() + ">\n";
  }

  void
  tFSCState_text::leave(tFSCState_chart *target)
  {
    // cerr << "</" + name()  + ">\n";
  }

  void
  tFSCState_text::characters(std::string chars)
  {
    // ignore
  }

  // =====================================================
  // state "lattice"
  // =====================================================

  tFSCState_lattice::tFSCState_lattice()
  : tFSCState("lattice")
  { }

  tFSCState_lattice*
  tFSCState_lattice::clone() const
  { return new tFSCState_lattice(*this); }

  void
  tFSCState_lattice::change_to(tFSCState *target, AttributeList &attrs)
  { target->enter(this, attrs); }

  void
  tFSCState_lattice::change_from(tFSCState *source)
  { source->leave(this); }

  void
  tFSCState_lattice::enter(tFSCState_chart *source, AttributeList &attrs)
  {
    // cerr << "<" + name() + ">\n";
    _chart = source->get_chart();
    _vertex_map.clear();
 }

  void
  tFSCState_lattice::leave(tFSCState_chart *target)
  {
    // cerr << "</" + name()  + ">\n";
  }

  void
  tFSCState_lattice::add_item(fs f, string vfrom_name, string vto_name)
  {
    if (_vertex_map[vfrom_name] == 0)
      _vertex_map[vfrom_name] = _chart->add_vertex(tChartVertex::create());
    if (_vertex_map[vto_name] == 0)
      _vertex_map[vto_name] = _chart->add_vertex(tChartVertex::create());
    tItem *item = tChartUtil::create_input_item(f);
    _chart->add_item(item, _vertex_map[vfrom_name], _vertex_map[vto_name]);
   }


  // =====================================================
  // state "edge"
  // =====================================================

  tFSCState_edge::tFSCState_edge()
  : tFSCState("edge")
  { }

  tFSCState_edge*
  tFSCState_edge::clone() const
  { return new tFSCState_edge(*this); }

  void
  tFSCState_edge::change_to(tFSCState *target, AttributeList &attrs)
  { target->enter(this, attrs); }

  void
  tFSCState_edge::change_from(tFSCState *source)
  { source->leave(this); }

  void
  tFSCState_edge::enter(tFSCState_lattice *source, AttributeList &attrs)
  {
    const XMLCh *src_vertex = attrs.getValue("source");
    const XMLCh *tgt_vertex = attrs.getValue("target");
    if (src_vertex == 0)
      throw tError("Missing required attribute \"source\".");
    if (tgt_vertex == 0)
      throw tError("Missing required attribute \"target\".");
    _vfrom_name = XMLCh2Native(src_vertex);
    _vto_name = XMLCh2Native(tgt_vertex);
  }

  void
  tFSCState_edge::leave(tFSCState_lattice *target)
  {
    // cerr << "</" + name()  + ">\n";
    target->add_item(_fs, _vfrom_name, _vto_name);
  }

  void
  tFSCState_edge::set_fs(fs f)
  {
    _fs = f;
  }

  // =====================================================
  // state "fs"
  // =====================================================

  tFSCState_fs::tFSCState_fs()
  : tFSCState("fs")
  { }

  tFSCState_fs*
  tFSCState_fs::clone() const
  { return new tFSCState_fs(*this); }

  void
  tFSCState_fs::change_to(tFSCState *target, AttributeList &attrs)
  { target->enter(this, attrs); }

  void
  tFSCState_fs::change_from(tFSCState *source)
  { source->leave(this); }

  void
  tFSCState_fs::enter(tFSCState_edge *source, AttributeList &attrs)
  {
    // cerr << "<" + name() + ">\n";
    // get type and instantiate the appropriate fs:
    const XMLCh *n = attrs.getValue("type");
    if (!n)
      throw tError("Missing required attribute \"type\".");
    type_t type = lookup_type(XMLCh2Native(n));
    if (type == -1)
      throw tError("Unknown type \"" + XMLCh2Native(n) + "\".");
    _fs = fs(type);
  }

  void
  tFSCState_fs::leave(tFSCState_edge *target)
  {
    target->set_fs(_fs);
    // cerr << "</" + name()  + ">\n";
  }

  void
  tFSCState_fs::enter(tFSCState_f *source, AttributeList &attrs)
  {
    //// cerr << "<" + name() + ">\n";
    // same as when coming from edge:
    enter((tFSCState_edge*)source, attrs);
  }

  void
  tFSCState_fs::leave(tFSCState_f *target)
  {
    // cerr << "</" + name()  + ">\n";
    target->add_value(_fs);
  }

  void
  tFSCState_fs::set_fs(fs f)
  {
    _fs = f;
  }

  fs
  tFSCState_fs::get_fs()
  {
    return _fs;
  }

  // =====================================================
  // state "f"
  // =====================================================

  tFSCState_f::tFSCState_f()
  : tFSCState("f"), _parent(0)
  { }

  tFSCState_f*
  tFSCState_f::clone() const
  { return new tFSCState_f(*this); }

  void
  tFSCState_f::change_to(tFSCState *target, AttributeList &attrs)
  { target->enter(this, attrs); }

  void
  tFSCState_f::change_from(tFSCState *source)
  { source->leave(this); }

  void
  tFSCState_f::enter(tFSCState_fs *source, AttributeList &attrs)
  {
    // cerr << "<" + name() + ">\n";
    const XMLCh *n = attrs.getValue("name");
    const XMLCh *o = attrs.getValue("org"); // optional
    if (n == 0)
      throw tError("Missing required XML attribute \"name\".");
    if ((o != 0) && (XMLCh2Native(o) != "list"))
      throw tError("Wrong value for XML attribute \"org\".");
    _attr = lookup_attr(XMLCh2Native(n).c_str());
    if (_attr == -1)
      throw tError("Unknown attribute \"" + XMLCh2Native(n) + "\".");
    _is_list = (o != 0);
    _parent = source;
  }

  void
  tFSCState_f::leave(tFSCState_fs *target)
  {
    // cerr << "</" + name()  + ">\n";

    // determine value (an HPSG list if org="list" or a single value, otherwise)
    fs value(BI_TOP);
    if (_is_list) {
      // \todo this code should be put into fs or --better-- some utility class:
      // convert std::list to HPSG list; use as value for _attr:
      list_int *anchor_path = 0;
      fs anchor;
      fs cons = fs(BI_CONS);
      fs nil = fs(BI_NIL);
      for (list<fs>::iterator it = _values.begin(); it != _values.end(); it++) {
        anchor = value.get_path_value(anchor_path);
        value = unify(value, anchor, cons); // ensure cons for the next value
        anchor = value.get_path_value(anchor_path).get_attr_value(BIA_FIRST);
        value = unify(value, anchor, *it); // set current fs in the list
        anchor_path = append(anchor_path, BIA_REST);
      }
      anchor = value.get_path_value(anchor_path);
      value = unify(value, anchor, nil); // close list
      assert(value.valid());
      free_list(anchor_path);
    } else { // no list
      if (!_values.empty())
        value = _values.front();
      else
        return; // no value provided. ignore.
    }

    // unify value into the root fs:
    fs root = _parent->get_fs();
    fs anchor = root.get_attr_value(_attr);
    if (!anchor.valid())
      throw tError("Feature `"+string(attrname[_attr])+"' not allowed here.");
    root = unify(root, anchor, value);
    if (!root.valid())
      throw tError("Invalid value for feature `"+string(attrname[_attr])+"'.");
    _parent->set_fs(root);
  }

  void
  tFSCState_f::add_value(fs value)
  {
    _values.push_back(value);
    if (!_is_list && (_values.size() > 1))
      throw tError("Attempt to reset a value.");
  }

  // =====================================================
  // state "str"
  // =====================================================

  tFSCState_str::tFSCState_str()
  : tFSCState("str")
  { }

  tFSCState_str*
  tFSCState_str::clone() const
  { return new tFSCState_str(*this); }

  void
  tFSCState_str::change_to(tFSCState *target, AttributeList &attrs)
  { target->enter(this, attrs); }

  void
  tFSCState_str::change_from(tFSCState *source)
  { source->leave(this); }

  void
  tFSCState_str::enter(tFSCState_f *source, AttributeList &attrs)
  {
    // cerr << "<" + name() + ">\n";
    _value = BI_STRING;
  }

  void
  tFSCState_str::leave(tFSCState_f *target)
  {
    // cerr << "</" + name()  + ">\n";
    target->add_value(_value);
  }

  void
  tFSCState_str::characters(std::string chars)
  {
    _value = retrieve_string_instance(chars);
  }

}

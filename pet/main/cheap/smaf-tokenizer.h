/* -*- Mode: C++ -*- */
/** \file smaf-tokenizer.h
 * SMAF XML input mode reader
 */

#ifndef _SMAF_TOKENIZER_H
#define _SMAF_TOKENIZER_H

#include "input-modules.h"
#include "pic-handler.h"
#include "xercesc/framework/MemBufInputSource.hpp"
#include "xercesc/framework/LocalFileInputSource.hpp"
#include "xercesc/util/XMLString.hpp"

/** Tokenizer similar to the yy mode tokenizer, using a custom XML DTD giving
 *  more flexibility for the input stage, and also maybe more clarity.
 */
class tSMAFTokenizer : public tTokenizer {
public:
  /** Create a new XML input reader.
   *  \param position_map if \c STANDOFF_COUNTS, an item starting, e.g., at
   *  position zero and ending at position one has length two, not one.
   */
  tSMAFTokenizer(position_map position_mapping = STANDOFF_POINTS)
    : tTokenizer(), _position_mapping(position_mapping) 
  { 
    _chartNodeMax = -1;
  }
  
  virtual ~tSMAFTokenizer() {}

  /** Produce a set of tokens from the given XML input. */
  virtual void tokenize(string input, inp_list &result);
  
  /** A string to describe the module */
  virtual string description() { return "SMAF XML input reader"; }

  /** Return \c true if the position in the returned tokens are counts instead
   *  of positions.
   */
  virtual position_map position_mapping() { return NO_POSITION_MAP ; }
  void set_position_mapping(position_map position_mapping) { 
    _position_mapping = position_mapping ; }

  // node mapping

  void clearNodeMapping();
  int add2nodeMapping(string smafNode);
  string getSmafNode(int chartNode);
  int getChartNode(string smafNode);
  void setNodeMap(string smafNode, int chartNode);

  // id mapping
  void clearIdMapping();
  bool add2idMapping(string id, tInputItem* item);

  bool addNodePoint(int node, int point);

private:
  position_map _position_mapping;

  map<string,int> _nodeMapping;
  int _chartNodeMax;

  // ids
  map<string,tInputItem*> _idMapping;

  // node + point
  map<int,int> _nodePoint;
};

#endif

/* -*- Mode: C++ -*- */
/** \file smaf-tokenizer.h
 * SMAF XML input mode reader

bmw20@cl.cam.ac.uk

 */

#ifndef _SMAF_TOKENIZER_H
#define _SMAF_TOKENIZER_H

#include "input-modules.h"
#include "pic-handler.h"
#include "xercesc/framework/MemBufInputSource.hpp"
#include "xercesc/framework/LocalFileInputSource.hpp"
#include "xercesc/util/XMLString.hpp"

#include <xercesc/dom/DOM.hpp> 

#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <map>

//
// tGMap
//

enum gMapType { sym, str, unk };
typedef std::list<std::string> gMapPath;

class tGMap
{
private:
  std::map<std::string,std::pair<gMapPath,gMapType> > _gMap;
  
public:
  
  // default constructor

  void clear();
  void add(const std::string &name, const gMapPath & path, gMapType type);
  std::pair<gMapPath,gMapType> get(const std::string &name) const;
  void print() const;
};

//
// tSaf
//

struct tSaf
{
  std::string *id, *type, *source, *target, *from, *to;
  std::list<std::string> deps;
  std::map<std::string,std::string> content;

  // local content
  std::map<std::string,std::string> lContent;
  // gMap names in local content
  std::list<std::string> gMapNames;

  void print() const;
};

//
// tSafConfNvpair
//

struct tSafConfNvpair
{
  std::string name;
  std::string* val; // value
  std::string* var; // variable

  void print() const;
};

//
// tSafConfMatch
//

struct tSafConfMatch
{
  std::string type;
  std::list<tSafConfNvpair> restrictions;

  void print() const;
};

//
// tSafConf
//

struct tSafConf
{
  tSafConfMatch match;
  std::list<tSafConfNvpair> nvPairs;

  void print() const;
};

class tSMAFTokenizer : public tTokenizer {
public:

  // CONSTRUCTOR
  //tSMAFTokenizer(position_map position_mapping = STANDOFF_POINTS);
  tSMAFTokenizer();
  
  // DESTRUCTOR
  virtual ~tSMAFTokenizer() {}

  // produce a set of tokens from the SMAF XML input
  virtual void tokenize(std::string input, inp_list &result);
  
  // string to describe module
  virtual std::string description() { return "SMAF XML input reader"; }

  // this class has NO_POSITION_MAP
  virtual position_map position_mapping() { return NO_POSITION_MAP ; }
  //void set_position_mapping(position_map position_mapping) { 
  //  _position_mapping = position_mapping ; }

private:

  //
  // node mapping: smaf node <--> chart node
  //

  std::map<std::string,int> _nodeMapping;
  int _chartNodeMax;

  // clear node mapping
  void clearNodeMapping();
  // add smaf node + chart node
  void setNodeMap(const std::string &smafNode, int chartNode);
  // add smaf node
  int add2nodeMapping(const std::string &smafNode);
  // retrieve chart node
  int getChartNode(const std::string &smafNode);

  //
  // smaf id <--> tInputItem*
  //

  std::map<std::string,tInputItem*> _idMapping;

  // clear id mapping
  void clearIdMapping();
  // add smaf id + tInputItem
  bool add2idMapping(const std::string &id, tInputItem &item);
  // retrieve tInputItem*
  tInputItem* getIdMapVal (const std::string &name);

  //
  // chart node <--> (int) point
  //

  std::map<int,int> _nodePoint;

  // add node + point
  bool addNodePoint(int node, int point);

  // renumber nodes to avoid seg fault
  void renumberNodes(inp_list &result);

  //
  // SMAF XML
  //

  // map XML to DOM
  XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* xml2dom (const std::string &input);
  // for debugging and error messages
  int serializeDOM(const XERCES_CPP_NAMESPACE_QUALIFIER DOMNode &node);
  // get string value from XML attribute
  std::string getAttribute_string(const XERCES_CPP_NAMESPACE_QUALIFIER DOMElement &element, const std::string &name);
  // get string value from XML element (with no daughters)
  std::string getTextContent_string(const XERCES_CPP_NAMESPACE_QUALIFIER DOMElement &element);
   // process 'lattice' element
  void processLatticeElt(const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument &dom);

  // process 'edge' elements
  void processEdges(const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument &dom, 
                    std::list<tSaf*> &tEdges, 
                    std::list<tSaf*> &mEdges);

  // map 'edge' to saf
  tSaf* getSaf(const XERCES_CPP_NAMESPACE_QUALIFIER DOMElement &element);

  //
  // SAF
  //

  // items in current saf lattice
  std::list<tSaf*> _mySafs;
  void clearMySafs();

  // saf config, defines mapping into local content
  std::list<tSafConf> _safConfs;

  // gMap conf ("define" lines in saf conf)
  tGMap _gMap;

  // read file and set _safConfs
  void processSafConfFile(const char *filename);
  void processSafConfDefault();
  void processSafConfIstream(std::istream *filename);
  // extract saf conf from line
  tSafConf* processSafConfLine(const std::string &line);
  // extract gMap setting from line
  bool processSafConfDefine(const std::string &line);
  // extract type + restrictions
  tSafConfMatch* processSafConfLinePre(const std::string &pre);
  // extract name=val or name=var pair
  tSafConfNvpair* extractNvpair(const std::string &str);

  // use saf morph annot to update input item set 
  void processSafMorphEdge(tSaf &saf, inp_list &items);
  
  // use _safConfs to set local content of saf
  void setLocalContent(tSaf &saf);
  // update local content wrt single saf conf
  bool updateLocalContent(std::map<std::string,std::string> &localContent, 
                          const tSafConf &safConf, 
                          const tSaf &saf);

  // retrieve saf by saf id
  tSaf* getSafById(const std::string &dep) const;

  // true if type + restrictions match
  bool match(tSafConfMatch match, tSaf saf);
  // resolve variable into a value
  std::string resolve(const std::string &var, const tSaf &saf);
  // spc separated concatentation of result of resolve variable wrt each dep in turn
  std::string resolveDeps(const std::string &var, const tSaf &saf);

  // retrieve content by name
  std::string getContentVal (const std::map<std::string,std::string> &content, const std::string &name);

  // retrieve gMapNames from local content
  std::list<std::string> getGMapNames (const std::map<std::string,std::string> &lContent);
  // apply gMap settings to modlist
  bool processAllGMapContent(const tSaf &saf, modlist &fsmods);
  // apply single gMap setting
  bool processGMapContent(const std::string &name, const tSaf &saf, modlist &fsmods);

  //
  // tInputItem
  //
  
  // map saf to tInputItem
  tInputItem* getInputItemFromSaf(const tSaf &saf);
  // retrieve input item by id
  tInputItem* getItemById(const std::string &id, const inp_list &items);

};
  
#endif

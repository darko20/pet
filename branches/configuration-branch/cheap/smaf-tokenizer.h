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

//
// tGMap
//

enum gMapType { sym, str, unk };
typedef list<string> gMapPath;

class tGMap
{
private:
  map<string,pair<gMapPath,gMapType> > _gMap;
  
public:
  
  // default constructor

  void clear();
  void add(const string &name, const gMapPath & path, gMapType type);
  pair<gMapPath,gMapType> get(const string &name) const;
  void print() const;
};

//
// tSaf
//

struct tSaf
{
  string *id, *type, *source, *target, *from, *to;
  list<string> deps;
  map<string,string> content;

  // local content
  map<string,string> lContent;
  // gMap names in local content
  list<string> gMapNames;

  void print() const;
};

//
// tSafConfNvpair
//

struct tSafConfNvpair
{
  string name;
  string* val; // value
  string* var; // variable

  void print() const;
};

//
// tSafConfMatch
//

struct tSafConfMatch
{
  string type;
  list<tSafConfNvpair> restrictions;

  void print() const;
};

//
// tSafConf
//

struct tSafConf
{
  tSafConfMatch match;
  list<tSafConfNvpair> nvPairs;

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
  virtual void tokenize(string input, inp_list &result);
  
  // string to describe module
  virtual string description() { return "SMAF XML input reader"; }

  // this class has NO_POSITION_MAP
  virtual position_map position_mapping() { return NO_POSITION_MAP ; }
  //void set_position_mapping(position_map position_mapping) { 
  //  _position_mapping = position_mapping ; }

private:

  //
  // node mapping: smaf node <--> chart node
  //

  map<string,int> _nodeMapping;
  int _chartNodeMax;

  // clear node mapping
  void clearNodeMapping();
  // add smaf node + chart node
  void setNodeMap(const string &smafNode, int chartNode);
  // add smaf node
  int add2nodeMapping(const string &smafNode);
  // retrieve chart node
  int getChartNode(const string &smafNode);

  //
  // smaf id <--> tInputItem*
  //

  map<string,tInputItem*> _idMapping;

  // clear id mapping
  void clearIdMapping();
  // add smaf id + tInputItem
  bool add2idMapping(const string &id, tInputItem &item);
  // retrieve tInputItem*
  tInputItem* getIdMapVal (const string &name);

  //
  // chart node <--> (int) point
  //

  map<int,int> _nodePoint;

  // add node + point
  bool addNodePoint(int node, int point);

  // renumber nodes to avoid seg fault
  void renumberNodes(inp_list &result);

  //
  // SMAF XML
  //

  // map XML to DOM
  XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* xml2dom (const string &input);
  // for debugging and error messages
  int serializeDOM(const XERCES_CPP_NAMESPACE_QUALIFIER DOMNode &node);
  // get string value from XML attribute
  string getAttribute_string(const XERCES_CPP_NAMESPACE_QUALIFIER DOMElement &element, const string &name);
  // get string value from XML element (with no daughters)
  string getTextContent_string(const XERCES_CPP_NAMESPACE_QUALIFIER DOMElement &element);
   // process 'lattice' element
  void processLatticeElt(const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument &dom);

  // process 'edge' elements
  void processEdges(const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument &dom, 
		    list<tSaf*> &tEdges, 
		    list<tSaf*> &mEdges);

  // map 'edge' to saf
  tSaf* getSaf(const XERCES_CPP_NAMESPACE_QUALIFIER DOMElement &element);

  //
  // SAF
  //

  // items in current saf lattice
  list<tSaf*> _mySafs;
  void clearMySafs();

  // saf config, defines mapping into local content
  list<tSafConf> _safConfs;

  // gMap conf ("define" lines in saf conf)
  tGMap _gMap;

  // read file and set _safConfs
  void processSafConfFile(const string &filename);
  // extract saf conf from line
  tSafConf* processSafConfLine(const string &line);
  // extract gMap setting from line
  bool processSafConfDefine(const string &line);
  // extract type + restrictions
  tSafConfMatch* processSafConfLinePre(const string &pre);
  // extract name=val or name=var pair
  tSafConfNvpair* extractNvpair(const string &str);

  // use saf morph annot to update input item set 
  void processSafMorphEdge(tSaf &saf, inp_list &items);
  
  // use _safConfs to set local content of saf
  void setLocalContent(tSaf &saf);
  // update local content wrt single saf conf
  bool updateLocalContent(map<string,string> &localContent, 
			  const tSafConf &safConf, 
			  const tSaf &saf);

  // retrieve saf by saf id
  tSaf* getSafById(const string &dep) const;

  // true if type + restrictions match
  bool match(tSafConfMatch match, tSaf saf);
  // resolve variable into a value
  string resolve(const string &var, const tSaf &saf);
  // spc separated concatentation of result of resolve variable wrt each dep in turn
  string resolveDeps(const string &var, const tSaf &saf);

  // retrieve content by name
  string getContentVal (const map<string,string> &content, const string &name);

  // retrieve gMapNames from local content
  list<string> getGMapNames (const map<string,string> &lContent);
  // apply gMap settings to modlist
  bool processAllGMapContent(const tSaf &saf, modlist &fsmods);
  // apply single gMap setting
  bool processGMapContent(const string &name, const tSaf &saf, modlist &fsmods);

  //
  // tInputItem
  //
  
  // map saf to tInputItem
  tInputItem* getInputItemFromSaf(const tSaf &saf);
  // retrieve input item by id
  tInputItem* getItemById(const string &id, const inp_list &items);

};
  
#endif

/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 *     2004 Bernd Kiefer bk@dfki.de
 *     2006-2007 Ben Waldron bmw20@cl.cam.ac.uk
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

#include "smaf-tokenizer.h"
#include "cheap.h"

#include <fstream>
using namespace std;

#include <xercesc/dom/DOM.hpp> 
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>

ostream &operator<<(ostream& os, const list<string> &s);

XERCES_CPP_NAMESPACE_USE

// simple misc utilities

void downcase(string &s) {
  unsigned int i;

  for (i=0; i!=s.length(); i++)
    {
      if ((unsigned char)s[i] < 127)
        {
          s[i]=tolower(s[i]);
        }
    }

  return;
}

void upcase(string &s) {
  unsigned int i;

  for (i=0; i!=s.length(); i++)
    {
      if ((unsigned char)s[i] < 127)
        {
          s[i]=toupper(s[i]);
        }
    }

  return;
}

list<string> splitOnSpc(string str)
{
  list<string> out;

  string str2="";
  for (string::iterator it=str.begin(); it != str.end(); it++)
    {
      if (*it==' ')
        {
          if (str2!="")
            out.push_back(str2);
          str2="";
        }
      else
        {
          str2+=*it;
        }
    }
  if (str2!="")
    out.push_back(str2);

  return out;
}

//
// whitespace utilities
//

// true if: space, tab, newline
bool whitespace(char c)
{
  if (c==' ' or c=='\t' or c=='\n')
    return true;
  else
    return false;
}

// out: true if str contains only whitespace (see above)
bool whitespace(const string &str)
  {
    for (string::const_iterator it = str.begin(); it != str.end(); it++)
    {
      if (not(whitespace(*it)))
        return false;
    }
    return true;
  }

// walk forwards from position i in str til we hit non-whitespace
int skipWhitespace(const string &str, unsigned int i)
{
  for (; i<str.size(); i++)
    {
      if (!whitespace(str[i]))
        return i;
    }
  return string::npos;
}

// walk forwards from position i in str til we hit whitespace
int findWhitespace(const string &str, unsigned int i)
{
  for (; i<str.size(); i++)
    {
      if (whitespace(str[i]))
        return i;
    }
  return string::npos;
}

//
// CONSTRUCTOR
//

//tSMAFTokenizer::tSMAFTokenizer(position_map position_mapping)
//  : tTokenizer(), _position_mapping(position_mapping) 
//{ 
//  // initialise chartNodeMax
//  _chartNodeMax = -1;
//  // load saf conf
//  char* safConfFilename = cheap_settings->value("smaf-conf");
//  _safConfs=processSafConfFile(safConfFilename);
//}

// saf conf file is set using 'smaf-conf' in cheap settings
tSMAFTokenizer::tSMAFTokenizer()
  : tTokenizer() 
{ 
  // initialise chartNodeMax
  _chartNodeMax = -1;
  // load saf conf
  char* safConfFilename = cheap_settings->value("smaf-conf");
  if (safConfFilename==NULL) 
    {
      cerr << endl << "WARNING: please set 'smaf-conf' in PET config file" << endl;
      processSafConfDefault();
    }
  else
    processSafConfFile(safConfFilename);
}


// map a string to a DOM:
//
// case (i): string is @PATHNAME
// case (ii): string is XML
//
DOMDocument* tSMAFTokenizer::xml2dom (const string &input) {
  InputSource *xmlinput;

  if (input.compare(0, 1, "@") == 0)
  // case (i)
    {  
      XMLCh * XMLFilename = XMLString::transcode((input.substr(1,100)).c_str());
      xmlinput = new LocalFileInputSource(XMLFilename);
    }
  else
    // case (ii)
    xmlinput = new MemBufInputSource((const XMLByte *) input.c_str(), input.length(), "STDIN");
  
  // initialise XML machinery
  try {
    XMLPlatformUtils::Initialize();
  }
  catch (const XMLException& toCatch) {
    char* message = XMLString::transcode(toCatch.getMessage());
    cerr << "ERROR: problem during XMLPlatformUtils::Initialize() :\n"
         << message << "\n";
    XMLString::release(&message);
    return NULL;
  }
  
  // get a parser
  XercesDOMParser* parser = new XercesDOMParser();
  parser->setValidationScheme(XercesDOMParser::Val_Always);    // optional.
  parser->setDoNamespaces(true);    // optional
  
  ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
  parser->setErrorHandler(errHandler);

  // parse the XML
  try {
    parser->parse(*xmlinput);
  }
  catch (const XMLException& toCatch) {
    char* message = XMLString::transcode(toCatch.getMessage());
    cerr << "ERROR: [XMLException] Exception message is: \n"
         << message << "\n";
    XMLString::release(&message);
    return NULL;
  }
  catch (const DOMException& toCatch) {
    char* message = XMLString::transcode(toCatch.msg);
    cerr << "ERROR: [DOMException] Exception message is: \n"
         << message << "\n";
    XMLString::release(&message);
    return NULL;
  }
  catch (const SAXParseException& toCatch) {
    char* message = XMLString::transcode(toCatch.getMessage());
    cerr << "ERROR: [SAXParseException] Exception message is: \n"
         << message << "\n";
    XMLString::release(&message);
    return NULL;
  }
  catch (...) {
    cerr << "ERROR: Unexpected Exception \n" ;
    return NULL;
  }

  // adopt document so it won't vanish
  DOMDocument* doc = parser->adoptDocument();

  delete parser;
  delete errHandler;

  XMLPlatformUtils::Terminate();

  // return the DOM doc
  return doc;
}

// this is a debugging aid
int tSMAFTokenizer::serializeDOM(const DOMNode &node) {
  
  XMLCh tempStr[100];
  XMLString::transcode("LS", tempStr, 99);
  DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(tempStr);
  DOMWriter* theSerializer = ((DOMImplementationLS*)impl)->createDOMWriter();
  
  // optionally you can set some features on this serializer
  if (theSerializer->canSetFeature(XMLUni::fgDOMWRTDiscardDefaultContent, true))
    theSerializer->setFeature(XMLUni::fgDOMWRTDiscardDefaultContent, true);
  
  if (theSerializer->canSetFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true))
    theSerializer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true);
  
  // StdOutFormatTarget prints the resultant XML stream
  // to stdout once it receives any thing from the serializer.
  XMLFormatTarget *myFormTarget = new StdOutFormatTarget();
  
  try {
    // do the serialization through DOMWriter::writeNode();
    theSerializer->writeNode(myFormTarget, node);
  }
  catch (const XMLException& toCatch) {
    char* message = XMLString::transcode(toCatch.getMessage());
    cerr << "ERROR: [XMLException] Exception message is: \n"
         << message << "\n";
    XMLString::release(&message);
    return -1;
  }
  catch (const DOMException& toCatch) {
    char* message = XMLString::transcode(toCatch.msg);
    cerr << "ERROR: [DOMException] Exception message is: \n"
         << message << "\n";
    XMLString::release(&message);
    return -1;
  }
  catch (...) {
    cerr << "ERROR: Unexpected Exception \n" ;
    return -1;
  }
  
  theSerializer->release();
  delete myFormTarget;
  cerr << endl;
  return 0;
}

// return value of attribute 'name', or ""
string tSMAFTokenizer::getAttribute_string(const DOMElement &element, const string &name){
  const XMLCh *xval =  element.getAttribute(XMLString::transcode(name.c_str()));
  string val = Conv->convert((UChar *)xval, XMLString::stringLen(xval));
  return val;
}

// return text content if element has only text children, or ""
string tSMAFTokenizer::getTextContent_string(const DOMElement &element)
{
  // all children should be text nodes
  DOMNodeList *nodes=element.getChildNodes();
  const XMLSize_t nodeCount = nodes->getLength() ;

  // iterate through slots, and abort if non-text node daughter
  for( XMLSize_t ix = 0 ; ix < nodeCount ; ++ix )
    {
      DOMNode* node = nodes->item( ix ) ;
      //      if (!node->getNodeType()==TEXT_NODE)
      //serializeDOM(node); 
      if (!(node->getNodeType()==3))
        return "";
    }

  // finally, get the text content
  const XMLCh *xval =  element.getTextContent();
  string val = Conv->convert((UChar *)xval, XMLString::stringLen(xval));
  return val;
}

// return upcased dotted list
string dottedString(const list<string> &s)
{
  string out;
  bool flag=true;
  for (list<string>::const_iterator it=s.begin(); it!=s.end(); it++)
    // construct dotted list
    {
      if (flag)
        {
          out += *it;
          flag=false;
        }
      else
        out += "." + *it;
    }
  // upcase result
  upcase(out);
  //cout << "DOTTED STRING: " << out << endl;
  return out;
}

// update fsmods input by applying single gMap from local content
bool tSMAFTokenizer::processGMapContent(const string &name, const tSaf &saf, modlist &fsmods)
{
  // sanity check
  if (name.substr(0,5)!="gMap.")
    {
      cerr << "WARNING: expected name in call to processGMapContent to start \"gMap.\"" << endl;
      return false;
    }
  // get string after the dot
  string gMapName=name.substr(5);
  // get value from local content
  string val = getContentVal(saf.lContent,name);
  // get path and type from _gMap
  pair<gMapPath,gMapType> pt=_gMap.get(gMapName);
  gMapPath path=pt.first;
  gMapType type=pt.second;
  if (type==unk)
    {
      cerr << "WARNING: no such gMap name: " << name << endl;
      return false;
    }
  // constructed dotted path
  const char* dPath=dottedString(path).c_str();
  // convert val to 'string' if required
  if (type==str)
    val = '"' + val + '"';
  // add to fsmods
  fsmods.push_back(pair<string, type_t>(dPath, retrieve_type(val)));

  return true;
}

// update fsmods by applying all gMaps from local content
// returns false if something went wrong
bool tSMAFTokenizer::processAllGMapContent(const tSaf &saf, modlist &fsmods)
{
  bool flag=true;
  for (list<string>::const_iterator it=saf.gMapNames.begin(); it!=saf.gMapNames.end(); it++)
    // process each gMapName
    {
      if (processGMapContent(*it,saf,fsmods)==false)
        flag=false;
    }
  return flag;
}

// map a SMAF token edge to an internal tInputItem
tInputItem* tSMAFTokenizer::getInputItemFromSaf(const tSaf &saf){

  // EDGETYPE
  string edgeType = getContentVal(saf.lContent,"edgeType");
  if (not(edgeType=="tok" or edgeType=="tok+morph"))
    {
      throw tError("expected tok or tok+morph edgeType");
    }

  // SURFACE
  string surface = getContentVal(saf.lContent,"tokenStr");
  // give up if no 'surface'
  if (surface=="")
    {
      cerr << "WARNING: no surface" << endl;
      return NULL;
    }
  // convert surface to lower-case
  downcase(surface);

  // get fsmods
  modlist fsmods = modlist() ;
  processAllGMapContent(saf,fsmods);

  // retrieve remaining attribs
  string* id = saf.id;

  // SOURCE
  string* sourceSmaf = saf.source;
  // give up if no source
  if (sourceSmaf==NULL)
    {
      cerr << "WARNING: no source" << endl;
      return NULL;
    }
  int source = add2nodeMapping(*sourceSmaf);

  // TARGET
  string* targetSmaf = saf.target;
  // give up if no target
  if (targetSmaf==NULL)
    {
      cerr << "WARNING: no target" << endl;
      return NULL;
    }
  int target = add2nodeMapping(*targetSmaf);

  // FROM
  int start;
  if (saf.from==NULL or EOF == sscanf(saf.from->c_str(), "%d", &start))
    {
      cerr << "WARNING: unable to extract 'from'" << endl;
      return NULL;
    }

  // TO
  int end;
  if (saf.to==NULL or EOF == sscanf(saf.to->c_str(), "%d", &end))
    {
      cerr << "WARNING: unable to extract 'to'" << endl;
      return NULL;
    }

  // create tInputItem
  tInputItem* item = new tInputItem(*id, source, target, start, end, surface, ""
                                    , tPaths()
                                    , WORD_TOKEN_CLASS, fsmods);

  // POS
  string pos = getContentVal(saf.lContent,"pos");
  {
    postags tags;
    tags.add(pos);
    item->set_in_postags(tags);    
  }
  
  // give up if 'id' is duplicate
  bool flag = add2idMapping(*id,*item);
  if (!flag)
    return NULL;

  return item;
}

// return item matching (external) id
tInputItem* tSMAFTokenizer::getItemById(const string &id, const inp_list &items)
{
  for(list<tInputItem *>::const_iterator it = items.begin()
        ; it != items.end(); it++) {
      if (((*it)->external_id())==id)
        {
          return *it;
        }
    }
  return NULL;
}

void tSMAFTokenizer::processSafMorphEdge(tSaf &saf, inp_list &items){

  // EDGETYPE
  string edgeType = saf.lContent["edgeType"];
  if (not(edgeType=="morph" or edgeType=="tok+morph"))
    {
      throw tError("expected morph or tok+morph edgeType");
    }

  // POS
  string tag = saf.lContent["pos"];
  if (tag=="")
    {
      cerr << "WARNING: unable to extract 'pos':";
      saf.print();
      return;
    }

  // (single) DEP
  list<string> deps = saf.deps;
  if (deps.size()==0)
    {
      cerr << "WARNING: no deps";
      saf.print();
      return;
    }
  if (deps.size()>1)
    {
      cerr << "WARNING: we don't currently allow edges with multiple deps";
      saf.print();
      return;
    }
  string dep=*(deps.begin());

  // look up DEP item
  tInputItem* item = getItemById(dep, items);

  if (item!=NULL)
    {
      // add POS to item
      postags pos;
      pos.add(tag);
      item->set_in_postags(pos);

      // add CARG to item
      {

        // get fsmods
        modlist fsmods = modlist() ;
        processAllGMapContent(saf,fsmods);

        // set CARG
        item->set_mods(fsmods);
      }
    }

  return;
}

// in: dom for SMAF XML
// side effects: setNodeMap(init,...) and setNodeMap(final,...)
void tSMAFTokenizer::processLatticeElt(const DOMDocument &dom)
{
  // get 'lattice' element
  XMLCh* xstr = XMLString::transcode("lattice");
  DOMNodeList* elts = dom.getElementsByTagName(xstr);
  XMLString::release(&xstr);
  const XMLSize_t nodeCount = elts->getLength() ;
  if (nodeCount!=1)
    {
      cerr << "WARNING: multiple lattice elements found" << endl;
      return;
    }
  DOMNode* lNode = elts->item( 0 ) ;
  DOMElement* element = dynamic_cast< xercesc::DOMElement* >( lNode );

  // INIT
  string init = getAttribute_string(*element,"init");
  if (init=="")
    {
      cerr << "ERROR: no 'init' value found in lattice" << endl;
      return;
    }
  else
    {
      //
      setNodeMap(init,0);
      _chartNodeMax=0;
    }
  
  // FINAL
  string final = getAttribute_string(*element,"final");
  if (final=="")
    {
      cerr << "ERROR: no 'final' value found in lattice" << endl;
      return;
    }
  else
    {
      // we will set final node to number of edges
      XMLCh* xstr2 = XMLString::transcode("edge");
      DOMNodeList* welts2 = dom.getElementsByTagName(xstr2);
      XMLString::release(&xstr2);
      const XMLSize_t nodeCount = welts2->getLength() ;
      
      //
      setNodeMap(final,nodeCount);
    }
}

// in: DOM doc
// out: token edges, morph edges
void tSMAFTokenizer::processEdges(const DOMDocument &dom, list<tSaf*> &tEdges, list<tSaf*> &mEdges)
{
  // get all 'edge' elements
  XMLCh* xstr = XMLString::transcode("edge");
  DOMNodeList* welts = dom.getElementsByTagName(xstr);
  XMLString::release(&xstr);
  const XMLSize_t nodeCount = welts->getLength() ;
 
  for( XMLSize_t ix = 0 ; ix < nodeCount ; ++ix )
    // walk thru edge elements
    {
      // get element
      DOMNode* wNode = welts->item( ix ) ;
      DOMElement* element = dynamic_cast< xercesc::DOMElement* >( wNode );

      tSaf* saf=getSaf(*element);
      if (saf!=NULL and saf->type!=NULL)
        {
          // store
          //cerr << "ADD MYSAFS" << endl;
          _mySafs.push_back(saf);

        }
      else
        {
          // skip malformed edge
          cerr << "WARNING: XML edge is malformed:" << endl;
          serializeDOM(*wNode);
          cerr << endl;
        }
    }

  // LOCAL CONTENT
  for (list<tSaf*>::iterator it=_mySafs.begin(); it!=_mySafs.end(); it++)
    {
      // set local content
      tSaf* saf=*it;
      setLocalContent(*saf);

      // set gMapNames
      saf->gMapNames = getGMapNames(saf->lContent);

      //cerr << "GMAP NAMES=" <<saf-> gMapNames << endl;

      // update morph/tok lists
      string edgeType = getContentVal(saf->lContent,"edgeType");
      // morph edge
      if (edgeType=="morph" or edgeType=="tok+morph")
        mEdges.push_back(saf);
      // tok edge
      if (edgeType=="tok" or edgeType=="tok+morph")
        tEdges.push_back(saf);
    }
}

// release safs
void tSMAFTokenizer::clearMySafs()
{
  //cerr << "* CLEAR MYSAFS *" << endl; 
  for (list<tSaf*>::iterator it=_mySafs.begin(); it!=_mySafs.end(); it++)
    {
      // release individual saf
      delete *it;
    }
  // clear list of saf *
  _mySafs.clear();
}

void tSMAFTokenizer::renumberNodes(inp_list &result)
{
  // rename nodes in forward seq to avoid seg fault later (!)
  map<int,int> nodePoint;
  // construct node point map
  for(inp_iterator it = result.begin(); it != result.end(); it++) 
    {
      int from=(*it)->start(), to=(*it)->end();
      int cfrom=(*it)->startposition(), cto=(*it)->endposition();

      // process 'from' node
      if (nodePoint[from]<cfrom)
        nodePoint[from]=cfrom;
      // process 'to' node
      if (nodePoint[to]<cto)
        nodePoint[to]=cto;
    }
  // construct sorted point node map
  map<int,int> pointNode;
  for (map<int,int>::iterator it = nodePoint.begin(); it != nodePoint.end(); it++)
    {
      int node=it->first;
      int point=it->second;
      pointNode[point]=node;
    }

  // construct node rename mapping
  map<int,int> nodeRename;
  int i=0;
  for (map<int,int>::iterator it = pointNode.begin(); it != pointNode.end(); it++)
    {
      int node=it->second;

      nodeRename[node]=i;
      i++;
    }

  // finally, rename the nodes
  for(inp_iterator it = result.begin(); it != result.end(); it++) 
    {
      // retrieve new values
      int from=(*it)->start(), to=(*it)->end();
      // set new values
      (*it)->set_start(nodeRename[from]);
      (*it)->set_end(nodeRename[to]);

    }
}

// map SMAF XML to set of tokens=input items
void tSMAFTokenizer::tokenize(string input, inp_list &result) {

  // initialise mappings
  clearIdMapping();
  clearNodeMapping();

  // map XML input to DOM
  DOMDocument* dom = xml2dom(input);

  // abort if something went wrong
  if (dom==NULL)
    return;

  // process lattice element
  processLatticeElt(*dom);

  if (dom!=NULL)
    {
      list<tSaf*> tokEdges, morphEdges;
      processEdges(*dom,tokEdges,morphEdges);

      // tokEdges
      for (list<tSaf*>::iterator it=tokEdges.begin(); it!=tokEdges.end(); it++)
        {
          tSaf *saf = *it;
          // get new tInputItem
          tInputItem* item = getInputItemFromSaf(*saf);

          if (item==NULL)
            {
              cerr << "WARNING: unable to construct chart edge from" << endl;
              saf->print();
            }
          else
            result.push_back(item);

        }

      // morphEdges
      for (list<tSaf*>::iterator it=morphEdges.begin(); it!=morphEdges.end(); it++)
        {
          tSaf *saf = *it;
          // update tInputItems
          processSafMorphEdge(*saf,result);
        }
    }
  
  // sanity check
  // to avoid seg fault later (why??) there must be an edge with start=0
  bool flag=false;
  for(inp_iterator it = result.begin(); it != result.end(); it++) 
    {
      if ((*it)->start()==0)
        flag=true;
    }
  if (!flag)
    {
      cerr << "ERROR: cannot find 'init' node in input lattice" << endl;
      result.clear();
      clearMySafs(); // release safs
      return;
    }
  
  // renumber nodes in fwd seq to avoid seg fault later
  renumberNodes(result);

  clearMySafs(); // release safs
  return;
}

tSaf* tSMAFTokenizer::getSaf(const DOMElement &element)
{
  tSaf* saf=new tSaf();

  // ID
  saf->id= new string(getAttribute_string(element,"id"));
  // TYPE
  saf->type= new string(getAttribute_string(element,"type"));
  // SOURCE
  saf->source= new string(getAttribute_string(element,"source"));
  // TARGET
  saf->target= new string(getAttribute_string(element,"target"));
  // FROM
  saf->from= new string(getAttribute_string(element,"cfrom"));
  // TO
  saf->to=new string(getAttribute_string(element,"cto"));
  // DEPS
  saf->deps=splitOnSpc(getAttribute_string(element,"deps"));
  // CONTENT
  // get all <slot> elements
  XMLCh* xstr = XMLString::transcode("slot");
  DOMNodeList* welts = element.getElementsByTagName(xstr);
  XMLString::release(&xstr);

  const XMLSize_t nodeCount = welts->getLength() ;
  for( XMLSize_t ix = 0 ; ix < nodeCount ; ++ix )
    // walk thru slots
    {
      DOMNode* wNode = welts->item( ix ) ;
      // get slot element
      DOMElement* slot = dynamic_cast< xercesc::DOMElement* >( wNode );
      // get name and val
      string name = getAttribute_string(*slot,"name");
      string val = getTextContent_string(*slot);
      // add to saf content
      if (name!="")
        saf->content[name]=val;
      else
        cerr << "WARNING: ignoring slot with name=\"\"" << endl;
    }
  // get simple content if no slots
  if (nodeCount==0)
    saf->content[""]=getTextContent_string(element);
  
  return saf;
}

// in: saf confs, saf
// out: local content
void tSMAFTokenizer::setLocalContent(tSaf &saf)
{
  map<string,string> localContent;

  for (list<tSafConf>::const_iterator it=_safConfs.begin(); it!=_safConfs.end(); it++)
    // walk thru safConf
    {
      tSafConf safConf=*it;
      updateLocalContent(localContent,safConf,saf);
    }
  saf.lContent=localContent;
}

// in: local content, saf conf, saf
// out: true if local content altered
bool tSMAFTokenizer::updateLocalContent(map<string,string> &localContent, const tSafConf &safConf, const tSaf &saf)
{
  if (match(safConf.match, saf))
    // we have a match
    {
      //safConf.print();
      for (list<tSafConfNvpair>::const_iterator it=safConf.nvPairs.begin(); 
           it!=safConf.nvPairs.end(); 
           it++)
        // walk thru nvPairs of saf conf
        {
          if (it->val!=NULL)
            // constant
            {
              localContent[it->name]=*(it->val);
            }
          else
            // resolve variable
            {
              string var=*(it->var);
              localContent[it->name]=resolve(var ,saf);
            }
        }
      return true;
    }
  else
    return false;
}

// in: content, name
// out: val or ""
string tSMAFTokenizer::getContentVal (const map<string,string> &content, const string &name)
{
     map<string, string>::const_iterator it = content.find(name);
     return (it != content.end()) ? it->second : "";
} 

// reimplement_me
tSaf* tSMAFTokenizer::getSafById(const string &dep) const
{
  //cerr << "MYSAFS size=" << _mySafs.size() << endl;
  for (list<tSaf*>::const_iterator it=_mySafs.begin(); it!=_mySafs.end(); it++)
    {
      tSaf *saf=*it;
      string id=*(saf->id);

      //cerr << "checking ID=" << id << " DEP=" << dep << endl;

      if (id==dep)
        return *it;
    }
  return NULL;
}

// concatenate (w/ spc between) result of resolving var wrt each dep
string tSMAFTokenizer::resolveDeps(const string &var, const tSaf &saf)
{
  string out;
  //cerr << "RESOLVE DEPS var=" << var << endl;
  //saf.print();

  bool flag=true;
  for (list<string>::const_iterator it=saf.deps.begin(); it!=saf.deps.end(); it++)
    {
      string dep=*it;

      // look up DEP item
      tSaf *depSaf = getSafById(dep);
      if (depSaf==NULL)
        {
          cerr << "WARNING: unable to resolve saf id=\"" << dep << "\"" << endl;
          return "";
        }
      if (flag)
        flag=false;
      else
        out += ' ';
      out += resolve(var,*depSaf);
    }
  
  //cerr << "OUT=" << out << endl;
  return out;
}

// in: var, saf
// out: resolved var
string tSMAFTokenizer::resolve(const string &var, const tSaf &saf)
{
  if (var=="content")
    // SIMPLE CONTENT
    return getContentVal(saf.content,"");
  else if (var.substr(0,8)=="content.")
    // SLOT CONTENT
    return getContentVal(saf.content,var.substr(8));
  else if (var.substr(0,5)=="deps.")
    // DEPS CONTENT
    return resolveDeps(var.substr(5),saf);
  else
    // unhandled
    {
      cerr << "WARNING: unknown saf conf variable: \"" << var << "\"" << endl;
      return "";
    }
}

// in: match, saf
// out: true if match
bool tSMAFTokenizer::match(tSafConfMatch match, tSaf saf)
{
  // check TYPE
  if (match.type!=*saf.type)
    return false;
  
  for (list<tSafConfNvpair>::iterator it=match.restrictions.begin();
       it!=match.restrictions.end();
       it++)
    // check each RESTRICTION
    {
      if (it->val==NULL)
        // corrupt struct
        {
          cerr << "WARNING: corrupted 'val' in tSafConfNvpair" << endl;
          it->print();
          return false;
        }
      string val=*(it->val);
      if (getContentVal(saf.content,it->name)!=val)
        // failed match
        return false;
    }

  return true;
}

//
// node mapping functions (SAF node -> chart node)
//

void tSMAFTokenizer::clearNodeMapping()
{
  _nodeMapping.clear();
}

// in: saf node
// returns chart node
int tSMAFTokenizer::add2nodeMapping(const string &smafNode)
{
  int node = _nodeMapping[smafNode]-1;
  if (node==-1)
    {
      int chartNode = ++_chartNodeMax;
      _nodeMapping[smafNode] = chartNode+1;
      node = chartNode;
    }
  return node;
}

// in: saf node, chart node
void  tSMAFTokenizer::setNodeMap(const string &smafNode, int chartNode)
{
  _nodeMapping[smafNode] = chartNode+1;
}

// in: saf node
// out: chart node
int tSMAFTokenizer::getChartNode(const string &smafNode)
{
  int chartNode = _nodeMapping[smafNode];
  return chartNode-1;
}

//
// id mapping functions (saf id -> tInputItem*)
//

tInputItem* tSMAFTokenizer::getIdMapVal (const string &name)
{
  map<string,tInputItem*>::const_iterator it = _idMapping.find(name);
  return (it != _idMapping.end()) ? it->second : NULL;
} 

void tSMAFTokenizer::clearIdMapping()
{
  _idMapping.clear();
}

// in: id, tInputItem*
// out: true, false=duplicate id
bool tSMAFTokenizer::add2idMapping(const string &id, tInputItem &item)
{
  if (getIdMapVal(id)==NULL)
    {
      _idMapping[id] = &item;
      return true;
    }
  else
    {
      cerr << "WARNING: Duplicate id in SMAF input: " << id << endl;
      return false;
    }
}

//
// SAF CONF
//

// in: filename
// out: list of saf conf
void tSMAFTokenizer::processSafConfFile(const char* filename)
  {
    // message to cerr
    cerr << endl << "reading SMAF conf '" << filename << "'..." << endl;
    // open file
    ifstream ff(filename);
    // process istream
    istream* istr =  &ff;
    processSafConfIstream(istr);
  }

// out: list of saf conf (defaults)
void tSMAFTokenizer::processSafConfDefault()
  {
    // default SMAF config text
    string default_config = "define gMap.carg (synsem lkeys keyrel carg) STRING\n\
token.[] -> edgeType='tok' tokenStr=content\n\
wordForm.[] -> edgeType='morph' stem=content.stem partialTree=content.partial-tree\n\
ersatz.[] -> edgeType='tok+morph' stem=content.name tokenStr=content.name gMap.carg=content.surface inject='t' analyseMorph='t'\n\
pos.[] -> edgeType='morph' fallback='' pos=content.tag gMap.carg=deps.content";

    // message to cerr
    cerr << "WARNING: falling back to built-in default SMAF config: " << endl << endl;
    cerr << default_config << endl << endl;

    // process istream
    std::stringstream s( default_config );
    istream* istr =  &s;
    processSafConfIstream(istr);
  }

// in: istream to SMAF config text
void tSMAFTokenizer::processSafConfIstream(istream* istr)
  {
    char ch;
    string line="";
    UnicodeString u_line;

    // walk thru chars
    //    while (ff.get(ch))
    while (istr->get(ch))
      {
        if (ch=='\n')
          // end of line
          {
            // extract safConf from line
            tSafConf* safConf=processSafConfLine(line);
            if (safConf!=NULL)
              // store it
              _safConfs.push_back(*safConf);
            
            u_line = Conv->convert(line); // do Unicode later
            // reset line
            line="";
          }
        else
          // collect char
          line += ch;
      }
    // extract safConf from line
    tSafConf* safConf=processSafConfLine(line);
    if (safConf!=NULL)
      _safConfs.push_back(*safConf);

    u_line = Conv->convert(line); // do Unicode later
    
    //cerr << "GMAP:" << endl;
    //_gMap.print();
  }

// in: saf conf line
// out: saf conf * or NULL
tSafConf* tSMAFTokenizer::processSafConfLine(const string &line)
  {
    if (line[0]==';')
      {
        // COMMENT
        return NULL;
      }
    else if (whitespace(line))
      {
        // EMPTY
        return NULL;
      }
    else if (line.substr(0,7)=="define ")
      {
        // DEFINE
        if (!processSafConfDefine(line))
          cerr << "WARNING: malformed SAF conf line: " << line << endl;
        return NULL; // coz no saf conf item
      }
    else
      {
        tSafConf* safConf = new tSafConf(); 
        
        // get strings before and after " -> "
        unsigned int i;
        i=line.find(" -> ");
        if (i==string::npos)
          // not found
          {
            cerr << "WARNING: missing \" -> \" "<< endl;
            cerr << "WARNING: malformed SAF conf line: " << line << endl;
            return NULL;
          }

        string pre=line.substr(0,i);
        string post=line.substr(i+4);

        // PRE: safConfMatch
        tSafConfMatch* match=processSafConfLinePre(pre);
        if (match!=NULL)
          safConf->match=*match;
        else
          {
            cerr << "WARNING: malformed SAF conf line: " << line << endl;
            return NULL;
          }

        // POST: nvPairs
        list<string> nvPairs = splitOnSpc(post);
        for (list<string>::iterator it=nvPairs.begin(); it!=nvPairs.end(); it++)
          {
            tSafConfNvpair* nvPair = extractNvpair(*it);
            if (nvPair!=NULL)
              safConf->nvPairs.push_back(*nvPair);
            else
              {
                cerr << "WARNING: malformed SAF conf line: " << line << endl;
                return NULL;
              }
          }
        return safConf;
      }

  }

// in: nvPair string
// out: nvPair *
tSafConfNvpair* tSMAFTokenizer::extractNvpair(const string &str)
{
  tSafConfNvpair* nvPair = new tSafConfNvpair();

  // find '.]'
  unsigned int i;
  i=str.find('=');
  if (i==string::npos)
    // not found
    {
      cerr << "WARNING: missing \"=\" "<< endl;
      return NULL;
    }

  string name=str.substr(0,i);
  if (name=="")
    {
      cerr << "WARNING: empty name "<< endl;
      return NULL;
    }

  string v=str.substr(i+1);

  // set NAME
  nvPair->name=name;

  if (v[0]=='\'' and v.size()>1 and v[v.size()-1]=='\'')
    // set VAL
    {
      nvPair->val=new string(v.substr(1,v.size()-2));
      nvPair->var=NULL;
    }
  else
    // set VAR
    {
      nvPair->var=new string(v);
      nvPair->val=NULL;
    }
  return nvPair;
}

// in: saf conf line before " -> "
tSafConfMatch* tSMAFTokenizer::processSafConfLinePre(const string &pre)
{
  // find '.]'
  unsigned int i;
  i=pre.find(".[");
  if (i==string::npos)
    // not found
    {
      cerr << "WARNING: missing \".[\" "<< endl;
      return NULL;
    }

  // TYPE = before '.]'
  string type=pre.substr(0,i);
  if (type=="")
    {
      cerr << "WARNING: empty type "<< endl;
      return NULL;
    }

  string rest=pre.substr(i+2);

  // find ']'
  unsigned int j;
  j=rest.find("]");
  if (j==string::npos)
    // not found
    {
      cerr << "WARNING: missing \"]\" " << endl;
      return NULL;
    }
  // abort if junk after ']'
  if (not(whitespace(rest.substr(j+1))))
    {
      cerr << "WARNING: junk after \"]\" " << endl;
      return NULL;
    }

  // get str betw '[' and ']'
  string restrStr=rest.substr(0,j);
  list<string> restrStrs = splitOnSpc(restrStr);

  tSafConfMatch* match = new tSafConfMatch();
  // set TYPE
  match->type=type;

  // set RESTRICTIONS
  list<tSafConfNvpair> restrs;
  for (list<string>::iterator it=restrStrs.begin(); it!=restrStrs.end(); it++)
    // walk thru nvPair strings
    {
      tSafConfNvpair* nvPair = extractNvpair(*it);
      if (nvPair!=NULL)
        restrs.push_back(*nvPair);
      else
        {
          // abort if problem
          delete match;
          cerr << "WARNING: malformed SAF conf match: " << pre << endl ;
          return NULL;
        }
    }
  match->restrictions=restrs;

  return match;
}

//
// tSafConfMatch
//

void tSafConfMatch::print() const
{
  // type
  cerr << "match.type=" << type << " "; 

  // restrictions
  cerr << "match.restrictions=[";
  for (list<tSafConfNvpair>::const_iterator it=restrictions.begin(); it!=restrictions.end(); it++)
    {
      it->print();
    }
  cerr << "]" << endl;

}

//
// tSafConfNvpair
//

void tSafConfNvpair::print() const
{
  // name
  cerr << name;
  if (val!=NULL)
    //val
    cerr << ":val." << *val << " "; 
  if (var!=NULL)
    //var
    cerr << ":var." << *var << " "; 

}

//
// tSafConf
//

void tSafConf::print() const
{
  cerr << endl;
  // match
  match.print();
  // nvPairs
  for (list<tSafConfNvpair>::const_iterator it=nvPairs.begin(); it!=nvPairs.end(); it++)
    {
      it->print();
    }
  cerr << endl;
}

//
// tSaf
//

void tSaf::print() const
{
  cerr << endl;

  // id
  if (id!=NULL)
    cerr << "saf.id=" << *id << " "; 
  else
    cerr << "saf.id NULL" << *id << " "; 
  // type
  if (type!=NULL)
    cerr << "saf.type=" << *type << " "; 
  else
    cerr << "saf.type NULL" << *type << " "; 
  // source
  if (source!=NULL)
    cerr << "saf.source=" << *source << " "; 
  else
    cerr << "saf.source NULL" << *source << " "; 
  // target
  if (target!=NULL)
    cerr << "saf.target=" << *target << " "; 
  else
    cerr << "saf.target NULL" << *target << " "; 
  // from
  if (from!=NULL)
    cerr << "saf.from=" << *from << " "; 
  else
    cerr << "saf.from NULL" << *from << " "; 
  // to
  if (to!=NULL)
    cerr << "saf.to=" << *to << " "; 
  else
    cerr << "saf.to NULL" << *to << " "; 

  // deps
  cerr << "saf.deps=[";
  for (list<string>::const_iterator it=deps.begin(); it!=deps.end(); it++)
    {
      cerr << *it << " ";
    }
  cerr << "]" << endl;

  // content
  cerr << "saf.content=[";
  for (map<string,string>::const_iterator it=content.begin(); it!=content.end(); it++)
    {
      cerr << it->first << ":" << it->second << " ";
    }
  cerr << "]" << endl;

  // local content
  cerr << "saf.lContent=[";
  for (map<string,string>::const_iterator it=lContent.begin(); it!=lContent.end(); it++)
    {
      cerr << it->first << ":" << it->second << " ";
    }
  cerr << "]" << endl;

}

//
// smaf conf gmap
//

//define gMap.carg (synsem lkeys keyrel carg) STRING

bool tSMAFTokenizer::processSafConfDefine(const string &line)
{
  if (!(line.substr(0,7)=="define "))
    return false;

  unsigned int i,j;

  i=skipWhitespace(line,7);
  j=findWhitespace(line,i);
  string name2=line.substr(i,j-i);
  //cerr << "NAME2=" << name2 << endl;
  //cerr << "NAME2=.substr(0,4)" << name2.substr(0,4) << endl;
  if (name2.substr(0,5)!="gMap.")
    {
      cerr << "WARNING: expexted define name to start with \"gMap.\"" << endl;
      return false;
    }
  string name=name2.substr(5);

  //cerr << "i=" << i << " j=" << j << " NAME: " << name << endl;
  //cerr << "line.substr(0,j)=" << line.substr(0,j) << endl;
  //cerr << "NAME=" << name << endl;
  //cerr << "after=" << line.substr(j) << endl;

  i=line.find('(',j);
  if (i==string::npos)
    {
      cerr << "WARNING: missing '('" << endl;
      return false;
    }
  if (not(whitespace(line.substr(j,i-j))))
    {
      cerr << "WARNING: junk before '('" << endl;
      return false;
    }

  //cout << "REST=" << line.substr(i) << endl;

  j=line.find(')',i);
  if (j==string::npos)
    {
      cerr << "WARNING: missing ')'" << endl;
      return false;
    }
  string pathStr=line.substr(i+1,j-i-1);
  string typeStr=line.substr(j+1);

  //cerr << "PATH=" << pathStr << endl;
  //cerr << "TYPE=" << typeStr << endl;

  list<string> path=splitOnSpc(pathStr);
  list<string> typeListTmp=splitOnSpc(typeStr);

  if (typeListTmp.size()>1)
    {
      cerr << "WARNING: junk after type: " << typeStr << endl;
      return false;
    }
  gMapType type=sym;
  if (typeListTmp.size()==1)
    {
      string typeStr=*typeListTmp.begin();
      downcase(typeStr);
      if (typeStr=="string")
        type=str;
      else if (typeStr=="symbol")
        type=sym;
      else
        {
          cerr << "WARNING: unknown type: " << typeStr << endl; 
          return false;
        }
    }

  _gMap.add(name,path,type);

  return true;
}

//
// gMap
//

ostream &operator<<(ostream& os, const list<string> &s)
{
  for (list<string>::const_iterator it=s.begin(); it!=s.end(); it++)
    {
      os << *it << " ";
    }
  return os;
}

ostream &operator<<(ostream& os, gMapType t)
{
  switch(t)
    {
    case str: os << "str"; break;
    case sym: os << "sym"; break;
    case unk: os << "unk"; break;
    default: throw tError("unexpected gMapType");
    }
  return os;
}

void tGMap::clear()
{
  _gMap.clear();
}

void tGMap::add(const string &name, const gMapPath & path, gMapType type)
{
  _gMap[name]=pair<gMapPath,gMapType>(path,type);
}

pair<gMapPath,gMapType> tGMap::get(const string &name) const
{
  static gMapPath unknownPath;
  map<string,pair<gMapPath,gMapType> >::const_iterator it = _gMap.find(name);
  return (it != _gMap.end()) ? it->second : pair<gMapPath,gMapType>(unknownPath,unk);
}

void tGMap::print() const
{
  for (map<string,pair<gMapPath,gMapType> >::const_iterator it=_gMap.begin(); it!=_gMap.end(); it++)
    {
      string name=it->first;
      gMapPath path=it->second.first;
      gMapType type=it->second.second;
      cout << "gMap " << name << " (" << path << ") " << type << endl;
    }
}

// in: content
// out: content with name=gMap.*
list<string> tSMAFTokenizer::getGMapNames (const map<string,string> &lContent)
{
  list<string> out;
  for (map<string, string>::const_iterator it = lContent.begin(); it!=lContent.end(); it++)
    {
      string name=it->first;
      if (name.substr(0,5)=="gMap.")
        out.push_back(name);
    }
  return out;
} 


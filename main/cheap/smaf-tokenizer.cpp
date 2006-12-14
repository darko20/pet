/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 *   2004 Bernd Kiefer bk@dfki.de
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

#include <iostream>

#include <xercesc/dom/DOM.hpp> 
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>

XERCES_CPP_NAMESPACE_USE

// NOTE: these are not private member functions because
//       (FIXME) global Grammar (defined in cheap.cpp) conflicts 
//       with contents of DOM.hpp
void downcase(string &s);
DOMDocument* xml2dom (string input);
int serializeDOM(DOMNode* node);
string getAttribute_string(DOMElement* element, string name);
int getAttribute_int(DOMElement* element, string name);
string getTextContent_string(DOMElement* element);
tInputItem* getInputItemFromTokenEdge(DOMElement* element, tSMAFTokenizer* tok);

void downcase(string &s) {
  int i;

  for (i=0; i!=s.length(); i++)
    {
       if ((unsigned char)s[i] < 127)
	{
	  s[i]=tolower(s[i]);
	}
    }

  return;
}

// map a string to a DOM:
//
// case (i): string is @PATHNAME
// case (ii): string is XML
//
DOMDocument* xml2dom (string input) {
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
    cout << "Error during XMLPlatformUtils::Initialize()! :\n"
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
    cout << "[XMLException] Exception message is: \n"
	 << message << "\n";
    XMLString::release(&message);
    return NULL;
  }
  catch (const DOMException& toCatch) {
    char* message = XMLString::transcode(toCatch.msg);
    cout << "[DOMException] Exception message is: \n"
	 << message << "\n";
    XMLString::release(&message);
    return NULL;
  }
  catch (const SAXParseException& toCatch) {
    char* message = XMLString::transcode(toCatch.getMessage());
    cout << "[SAXParseException] Exception message is: \n"
	 << message << "\n";
    XMLString::release(&message);
    return NULL;
  }
  catch (...) {
    cout << "Unexpected Exception \n" ;
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
int serializeDOM(DOMNode* node) {
  
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
    theSerializer->writeNode(myFormTarget, *node);
  }
  catch (const XMLException& toCatch) {
    char* message = XMLString::transcode(toCatch.getMessage());
    cout << "Exception message is: \n"
	 << message << "\n";
    XMLString::release(&message);
    return -1;
  }
  catch (const DOMException& toCatch) {
    char* message = XMLString::transcode(toCatch.msg);
    cout << "Exception message is: \n"
	 << message << "\n";
    XMLString::release(&message);
    return -1;
  }
  catch (...) {
    cout << "Unexpected Exception \n" ;
    return -1;
  }
  
  
  theSerializer->release();
  delete myFormTarget;
  cout << endl;
  return 0;
}

// return value of attribute 'name', or ""
string getAttribute_string(DOMElement* element, string name){
  const XMLCh *xval =  element->getAttribute(XMLString::transcode(name.c_str()));
  string val = Conv->convert((UChar *)xval, XMLString::stringLen(xval));
  return val;
}

// return int value of attribute 'name', 
//  or -1 is conversion to int fails,
//  or -1
int getAttribute_int(DOMElement* element, string name){
  const XMLCh *xval =  element->getAttribute(XMLString::transcode(name.c_str()));
  if (xval==NULL) return -1;

  const char *str = XMLCh2UTF8(xval);
  char *end;
  int res = strtol(str, &end, 10);
  if ((*end != '\0') || (str == end))
    {
      if (strlen(str)>1)
	// temp hack to allow node names of form letter + int eg. "v1"
	{
	  res = strtol(str+1, &end, 10);
	  if (!((*end != '\0') || (str == end)))
	    {
	      //cout << "STR: " << str << endl;
	      return res;
	    }
	}

      cout <<  "[WARNING] expected integer value for attribute '" << name << "' but got '" << str << "'" << endl;
    res = -1;
    }
  return res;
}

// return text content if element has only text children, or ""
string getTextContent_string(DOMElement* element)
{
  // all children should be text nodes
  DOMNodeList *nodes=element->getChildNodes();
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
  const XMLCh *xval =  element->getTextContent();
  string val = Conv->convert((UChar *)xval, XMLString::stringLen(xval));
  return val;
}

string getSlot(DOMElement* element, string name)
{
  // get all slots
  XMLCh* xstr = XMLString::transcode("slot");
  DOMNodeList *slots=element->getElementsByTagName(xstr);
  const XMLSize_t nodeCount = slots->getLength() ;

  // iterate through slots
  for( XMLSize_t ix = 0 ; ix < nodeCount ; ++ix )
    {
      DOMNode* sNode = slots->item( ix ) ;
      DOMElement* slot = dynamic_cast< xercesc::DOMElement* >( sNode );
      string sName = getAttribute_string(slot,"name");
      // if name matches return values
      if (sName==name)
	{
	string val = getTextContent_string(slot);
	if (val.length()!=0)
	  {
	    return val;
	  }
	}
    }
  return "";
}

// map a SMAF token edge to an internal tInputItem
tInputItem* getInputItemFromTokenEdge(DOMElement* element, tSMAFTokenizer* tok){
  // sanity check
  string type = getAttribute_string(element,"type");
  if (type!="token")
    throw Error("INTERNAL");
  
  // surface is SIMPLE CONTENT...
  string surface = getTextContent_string(element);
  if (surface.length()==0) 
    // ... or VALUE
    surface = getAttribute_string(element,"value");
  if (surface.length()==0)
    // ... or a SLOT
    surface = getSlot(element,"surface");
  if (surface.length()==0)
    {
      cout << "[WARNING] unable to extract surface value from edge:" << endl;
      serializeDOM(element);
      cout << endl;
      return NULL;
    }


  downcase(surface);

  string id = getAttribute_string(element,"id");
  int start = getAttribute_int(element,"cfrom");
  int end = getAttribute_int(element,"cto");
  string sourceSmaf = getAttribute_string(element,"source");
  string targetSmaf = getAttribute_string(element,"target");

  if (sourceSmaf.length()==0)
    {
      cout << "[WARNING] unable to extract 'source' value from edge:" << endl;
      serializeDOM(element);
      cout << endl;
      return NULL;
    }

  if (targetSmaf.length()==0)
    {
      cout << "[WARNING] unable to extract 'target' value from edge:" << endl;
      serializeDOM(element);
      cout << endl;
      return NULL;
    }

  int source, target;
  source = (*tok).add2nodeMapping(sourceSmaf);
  target = (*tok).add2nodeMapping(targetSmaf);

  //cout << "SOURCE= " << source << " / " << sourceSmaf << endl;
  //cout << "TARGET= " << target << " / " << targetSmaf << endl;

  ::modlist mods;
  
  if (start<0)
    {
      cout << "[WARNING] unable to extract 'cfrom' value from edge:" << endl;
      serializeDOM(element);
      cout << endl;
      return NULL;
    }

  if (end<0)
    {
      cout << "[WARNING] unable to extract 'cto' value from edge:" << endl;
      serializeDOM(element);
      cout << endl;
      return NULL;
    }

  tInputItem* item = new tInputItem(id, source, target, start, end, surface, ""
  //tInputItem* item = new tInputItem(id, start, end, start, end, surface, ""
				    , tPaths()
				    , WORD_TOKEN_CLASS, mods);

  return item;
}

// return item matching (external) id
tInputItem* getItemById(string id, inp_list &items)
{
  for(list<tInputItem *>::iterator it = items.begin()
        ; it != items.end(); it++) {
      if (((*it)->external_id())==id)
	{
	  return *it;
	}
    }
  return NULL;
}

void processPosEdge(DOMElement* element, inp_list &items){
  string type = getAttribute_string(element,"type");
  if (type!="pos")
    throw Error("INTERNAL");

  // tag is SIMPLE CONTENT...
  string tag = getTextContent_string(element);
  if (tag.length()==0) 
    // ... or VALUE
    tag = getAttribute_string(element,"value");
  if (tag.length()==0)
    // ... or a SLOT
    tag = getSlot(element,"tag");
  if (tag.length()==0)
    {
      cout << "[WARNING] unable to extract tag from edge:";
      serializeDOM(element);
      cout << endl;
      return;
    }
  //cout << "tag= ." << tag << "." << endl;

  string deps = getAttribute_string(element,"deps");

  // we only handle SINGLE dependencies
  if (deps.find(' ')!=string::npos)
    {
      cout << "[WARNING] ignoring edge with unhandled multiple dependencies:";
      serializeDOM(element);
      cout << endl;
      return;
    }
  string dep = deps;

  tInputItem* item = getItemById(dep, items);
  if (item!=NULL)
    {
      postags pos;
      pos.add(tag);
      item->set_in_postags(pos);
    }

  return;
}


void tSMAFTokenizer::tokenize(string input, inp_list &result) {

  clearNodeMapping();
  DOMDocument* dom = xml2dom(input);

  // abort if something went wrong
  if (dom==NULL)
    return;

  // process lattice element
  {
      XMLCh* xstr = XMLString::transcode("lattice");
      DOMNodeList* elts = dom->getElementsByTagName(xstr);
      XMLString::release(&xstr);
      const XMLSize_t nodeCount = elts->getLength() ;
      if (nodeCount!=1)
	{
	  cout << "[WARNING] corrupted lattice (multiple lattice elements found)!" << endl;
	  return;
	}
      DOMNode* lNode = elts->item( 0 ) ;
      DOMElement* element = dynamic_cast< xercesc::DOMElement* >( lNode );

      string init, final;
      init = getAttribute_string(element,"init");
      final = getAttribute_string(element,"final");

      if (init.length()==0)
	{
	  cout << "[ERROR] no 'init' value found in lattice!" << endl;
	  return;
	}
      else
	{
	  setNodeMap(init,0);
	  _chartNodeMax=0;
	}

      if (final.length()==0)
	{
	  cout << "[ERROR] no 'final' value found in lattice!" << endl;
	  return;
	}
      else
	{
	  //we will set final node to number of edges
	  XMLCh* xstr2 = XMLString::transcode("edge");
	  DOMNodeList* welts2 = dom->getElementsByTagName(xstr2);
	  XMLString::release(&xstr2);
	  const XMLSize_t nodeCount = welts2->getLength() ;

	  setNodeMap(final,nodeCount);
	}
  }

  if (dom!=NULL)
    // TOKENS
    {
      // get all <edge> elements
      XMLCh* xstr = XMLString::transcode("edge");
      DOMNodeList* welts = dom->getElementsByTagName(xstr);
      XMLString::release(&xstr);
      const XMLSize_t nodeCount = welts->getLength() ;
      // map each into a tInputItem
      for( XMLSize_t ix = 0 ; ix < nodeCount ; ++ix ){
	DOMNode* wNode = welts->item( ix ) ;
	//serializeDOM(wNode);
	DOMElement* element = dynamic_cast< xercesc::DOMElement* >( wNode );
	string type = getAttribute_string(element,"type");
	if (type=="token")
	  {
	    tInputItem* item = getInputItemFromTokenEdge(element, this);
	    if (item==NULL)
	      {
		cout << "[WARNING] unable to construct chart edge from:" << endl;
		serializeDOM(element);
		cout << endl;
	      }
	    else
	      result.push_back(item);
	  }
      }
    }

  //  cout << "tags" << endl;
  if (dom!=NULL)
    // TAGS
    {
      // get all <edge> elements
      XMLCh* xstr = XMLString::transcode("edge");
      DOMNodeList* welts = dom->getElementsByTagName(xstr);
      XMLString::release(&xstr);
      const XMLSize_t nodeCount = welts->getLength() ;
      // map each into a tInputItem
      for( XMLSize_t ix = 0 ; ix < nodeCount ; ++ix ){
	DOMNode* wNode = welts->item( ix ) ;
	
	//cout << "TAG" << endl;
	//serializeDOM(wNode);
	DOMElement* element = dynamic_cast< xercesc::DOMElement* >( wNode );
	string type = getAttribute_string(element,"type");
	if (type=="pos")
	  {
	    //    cout << "pos" << endl;
	    processPosEdge(element, result);
	  }  
	//cout << "*" << endl;
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
      cout << "[ERROR] cannot find 'init' node in input lattice" << endl;
      result.clear();
      return;
    }
  
  //cout << "tokenize OUT" << endl;

  return;
}

void tSMAFTokenizer::clearNodeMapping()
{
  _nodeMapping.clear();
}

int tSMAFTokenizer::add2nodeMapping(string smafNode)
{
  //cout << "add2nodeMapping " << smafNode;
  int node = _nodeMapping[smafNode]-1;
  if (node==-1)
    {
      int chartNode = ++_chartNodeMax;
      _nodeMapping[smafNode] = chartNode+1;
      node = chartNode;
    }
  //cout << " -> " << node << endl;;
  return node;
}

void  tSMAFTokenizer::setNodeMap(string smafNode, int chartNode)
{
  _nodeMapping[smafNode] = chartNode+1;
}

string tSMAFTokenizer::getSmafNode(int chartNode)
{
  // not implemented
  return "";
}

int tSMAFTokenizer::getChartNode(string smafNode)
{
  int chartNode = _nodeMapping[smafNode];
  return chartNode-1;
}

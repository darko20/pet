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
// FIXME: global Grammar (defined in cheap.cpp) conflicts with contents of DOM.hpp
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>

XERCES_CPP_NAMESPACE_USE

DOMDocument* xml2dom (string input) {
  //  cout << "INPUT START" << endl;
  //  cout << input;
  //  cout << "INPUT END" << endl;
  string buffer = input;
  MemBufInputSource xmlinput((const XMLByte *) buffer.c_str()
			     , buffer.length(), "STDIN");
  
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
  
  XercesDOMParser* parser = new XercesDOMParser();
  parser->setValidationScheme(XercesDOMParser::Val_Always);    // optional.
  parser->setDoNamespaces(true);    // optional
  
  ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
  parser->setErrorHandler(errHandler);

  try {
    parser->parse(xmlinput);
  }
  catch (const XMLException& toCatch) {
    char* message = XMLString::transcode(toCatch.getMessage());
    cout << "Exception message is: \n"
	 << message << "\n";
    XMLString::release(&message);
    return NULL;
  }
  catch (const DOMException& toCatch) {
    char* message = XMLString::transcode(toCatch.msg);
    cout << "Exception message is: \n"
	 << message << "\n";
    XMLString::release(&message);
    return NULL;
  }
  catch (...) {
    cout << "Unexpected Exception \n" ;
    return NULL;
  }
  
  // what about XMLPlatformUtils::terminate() ???

  DOMDocument* doc = parser->adoptDocument();

  delete parser;
  delete errHandler;
  return doc;
}


int serializeDOM(DOMNode* node) {
  
  cout << "SERIALIZE START" << endl;
  
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
  cout << "SERIALIZE END" << endl;
  return 0;
}

string getAttribute_string(DOMElement* element, char* name){
  const XMLCh *xval =  element->getAttribute(XMLString::transcode(name));
  string val = Conv->convert((UChar *)xval, XMLString::stringLen(xval));
  return val;
}
 
int getAttribute_int(DOMElement* element, char* name){
  const XMLCh *xval =  element->getAttribute(XMLString::transcode(name));

  const char *str = XMLCh2UTF8(xval);
  char *end;
  int res = strtol(str, &end, 10);
  if ((*end != '\0') || (str == end))
    throw Error((string) "expected integer value for attribute '" + name + "' in ");
  return res;
}

string getTextContent_string(DOMElement* element){
  const XMLCh *xval =  element->getTextContent();
  string val = Conv->convert((UChar *)xval, XMLString::stringLen(xval));
  return val;
}

tInputItem* getInputItemFromW(DOMElement* element){
    //serializeDOM(wNode);
    
    string id = getAttribute_string(element,"id");
    int start = getAttribute_int(element,"cstart");
    int end = getAttribute_int(element,"cend");
    int source = start;
    int target = end;
    string surface = getTextContent_string(element);

    ::modlist mods;

    tInputItem* item = new tInputItem(id, source, target, start, end, surface, ""
				      , tPaths()
				      , WORD_TOKEN_CLASS, mods);
    
    //    cout << id << endl;
    //    cout << start << endl;
    //    cout << end << endl;
    //    cout << surface << endl;
    //item->print2();
    return item;
}

//FIXME:
// - SMAF soure/target should be general text
// - seg faults on <olac:olac> ?namespaces?
// - seg faults on DOCTYPE declaration

tInputItem* getInputItemFromEdge(DOMElement* element){  
  string id = getAttribute_string(element,"id");
  int start = getAttribute_int(element,"cfrom");
  int end = getAttribute_int(element,"cto");
  int source = getAttribute_int(element,"source");
  int target = getAttribute_int(element,"target");
  string surface = getTextContent_string(element);
  
  ::modlist mods;
  
  tInputItem* item = new tInputItem(id, source, target, start, end, surface, ""
				    , tPaths()
				    , WORD_TOKEN_CLASS, mods);
  return item;
}



/** Produce a set of tokens from the given XML input. */
void tSMAFTokenizer::tokenize(string input, inp_list &result) {

  DOMDocument* dom = xml2dom(input);
  //serializeDOM(dom);

  // get all <w> elements
  //XMLCh* xstr = XMLString::transcode("w");
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
    //tInputItem* item = getInputItemFromW(element);
    tInputItem* item = getInputItemFromEdge(element);
    result.push_back(item);    
  }
  return;
}

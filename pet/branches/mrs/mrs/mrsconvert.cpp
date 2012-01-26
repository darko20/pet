#include "mrs-reader.h"
#include "mrs-printer.h"
#include "eds.h"
#ifdef MRS_ONLY
#include "mrs-errors.h"
#else
#include "pet-config.h"
#include "errors.h"
#endif
#ifdef HAVE_XML
#include "mrs-handler.h"
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/util/XMLString.hpp>
#include "unicode.h"
XERCES_CPP_NAMESPACE_USE
#endif
#include <iostream>
#include <cstring>
#include <cstdlib>

extern class EncodingConverter *Conv;

using namespace std;


int main(int argc, char **argv) {
  mrs::MrsPrinter *printer;
  mrs::tMrsReader *reader;
  if (argc == 2) {
    if (strcmp(argv[1], "-s2x") == 0) {
      reader = new mrs::SimpleMrsReader();
      printer = new mrs::MrxMrsPrinter(cout);
    } else if (strcmp(argv[1], "-x2s") == 0) {
#ifdef HAVE_XML
      reader = new mrs::XmlMrsReader();
      printer = new mrs::SimpleMrsPrinter(cout);
      xml_initialize();
      Conv = new EncodingConverter("utf8");
#else
      cerr << "Not compiled with XML, exiting." << endl;
      exit(1);
#endif
    } else {
      cerr << "Unknown conversion." << endl;
      exit(1);
    }

  } else {
      reader = new mrs::SimpleMrsReader();
      printer = new mrs::MrxMrsPrinter(cout);
  }
  
  string line;
  do {
    getline(cin, line);
    if (!line.empty()) {
      try {
  	mrs::tMrs *mrs = reader->readMrs(line);
   	printer->print(mrs);
//    mrs::tEds *eds = new mrs::tEds(mrs);
//    eds->print_eds();
      }
      catch (tError &e) {
   	cerr << "Failed to parse with error: \"" << e.getMessage() << "\""
   	     << endl;
      }
    }
  } while (!cin.eof());
  
  #ifdef HAVE_XML
  delete Conv;
  #endif
  delete printer;
  delete reader;
  return 0;
}

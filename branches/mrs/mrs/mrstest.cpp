#include "mrs.h"
#include "eds.h"
#include "mrs-reader.h"
#include "mrs-printer.h"
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

extern class EncodingConverter *Conv;

using namespace std;

int main(int argc, char **argv) {

  mrs::SimpleMrsReader reader;
//  mrs::XmlMrsReader reader;
//  xml_initialize();
//  Conv = new EncodingConverter("utf8");
  mrs::SimpleMrsPrinter printer(cout);
//  mrs::HtmlMrsPrinter printer(cout);
//  mrs::MrxMrsPrinter printer(cout);
  string line;
  getline(cin, line);
  while (!cin.eof()) {
    if (!line.empty()) {  
      try {
        mrs::tMrs *mrs = reader.readMrs(line);
        mrs::tEds *eds = new mrs::tEds(mrs);
//        printer.print(mrs);
//        mrs::tEds *eds = new mrs::tEds();
//        eds->read_eds(line);
        eds->print_eds();
//        eds->print_triples();
        delete mrs;
        delete eds;
      }
      catch (tError &e) {
        cerr << "Failed to parse with error: \"" << e.getMessage() << "\"" 
          << endl;;
      }
    }
    getline(cin, line);
  }
}

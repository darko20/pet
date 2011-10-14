#include "mrs-reader.h"
#include "mrs-printer.h"
#include "mrs-handler.h"
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/util/XMLString.hpp>
#include "unicode.h"
XERCES_CPP_NAMESPACE_USE
#include "errors.h"
#include <iostream>

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
      reader = new mrs::XmlMrsReader();
      printer = new mrs::SimpleMrsPrinter(cout);
    } else {
      cerr << "Unknown conversion." << endl;
      exit(1);
    }

  } else {
      reader = new mrs::SimpleMrsReader();
      printer = new mrs::MrxMrsPrinter(cout);
  }
  
  xml_initialize();
  //  XMLPlatformUtils::Initialize();
  Conv = new EncodingConverter("utf8");
  //initialize_encoding_converter("utf8");
  //mrs::SimpleMrsPrinter printer(cout);
  //MrsHandler reader(false);
  //mrs::XmlMrsReader reader;
  // XMLCh * XMLFilename = XMLString::transcode("/tmp/mrs");
  // LocalFileInputSource inputfile(XMLFilename);
  // if (parse_file(inputfile, &reader)
  //     && (! reader.error())) {
  //   printer.print(reader.mrss().front());
  // }
  string line;
  do {
    getline(cin, line);
    if (!line.empty()) {
      try {
  	mrs::tMrs *mrs = reader->readMrs(line);
   	printer->print(mrs);
      }
      catch (tError &e) {
   	cerr << "Failed to parse with error: \"" << e.getMessage() << "\""
   	     << endl;
      }
    }
  } while (!cin.eof());
  
  delete Conv;
  delete printer;
  delete reader;
  return 0;
}

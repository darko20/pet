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


int main(int argc, char ** argv) {
  xml_initialize();
  //  XMLPlatformUtils::Initialize();
  Conv = new EncodingConverter("utf8");
  //initialize_encoding_converter("utf8");
  //mrs::XmlMrsReader reader;
  mrs::SimpleMrsPrinter printer(cout);
  MrsHandler reader(true);
  XMLCh * XMLFilename = XMLString::transcode("/tmp/mrs");
  LocalFileInputSource inputfile(XMLFilename);
  if (parse_file(inputfile, &reader)
      && (! reader.error())) {
    printer.print(reader.mrss().front());
  }
  // string line;
  // getline(cin, line);
  // while (!cin.eof()) {
  //   if (!line.empty()) {
  //     try {
  // 	mrs::tMrs *mrs = reader.readMrs(line);
  // 	printer.print(mrs);
  //     }
  //     catch (tError &e) {
  // 	cerr << "Failed to parse with error: \"" << e.getMessage() << "\""
  // 	     << endl;
  //     }
  //   }
  //   getline(cin, line);
  // }
  delete Conv;
  return 0;
}

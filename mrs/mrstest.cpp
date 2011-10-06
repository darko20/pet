#include "mrs.h"
#include "mrs-reader.h"
#include "mrs-printer.h"
#include "errors.h"
#include <iostream>

using namespace std;

int main(int argc, char **argv) {

  mrs::SimpleMrsReader reader;
//  mrs::SimpleMrsPrinter printer(cout);
//  mrs::HtmlMrsPrinter printer(cout);
  mrs::MrxMrsPrinter printer(cout);
  string line;
  getline(cin, line);
  while (!cin.eof()) {
    if (!line.empty()) {  
      try {
        mrs::tMrs *mrs = reader.readMrs(line);
        printer.print(mrs);
      }
      catch (tError &e) {
        cerr << "Failed to parse with error: \"" << e.getMessage() << "\"" 
          << endl;;
      }
    }
    getline(cin, line);
  }
}

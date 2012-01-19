#include "mrs.h"
#include "edg.h"
#include "mrs-reader.h"
#include "mrs-printer.h"
#ifdef MRS_ONLY
#include "mrs-errors.h"
#else
#include "pet-config.h"
#include "errors.h"
#endif
#include <iostream>

using namespace std;

int main(int argc, char **argv) {

  mrs::SimpleMrsReader reader;
  mrs::SimpleMrsPrinter printer(cout);
//  mrs::HtmlMrsPrinter printer(cout);
//  mrs::MrxMrsPrinter printer(cout);
  string line;
  getline(cin, line);
  while (!cin.eof()) {
    if (!line.empty()) {  
      try {
        mrs::tMrs *mrs = reader.readMrs(line);
        mrs::tEdg *edg = new mrs::tEdg(mrs);
        printer.print(mrs);
        edg->print_edg();
      }
      catch (tError &e) {
        cerr << "Failed to parse with error: \"" << e.getMessage() << "\"" 
          << endl;;
      }
    }
    getline(cin, line);
  }
}

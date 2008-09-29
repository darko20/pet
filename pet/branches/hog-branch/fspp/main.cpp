// stand-alone DELPH-IN preprocessor (FSPP)
// (C) Ben Waldron, Bernd Kiefer, Stephan Oepen
//

#define FSPP_VERSION 0.01

using namespace std;

#include <iostream>
#include <iomanip>   // format manipulation
#include <string>
#include <fspp.h>
//#include <ecl.h>

#include "unicode.h"
#include "petecl.h"

class EncodingConverter *Conv;

int
help(void) {
    cout << "Usage:\tfspp FSR_FILENAME (OUTPUT_FORMAT)" << endl;
    cout << "\twhere\t- OUTPUT_FORMAT may be one of :smaf :saf";
    cout << endl;
    cout << "\t\t- (stdin input) each sentence must be terminated by CONTROL-Q followed by NEWLINE";
    cout << endl;
    cout << "\t\t- (stdout output) as appropriate for OUTPUT_FORMAT, followed by CONTROL-Q";
    cout << endl;
    cout << "\t\t- all character encoding should be UTF-8";
    cout << endl;
    return 0;
}
int main(int argc, char **argv) {
    cout << "(FSPP version " << fixed << setprecision(2) << FSPP_VERSION << ")" << endl;
    cout << "(For support please see: http://wiki.delph-in.net)" << endl;

    if (argc < 2)  { help(); return 1; }
    const char *format = (argc < 3) ? ":smaf" : argv[2];

    Conv = new EncodingConverter("utf-8");
    cout << "; Input character-encoding: " << Conv->encodingName() << endl;
  
    ecl_initialize(1, argv);
    
    preprocessor_initialize(argv[1]);
    cout << "Initialized" << endl;

    for (;;)
    {
        string input;
        UnicodeString u_input;
        for (;;)
        {
            char ch;
            cin.get(ch);

            //cout << "C" << (int) ch << endl;

            if (ch == '\x16') break; // CONTROL-V
            if (ch == '\x11') break; // CONTROL-Q
            input += ch;
        }
        cin.ignore( INT_MAX, '\n' ); // skip remainder of line
        if (input.empty()) break; // empty input is signal to exit
        u_input = Conv->convert(input);
        cout << Conv->convert(preprocess(u_input, format)) << '\x11' << endl;
    }
    return 0;
}


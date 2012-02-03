#include "mrs.h"
#include "eds.h"
#include "mrs-reader.h"
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
  string line1, line2;
  getline(cin, line1);
  getline(cin, line2);
  if (!line1.empty() && !line2.empty()) {
    mrs::tMrs *mrs_a = reader.readMrs(line1);
    mrs::tEds *eds_a = new mrs::tEds(mrs_a);
    mrs::tMrs *mrs_b = reader.readMrs(line2);
    mrs::tEds *eds_b = new mrs::tEds(mrs_b);
    //cout << "A" << endl;
    //eds_a->print_eds();
    //eds_a->print_triples();
    //cout << "B" << endl;
    //eds_b->print_triples();

    mrs::tEdsComparison *result = eds_a->compare_triples(eds_b);

    cout << "score is " << result->score << endl;
    if (!result->unmatchedA.empty() || !result->unmatchedB.empty()) {
      cout << "Unmatched Triples" << endl;
      for (vector<string>::iterator it = result->unmatchedA.begin();
            it != result->unmatchedA.end(); ++it) 
        cout << "< " << *it << endl;
      for (vector<string>::iterator it = result->unmatchedB.begin();
            it != result->unmatchedB.end(); ++it) 
        cout << "> " << *it << endl;
    }
  }
}

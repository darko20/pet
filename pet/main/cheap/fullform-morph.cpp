/* -*- Mode: C++ -*- */

#include "tFullformMorph.h"
#include "grammar-dump.h"

/** Create a morphology component from the fullform tables. 
 * \pre  The dumper must be set to the beginning of the (existing)
 * fullform section
 */
tFullformMorphology *
tFullformMorphology::create(dumper &dmp) {
  dmp.seek(0);
  int version;
  undump_header(&dmp, version);
  dump_toc toc(&dmp);
  if(toc.goto_section(SEC_FULLFORMS)) {
    tFullformMorphology *newff = new tFullformMorphology(dmp);
    if (newff->_fullforms.empty()) {
      delete newff;
      newff = NULL;
    }
    return newff;
  } else {
    return NULL;
  }       
}

tFullformMorphology::tFullformMorphology(dumper &dmp) {
  int nffs = dmp.undump_int();
  int invalid = 0;
  for(int i = 0; i < nffs; i++)
    {
      int preterminal = dmp.undump_int();
      lex_stem *lstem;
      // If we do not find a stem for the preterminal, this full form is not
      // valid 
      if ((lstem = Grammar->find_stem(preterminal)) != NULL) {
          
        const char *stem = print_name(preterminal);

        int affix = dmp.undump_int();
        list<type_t> affixes;
        if (affix != -1) {
          affixes.push_front(affix);
        }
          
        int offset = dmp.undump_char();
        // _fix_me_ the is not yet possible, and wrongly modelled anyway
        // lstem->keydtr(offset);
          
        char *s = dmp.undump_string(); 
        list<string> forms;
        forms.push_front(string(s));
        forms.push_back(string(stem));
        delete[] s;
          
        _fullforms.insert(make_pair(string(stem)
                                    , tMorphAnalysis(forms, affixes)));
          
        if(verbosity > 14)
          {
            // print(fstatus); // _fix_me_ do this explicitely
            fprintf(fstatus, "\n");
          }
      } else {
        invalid++;
        fprintf(fstatus, "!"); fflush(status);
      }
    }

  if(verbosity > 4) {
    fprintf(fstatus, ", %d full form entries", nffs);
    if (invalid > 0) fprintf(fstatus, ", %d of them invalid", invalid);
  }
}

/** Compute morphological results for \a form. */
list<tMorphAnalysis> tFullformMorphology::operator()(const myString &form){
  list<tMorphAnalysis> result;
  pair<ffdict::iterator,ffdict::iterator> p = _fullforms.equal_range(form);
  for(ffdict::iterator it = p.first; it != p.second; ++it) {
    result.push_back(it->second);
  }
  return result;
}


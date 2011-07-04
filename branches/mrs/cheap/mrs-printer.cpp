#include "mrs-printer.h"
#include "vpm.h"
#include "utility.h"

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

using namespace mrs;

extern tVPM *vpm;

void print_mrs_as(char format, dag_node *dag, std::ostream &out) {
  // if(it->trait() == PCFG_TRAIT) f = instantiate_robust(it);
  tPSOA* mrs = new tPSOA(dag);
  if (mrs->valid()) {
    tPSOA* mapped_mrs = vpm->map_mrs(mrs, true);
    if (mapped_mrs->valid()) {
      out << std::endl;
      switch (format) {
      case 'n':
        { MrxMRSPrinter ptr(out); ptr.print(mapped_mrs); break; }
      case 's':
        { SimpleMRSPrinter ptr(out); ptr.print(mapped_mrs); break; }
      }
      out << std::endl;
    }
    delete mapped_mrs;
  }
  delete mrs;
}


void MrxMRSPrinter::
print(tPSOA* mrs) {
  *_out << "<mrs>\n";
  *_out << boost::format("<label vid='%d'/>") % mrs->top_h->id;
  *_out << boost::format("<var vid='%d'/>\n") % mrs->index->id;
  for (std::list<tBaseRel*>::iterator rel = mrs->liszt.begin();
       rel != mrs->liszt.end(); ++rel) {
    print((tRel*)(*rel));
  }
  for (std::list<tHCons*>::iterator hcons = mrs->h_cons.begin();
       hcons != mrs->h_cons.end(); ++hcons) {
    print(*hcons);
  }
  for (std::list<tHCons*>::iterator acons = mrs->a_cons.begin();
       acons != mrs->a_cons.end(); ++acons) {
    print(*acons);
  }
  *_out << "</mrs>\n";
}

void MrxMRSPrinter::
print(tRel* rel) {
  *_out << boost::format("<ep cfrom='%d' cto='%d'>") % rel->cfrom % rel->cto;
  if (rel->pred[0] == '"')
    *_out << "<spred>" << xml_escape(rel->pred.substr(1,rel->pred.length()-2))
          << "</spred>";
  else {
    char* uppred = new char[rel->pred.length()+1];
    strtoupper(uppred, rel->pred.c_str());
    *_out << "<pred>" << xml_escape(uppred) << "</pred>"; //pred;
    delete uppred;
  }
  *_out << boost::format("<label vid='%d'/>") % rel->handel->id;
  std::list<std::string> feats;
  for (std::map<std::string,tValue*>::iterator fvpair = rel->flist.begin();
       fvpair != rel->flist.end(); ++fvpair)
    feats.push_back((*fvpair).first);
  feats.sort(ltfeat());
  for (std::list<std::string>::iterator feat = feats.begin();
       feat != feats.end(); ++feat) {
    *_out << std::endl << "<fvpair>";
    *_out << "<rargname>" << xml_escape(*feat) << "</rargname>";
    print_full(rel->flist[*feat]);
    *_out << "</fvpair>";
  }
  *_out << "</ep>" << std::endl;
}

void MrxMRSPrinter::
print(tValue* val) {
  if (dynamic_cast<tConstant *>(val) != NULL) {
    print((tConstant*)val);
  } else if (dynamic_cast<tVar *>(val) != NULL) {
    print((tVar*)val);
  }
}

void MrxMRSPrinter::
print_full(tValue* val) {
  if (dynamic_cast<tConstant *>(val) != NULL) {
    print((tConstant*)val);
  } else if (dynamic_cast<tVar *>(val) != NULL) {
    print_full((tVar*)val);
  }
}

void MrxMRSPrinter::
print(tConstant* constant) {
  if (constant->value[0] == '"')
    *_out << "<constant>" << xml_escape(constant->value.substr(1,constant->value.length()-2))
        << "</constant>";
  else
    *_out << "<constant>" << xml_escape(constant->value) << "</constant>";
}

void MrxMRSPrinter::
print(tVar* var) {
    *_out << boost::format("<var vid='%d' sort='%s'></var>") % var->id % xml_escape(var->type);
}

void MrxMRSPrinter::
print_full(tVar* var) {
  *_out << boost::format("<var vid='%d' sort='%s'>") % var->id % xml_escape(var->type);
  // _todo_ this should be adapted to handle the feature priority
  std::list<std::string> feats;
  for (std::map<std::string,std::string>::iterator extrapair = var->extra.begin();
       extrapair != var->extra.end(); ++extrapair)
    feats.push_back((*extrapair).first);
  feats.sort(ltextra());
  for (std::list<std::string>::iterator feat = feats.begin();
       feat != feats.end(); ++feat) {
    char* upvalue = new char[var->extra[*feat].length()+1];
    strtoupper(upvalue, var->extra[*feat].c_str());
    *_out << std::endl << "<extrapair><path>" << xml_escape(*feat)
          << "</path><value>"
          << xml_escape(upvalue) << "</value></extrapair>";
    delete upvalue;
  }
  *_out << "</var>";
}

void MrxMRSPrinter::
print(tHCons* hcons) {
  *_out << "<hcons hreln='";
  if (hcons->relation == tHCons::QEQ)
    *_out << "qeq";
  else if (hcons->relation == tHCons::LHEQ)
    *_out << "lheq";
  else if (hcons->relation == tHCons::OUTSCOPES)
    *_out << "outscopes";
  *_out << "'>";
  *_out << "<hi>";
  print(hcons->scarg);
  *_out << "</hi>";
  *_out << "<lo>";
  print(hcons->outscpd);
  *_out << "</lo>";
  *_out << "</hcons>\n";
}

void SimpleMRSPrinter::
print(tPSOA* mrs) {
  *_out << boost::format(" [ LTOP: %s%d") % mrs->top_h->type % mrs->top_h->id;
  *_out << std::endl << "   INDEX: ";
  print(mrs->index);
  *_out << std::endl << "   RELS: <";
  for (std::list<tBaseRel*>::iterator rel = mrs->liszt.begin();
       rel != mrs->liszt.end(); ++rel) {
    *_out << std::endl;
    print((tRel*)(*rel));
  }
  *_out << " >" << std::endl << "   HCONS: <";
  for (std::list<tHCons*>::iterator hcons = mrs->h_cons.begin();
       hcons != mrs->h_cons.end(); ++hcons) {
    *_out << " ";
    print(*hcons);
  }
  *_out << " >";
  // _todo_ i'm not sure how acons (if any) are printed in the simple format (yi)
  if (!mrs->a_cons.empty()) {
    *_out << std::endl << "   ACONS: <";
    for (std::list<tHCons*>::iterator acons = mrs->a_cons.begin();
         acons != mrs->a_cons.end(); ++acons) {
      *_out << " ";
      print(*acons);
    }
    *_out << " >";
  }
  *_out << " ]" << std::endl;
  _vars.clear();
}

void SimpleMRSPrinter::
print(tRel* rel) {
  *_out << boost::format("          [ %s<%d:%d>") % rel->pred % rel->cfrom % rel->cto << std::endl;
  *_out << boost::format("            LBL: %s%d") % rel->handel->type % rel->handel->id;
  std::list<std::string> feats;
  for (std::map<std::string,tValue*>::iterator fvpair = rel->flist.begin();
       fvpair != rel->flist.end(); ++fvpair)
    feats.push_back((*fvpair).first);
  feats.sort(ltfeat());
  for (std::list<std::string>::iterator feat = feats.begin();
       feat != feats.end(); ++feat) {
    *_out << std::endl << "            " << *feat << ": ";
    print(rel->flist[*feat]);
  }
  *_out << " ]";
}

void SimpleMRSPrinter::
print(tValue* val) {
  // _todo_
  if (dynamic_cast<tConstant *>(val)) {
    print((tConstant*)val);
  } else if (dynamic_cast<tVar *>(val)) {
    print((tVar*)val);
  }
}

void SimpleMRSPrinter::
print(tConstant* constant) {
  *_out << constant->value;
}

void SimpleMRSPrinter::
print(tVar* var) {
  if (_vars.find(var->id) == _vars.end()) { // first time of printing the variable
    print_full(var);
    _vars[var->id] = var;
  } else { // the variable has been printed before
    *_out << boost::format("%s%d") % var->type % var->id;
  }
}

void SimpleMRSPrinter::
print_full(tVar* var) {
  if (var->extra.begin() == var->extra.end()) {
    *_out << boost::format("%s%d") % var->type % var->id;
    return;
  }
  *_out << boost::format("%s%d [ %s ") % var->type % var->id % var->type;
  std::list<std::string> feats;
  for (std::map<std::string,std::string>::iterator extrapair = var->extra.begin();
       extrapair != var->extra.end(); ++extrapair)
    feats.push_back((*extrapair).first);
  feats.sort(ltextra());
  for (std::list<std::string>::iterator feat = feats.begin();
       feat != feats.end(); ++feat) {
    char* upvalue = new char[var->extra[*feat].length()+1];
    strtoupper(upvalue, var->extra[*feat].c_str());
    *_out  << *feat << ": " << upvalue << " ";
    delete upvalue;
  }
  *_out << "]";
}

void SimpleMRSPrinter::
print(tHCons* hcons) {
  print(hcons->scarg);
  if (hcons->relation == tHCons::QEQ)
    *_out << " qeq ";
  else if (hcons->relation == tHCons::LHEQ)
    *_out << " lheq ";
  else if (hcons->relation == tHCons::OUTSCOPES)
    *_out << " outscopes ";
  print(hcons->outscpd);
}

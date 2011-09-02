#include "mrs-printer.h"
#include "utility.h"

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

using namespace mrs;

/* PET specific
extern tVPM *vpm;

void print_mrs_as(char format, dag_node *dag, std::ostream &out) {
  // if(it->trait() == PCFG_TRAIT) f = instantiate_robust(it);
  tMRS* mrs = new tMRS(dag);
  if (mrs->valid()) {
    tMRS* mapped_mrs = vpm->map_mrs(mrs, true);
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

*/

void MrxMrsPrinter::print(tMrs* mrs) {
  *_out << "<mrs>\n";
  *_out << boost::format("<label vid='%d'/>") % mrs->ltop->id;
  *_out << boost::format("<var vid='%d'/>\n") % mrs->index->id;
  for (std::vector<tBaseEp*>::iterator ep = mrs->eps.begin();
       ep != mrs->eps.end(); ++ep) {
    print((tEp*)(*ep));
  }
  for (std::vector<tHCons*>::iterator hcons = mrs->hconss.begin();
       hcons != mrs->hconss.end(); ++hcons) {
    print(*hcons);
  }
  *_out << "</mrs>\n";
}

void MrxMrsPrinter::print(tEp* ep) {
  *_out << boost::format("<ep cfrom='%d' cto='%d'>") % ep->cfrom % ep->cto;
  if (ep->pred[0] == '"')
    *_out << "<spred>" << xml_escape(ep->pred.substr(1, ep->pred.length()-2))
          << "</spred>";
  else {
    char* uppred = new char[ep->pred.length()+1];
    strtoupper(uppred, ep->pred.c_str());
    *_out << "<pred>" << xml_escape(uppred) << "</pred>"; //pred;
    delete uppred;
  }
  *_out << boost::format("<label vid='%d'/>") % ep->label->id;
  std::list<std::string> feats;
  for (std::map<std::string, tValue*>::iterator role = ep->roles.begin();
       role != ep->roles.end(); ++role)
    feats.push_back((*role).first);
//  feats.sort(ltfeat());
  for (std::list<std::string>::iterator feat = feats.begin();
       feat != feats.end(); ++feat) {
    *_out << std::endl << "<fvpair>";
    *_out << "<rargname>" << xml_escape(*feat) << "</rargname>";
    print_full(ep->roles[*feat]);
    *_out << "</fvpair>";
  }
  *_out << "</ep>" << std::endl;
}

void MrxMrsPrinter::print(tValue* val) {
  if (dynamic_cast<tConstant *>(val) != NULL) {
    print((tConstant*)val);
  } else if (dynamic_cast<tVar *>(val) != NULL) {
    print((tVar*)val);
  }
}

void MrxMrsPrinter::print_full(tValue* val) {
  if (dynamic_cast<tConstant *>(val) != NULL) {
    print((tConstant*)val);
  } else if (dynamic_cast<tVar *>(val) != NULL) {
    print_full((tVar*)val);
  }
}

void MrxMrsPrinter::print(tConstant* constant) {
  if (constant->value[0] == '"')
    *_out << "<constant>" 
      << xml_escape(constant->value.substr(1, constant->value.length()-2))
      << "</constant>";
  else
    *_out << "<constant>" << xml_escape(constant->value) << "</constant>";
}

void MrxMrsPrinter::print(tVar* var) {
    *_out << boost::format("<var vid='%d' sort='%s'></var>") 
      % var->id % xml_escape(var->type);
}

void MrxMrsPrinter::print_full(tVar* var) {
  *_out << boost::format("<var vid='%d' sort='%s'>") 
    % var->id % xml_escape(var->type);
  // _todo_ this should be adapted to handle the feature priority
  std::list<std::string> feats;
  for (std::map<std::string,std::string>::iterator prop 
    = var->properties.begin(); prop != var->properties.end(); ++prop)
    feats.push_back((*prop).first);
//  feats.sort(ltextra());
  for (std::list<std::string>::iterator feat = feats.begin();
       feat != feats.end(); ++feat) {
    char* upvalue = new char[var->properties[*feat].length()+1];
    strtoupper(upvalue, var->properties[*feat].c_str());
    *_out << std::endl << "<extrapair><path>" << xml_escape(*feat)
          << "</path><value>"
          << xml_escape(upvalue) << "</value></extrapair>";
    delete upvalue;
  }
  *_out << "</var>";
}

void MrxMrsPrinter::print(tHCons* hcons) {
  *_out << "<hcons hreln='";
  if (hcons->relation == tHCons::QEQ)
    *_out << "qeq";
  else if (hcons->relation == tHCons::LHEQ)
    *_out << "lheq";
  else if (hcons->relation == tHCons::OUTSCOPES)
    *_out << "outscopes";
  *_out << "'>";
  *_out << "<hi>";
  print(hcons->harg);
  *_out << "</hi>";
  *_out << "<lo>";
  print(hcons->larg);
  *_out << "</lo>";
  *_out << "</hcons>\n";
}

void SimpleMrsPrinter::print(tMrs* mrs) {
  *_out << boost::format(" [ LTOP: %s%d") % mrs->ltop->type % mrs->ltop->id;
  *_out << std::endl << "   INDEX: ";
  print(mrs->index);
  *_out << std::endl << "   RELS: <";
  for (std::vector<tBaseEp*>::iterator ep = mrs->eps.begin();
       ep != mrs->eps.end(); ++ep) {
    *_out << std::endl;
    print((tEp*)(*ep));
  }
  *_out << " >" << std::endl << "   HCONS: <";
  for (std::vector<tHCons*>::iterator hcons = mrs->hconss.begin();
       hcons != mrs->hconss.end(); ++hcons) {
    *_out << " ";
    print(*hcons);
  }
  *_out << " >";
  *_out << " ]" << std::endl;
  _vars.clear();
}

void SimpleMrsPrinter::print(tEp* ep) {
  *_out << boost::format("          [ %s<%d:%d>") 
    % ep->pred % ep->cfrom % ep->cto << std::endl;
  *_out << boost::format("            LBL: %s%d") 
    % ep->label->type % ep->label->id;
  std::list<std::string> feats;
  for (std::map<std::string, tValue*>::iterator role = ep->roles.begin();
       role != ep->roles.end(); ++role) 
    if ((*role).first != "LBL") feats.push_back((*role).first);
//  feats.sort(ltfeat());
  for (std::list<std::string>::iterator feat = feats.begin();
       feat != feats.end(); ++feat) {
    *_out << std::endl << "            " << *feat << ": ";
    print(ep->roles[*feat]);
  }
  *_out << " ]";
}

void SimpleMrsPrinter::print(tValue* val) {
  // _todo_
  if (dynamic_cast<tConstant *>(val)) {
    print((tConstant*)val);
  } else if (dynamic_cast<tVar *>(val)) {
    print((tVar*)val);
  }
}

void SimpleMrsPrinter::print(tConstant* constant) {
  *_out << constant->value;
}

void SimpleMrsPrinter::print(tVar* var) {
  if (_vars.find(var->id) == _vars.end()) { // first time of printing the variable
    print_full(var);
    _vars[var->id] = var;
  } else { // the variable has been printed before
    *_out << boost::format("%s%d") % var->type % var->id;
  }
}

void SimpleMrsPrinter::print_full(tVar* var) {
  if (var->properties.begin() == var->properties.end()) {
    *_out << boost::format("%s%d") % var->type % var->id;
    return;
  }
  *_out << boost::format("%s%d [ %s ") % var->type % var->id % var->type;
  std::list<std::string> feats;
  for (std::map<std::string,std::string>::iterator prop 
    = var->properties.begin(); prop != var->properties.end(); ++prop)
    feats.push_back((*prop).first);
//  feats.sort(ltextra());
  for (std::list<std::string>::iterator feat = feats.begin();
       feat != feats.end(); ++feat) {
    char* upvalue = new char[var->properties[*feat].length()+1];
    strtoupper(upvalue, var->properties[*feat].c_str());
    *_out  << *feat << ": " << upvalue << " ";
    delete upvalue;
  }
  *_out << "]";
}

void SimpleMrsPrinter::print(tHCons* hcons) {
  print(hcons->harg);
  if (hcons->relation == tHCons::QEQ)
    *_out << " qeq ";
  else if (hcons->relation == tHCons::LHEQ)
    *_out << " lheq ";
  else if (hcons->relation == tHCons::OUTSCOPES)
    *_out << " outscopes ";
  print(hcons->larg);
}

void HtmlMrsPrinter::print(tMrs* mrs) {
  *_out <<"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n";
  *_out << "<html>";
  *_out << getHeader();
  *_out << "<body>\n<table class=mrsMrs>\n";
//  *_out << "<caption>" << item_id << ": " << item_string << "</caption>\n";
  *_out << "<tr><td>TOP</td>\n<td>\n";
  print_full(mrs->ltop);
  *_out << "</td>\n<tr>\n<td>INDEX</td>\n<td>\n";
  print_full(mrs->index);
  *_out << "</td>\n<tr>\n<td>RELS</td>\n<td>\n<table class=mrsRelsContainer>\n";
  *_out << "<tr>\n<td valign=middle><span class=mrsBracket>{</span></td>\n";
  *_out << "<td><table class=mrsRelsContainer><tr>\n";
  *_out << "<td><table class=mrsRelsContainer><tr>\n";
  int epcount = 0;
  for (std::vector<tBaseEp*>::iterator ep = mrs->eps.begin();
       ep != mrs->eps.end(); ++ep) {
    if (epcount > 0 && epcount % 6 == 0) {
      *_out << "</tr></table></td></tr>\n";
      *_out << "<tr><td><table class=mrsRelsContainer><tr>\n";
    }
    print((tEp*)(*ep));
    epcount++;

  }
  *_out << "</tr></table></td></tr></table>\n";
  *_out << "<td valign=middle><span class=mrsBracket>}</span></td>\n";
  *_out << "</table>\n</td>\n<tr><td>HCONS</td>\n<td class=mrsValueHcons>{ ";
  for (std::vector<tHCons*>::iterator hcons = mrs->hconss.begin();
       hcons != mrs->hconss.end(); ++hcons) {
    if (hcons != mrs->hconss.begin()) *_out << ", ";
    print(*hcons);
  }
  *_out << " }</td></tr></table\n<script>";
  for (std::vector<tHCons*>::iterator hcons = mrs->hconss.begin();
       hcons != mrs->hconss.end(); ++hcons) {
     *_out << "mrsHCONSsForward[\'0";
     print((*hcons)->harg);
     *_out << "\'] = \'0";
     print((*hcons)->larg);
     *_out << "\';\nmrsHCONSsBackward[\'0";
     print((*hcons)->larg);
     *_out << "\'] = \'0";
     print((*hcons)->harg);
     *_out << "\';\n";
  }
  *_out << "</script>\n<div id=\"navtxt\" class=\"navtext\" style=";
  *_out << "\"position:absolute; top:-100px; left:0px; visibility:hidden;\">";
  *_out << "</div>\n</body>\n</html>\n";
}

void HtmlMrsPrinter::print(tEp* ep) {
  *_out << "<td><table class=mrsRelation>\n";
  *_out << "<tr><td class=mrsPredicate colspan=2>" << ep->pred << ep->link;
  *_out << "</td>\n<tr><td>LBL</td><td class=mrsValue>\n";
  print_full(ep->label);
  *_out << "</td>";
  std::list<std::string> feats;
  for (std::map<std::string, tValue*>::iterator role = ep->roles.begin();
       role != ep->roles.end(); ++role)
    if ((*role).first != "LBL") feats.push_back((*role).first);
//  feats.sort(ltfeat());
  for (std::list<std::string>::iterator feat = feats.begin();
       feat != feats.end(); ++feat) {
    *_out << "<tr><td>" << *feat << "</td><td class=mrsValue>\n";
    print_full(ep->roles[*feat]);
    *_out << "</td></tr>\n";
  }
  for (std::map<std::string, tConstant*>::iterator carg 
      = ep->parameter_strings.begin(); 
      carg != ep->parameter_strings.end(); ++carg) {
    *_out << "<tr><td>" << carg->first << "</td><td class=mrsValue>\n";
    *_out << carg->second->value << "</td></tr>\n";
  }
  *_out << "</table></td>\n";
}

void HtmlMrsPrinter::print(tValue* val) {
  if (dynamic_cast<tConstant *>(val) != NULL) {
    print((tConstant*)val);
  } else if (dynamic_cast<tVar *>(val) != NULL) {
    print((tVar*)val);
  }
}

void HtmlMrsPrinter::print_full(tValue* val) {
  if (dynamic_cast<tConstant *>(val) != NULL) {
    print((tConstant*)val);
  } else if (dynamic_cast<tVar *>(val) != NULL) {
    print_full((tVar*)val);
  }
}

void HtmlMrsPrinter::print(tConstant* constant) {
  *_out << constant->value;
}

void HtmlMrsPrinter::print(tVar* var) {
    *_out << var->type << var->id;
}

void HtmlMrsPrinter::print_full(tVar* var) {
  *_out << "<div class=\"mrsVariable0" << var->type << var->id << "\"\n";
  *_out << "onMouseOver=\"mrsVariableSelect(\'0" << var->type << var->id 
    << "\',";
  std::list<std::string> feats;
  for (std::map<std::string,std::string>::iterator prop 
    = var->properties.begin(); prop != var->properties.end(); ++prop)
    feats.push_back((*prop).first);
//  feats.sort(ltextra());
  if (feats.empty()) {
    *_out << " '')\"\n";
  } else {
    *_out << " '<table class=mrsProperties>";
    for (std::list<std::string>::iterator feat = feats.begin();
        feat != feats.end(); ++feat) {
      *_out << "<tr><td class=mrsPropertyFeature>";
      *_out << *feat << "</td><td class=mrsPropertyValue>";
      *_out << var->properties[*feat] << "</td></tr>";
    }
    *_out << "</table>')\"\n";
  }
  *_out << "onMouseOut=\"mrsVariableUnselect('0" << var->type << var->id;
  *_out << "')\">" << var->type << var->id << "</div>\n";
}

void HtmlMrsPrinter::print(tHCons* hcons) {
  *_out << "<span class=\"mrsVariable0"; 
  print(hcons->harg);
  *_out << "\" onMouseOver=\"mrsVariableSelect('0";
  print(hcons->harg);
  *_out << "', '')\" onMouseOut=\"mrsVariableUnselect('0";
  print(hcons->harg);
  *_out << "')\">";
  print(hcons->harg);
  *_out << "</span>&thinsp;";
  if (hcons->relation == tHCons::QEQ)
    *_out << "=q";
  else if (hcons->relation == tHCons::LHEQ)
    *_out << "lheq";
  else if (hcons->relation == tHCons::OUTSCOPES)
    *_out << "outscopes";
  *_out << "&thinsp;<span class=\"mrsVariable0";
  print(hcons->larg);
  *_out << "\" onMouseOver=\"mrsVariableSelect('0";
  print(hcons->larg);
  *_out << "', '')\" onMouseOut=\"mrsVariableUnselect('0";
  print(hcons->larg);
  *_out << "')\">";
  print(hcons->larg);
  *_out << "</span>";
}

std::string HtmlMrsPrinter::getHeader()
{
  std::string header("<head>\n<meta http-equiv=\"Content-Type\" ");
  header += "content=\"text/html ; charset=utf8\">\n";
  if (_inline_headers) {
  //TODO
  } else {
    header += "<link TYPE=\"text/css\" REL=\"stylesheet\" HREF=\"mrs.css\">\n";
    header += "<script SRC=\"mrs.js\" LANGUAGE=\"javascript\" ";
    header += "TYPE=\"text/javascript\"></script>\n";
    header += "<script SRC=\"hover.js\" LANGUAGE=\"javascript\" ";
    header += "TYPE=\"text/javascript\"></script>\n";
  }
  header += "</meta>";
  //*_out << "<title>"<< item_id << ": " << item_string << "</title>\n";
  header += "</head>\n";

  return header;
}

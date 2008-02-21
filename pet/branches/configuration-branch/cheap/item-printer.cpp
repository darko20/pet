#include "item-printer.h"
#include "dagprinter.h"
#include <sstream>

using namespace std;

// ----------------------------------------------------------------------
// Default item printer 
// ----------------------------------------------------------------------

void tItemPrinter::print_tofill(const tItem *item) {
  if (item->passive()) return; 
  list_int *l;
  _out << item->nextarg() ;
  for (l = item->restargs(); l != NULL; l = rest(l)) {
    _out << " " << first(l);
  }
}

void tItemPrinter::print_inflrs(const tItem *item) {
  const list_int *l = inflrs_todo(item);
  if (l != NULL) { _out << print_name(first(l)) ; l = rest(l); }
  for (; l != NULL; l = rest(l)) {
    _out << " " << print_name(first(l)) ;
  }
}

void tItemPrinter::print_paths(const tItem *item) {
  const  list<int> &paths = get_paths(item).get();
  list<int>::const_iterator it = paths.begin();
  if (it != paths.end()) { _out << *it; ++it; }
  for(; it != paths.end(); ++it) {
    _out << " " << *it;
  }
}

void tItemPrinter::print_packed(const tItem *item) {
  const itemlist &packed = item->packed;
  if(packed.size() == 0) return;
  
  _out << " < packed: ";
  for(itemlist_citer pos = packed.begin(); pos != packed.end(); ++pos)
    _out << get_id(*pos) << " ";
  _out << ">";
}

void tItemPrinter::print_family(const tItem *item) {
  const itemlist &items = item->daughters();
  _out << " < dtrs: ";
  for(itemlist_citer pos = items.begin(); pos != items.end(); ++pos)
    _out << get_id(*pos) << " ";
  _out << "parents: ";
  const itemlist &parents = item->parents;
  for(itemlist_citer pos = parents.begin(); pos != parents.end(); ++pos)
    _out << get_id(*pos) << " ";
  _out << ">";
}

void tItemPrinter::print_common(const tItem *item) {
  _out << "[" << item->id() 
       << " " << item->start() << "-" << item->end() << " " 
       << get_fs(item).printname() << " (" << item->trait() << ") "
       << std::setprecision(4) << item->score()
       << " {";
  print_tofill(item);
  _out << "} {";
  print_inflrs(item);
  _out << "} {";
  print_paths(item);
  _out << "}]";
  print_family(item);
  print_packed(item);
  
  /*
  if(verbosity > 2 && compact == false) {
    print_derivation(iph, false);
  }
#ifdef LUI
  lui_dump();
#endif
  */
}

void tItemPrinter::real_print(const tInputItem *item) {
  // [bmw] print also start/end nodes
  _out << "[n" << item->start() << " - n" << item->end() << "] ["
       << item->startposition() << " - " << item->endposition() << "] ("
       << item->external_id() << ") \"" 
       << item->stem() << "\" \"" << item->form() << "\" ";
  //pbprintf(iph, "[n%d - n%d] [%d - %d] (%s) \"%s\" \"%s\" "
  //    , _start, _end, _startposition, _endposition , _input_id.c_str()
  //    , _stem.c_str(), _surface.c_str());


  _out << "{";
  print_inflrs(item);
  _out << "} ";

  // fprintf(f, "@%d", inflpos);

  _out << "{";
  item->get_in_postags().print(_out);
  _out << "}";
}

void tItemPrinter::print_fs(const fs &f) {
  if (_dag_printer == NULL) {
    _dag_printer = new ReadableDagPrinter();
  }
  f.print(_out, *_dag_printer);
}

void tItemPrinter::real_print(const tLexItem *item) {
  _out << "L ";
  print_common(item);
  if (_print_fs_p) {
    // \todo cast is not nice
    print_fs(const_cast<tLexItem *>(item)->get_fs());
  }
}

void tItemPrinter::real_print(const tPhrasalItem *item) {
  _out << "P ";
  print_common(item);
  if (_print_fs_p) {
    // \todo cast is not nice
    print_fs(const_cast<tPhrasalItem *>(item)->get_fs());
  }
}

// ----------------------------------------------------------------------
// TCL chart item printer 
// ----------------------------------------------------------------------

const char tcl_chars_to_escape[] = { '\\', '"', '\0' };

void print_string_escaped(std::ostream &out, const char *str
                          , const char *to_escape, char esc_char = '\\'){
  const char *next, *pos;
  while ((next = strpbrk(str, to_escape)) != NULL) {
    pos = str; 
    while (pos < next) {
      out << *pos; 
      pos ++;
    }
    out << esc_char << *next;
    str = next + 1;
    next = strpbrk(str, to_escape);
  }
  out << str;
}

// it is possible to define functions to private functionality of the items
// here to avoid multiple friend class entries in the items. The private
// fields or methods can then be accessed via the superclass

void
tTclChartPrinter::print_it(const tItem *item, bool passive, bool left_ext){
  const list<tItem*> &dtrs = item->daughters();
  item_map::iterator it = _items.find((long int) item);
  if (it == _items.end()) {
    list<tItem *>::const_iterator dtr;
    for(dtr = dtrs.begin(); dtr != dtrs.end(); dtr++) {
      print(*dtr);
    }
    _out << "draw_edge " << _chart_id << " " << _item_id 
         << " " << item->start() << " " << item->end()
         << " \"";
    print_string_escaped(_out, item->printname(), tcl_chars_to_escape);
    _out << "\" " << (passive ? "0" : "1") << (left_ext ? " 0" : " 1")
         << " { ";
    for(dtr = dtrs.begin(); dtr != dtrs.end(); dtr++) {
      _out << _items[(long int) *dtr] << " ";
    }
    _out << "}" << std::endl;
    _items[(long int) item] = _item_id;
    _item_id++;
  }
}

/* ------------------------------------------------------------------------- */
/* ----------------------- CompactDerivationPrinter ------------------------ */
/* ------------------------------------------------------------------------- */

void
tCompactDerivationPrinter::real_print(const tInputItem *item) {
  _out << "(" 
       << (_quoted ? "\\\"" : "\"")
       << item->orth()
       << (_quoted ? "\\\" " : "\" ")
       << std::setprecision(4) << item->score() << " "
       << item->start() << " " << item->end()
       << ")";
  /*
    fprintf (_out, "(%s\"%s%s\" %.4g %d %d)"
    , _quoted ? "\\" : "", item->orth().c_str(), _quoted ? "\\" : ""
    , item->score(), item->start(), item->end());
  */
}

void 
tCompactDerivationPrinter::print_inflrs(const tItem* item) {
  _out << " [";
  const list_int *l = inflrs_todo(item);
  if (l != 0) { _out << print_name(first(l)); l = rest(l);}
  for(; l != 0; l = rest(l))
    _out << " " << print_name(first(l));
  _out << "]";
}

void 
tCompactDerivationPrinter::print_daughters(const tItem* item) {
  for(list<tItem *>::const_iterator pos = item->daughters().begin();
      pos != item->daughters().end(); ++pos)
    print(*pos);
}

void 
tCompactDerivationPrinter::real_print(const tLexItem *item) {
  _out << "(" << item->id() << " " << stem(item)->printname() << "/"
       << print_name(const_cast<tLexItem *>(item)->type()) << " "
       << std::setprecision(4) << item->score() << " "
       << item->start() << " " << item->end();
  /*
  fprintf(_out, "(%d %s/%s %.4g %d %d "
           , item->id(), stem(item)->printname()
           , print_name(const_cast<tLexItem *>(item)->type()), item->score()
           , item->start(), item->end());
  fprintf(_out, "[");
  */
  /*out << " [";
  const list_int *l = inflrs_todo(item);
  if (l != 0) { _out << print_name(first(l)); l = rest(l);}
  for(; l != 0; l = rest(l))
    _out << " " << print_name(first(l));
  //fprintf(_out, "]");
  _out << "]";
  */
  print_inflrs(item);

  print_daughters(item);
  /*
  for(list<tItem *>::const_iterator pos = item->daughters().begin();
      pos != item->daughters().end(); ++pos)
    print(*pos);
  */
  _out << ")";
  //fprintf(_out, ")");
}

void 
tCompactDerivationPrinter::real_print(const tPhrasalItem *item) {
  _out << "(" << item->id() << " " << item->printname()
       << std::setprecision(4) << item->score() << " "
       << item->start() << " " << item->end();
  /*
  fprintf(_out, "(%d %s %.4g %d %d"
          , item->id(), item->printname(), item->score()
          , item->start(), item->end());
  */
  if(item->packed.size()) {
    _out << " {";
    //fprintf(_out, " {");
    list<tItem *>::const_iterator pack = item->packed.begin();
    if (pack != item->packed.end()) { _out << (*pack)->id(); ++pack; }
    for(; pack != item->packed.end(); ++pack) {
      _out << " " <<  (*pack)->id();
      //fprintf(_out, "%s%d", pack == item->packed.begin() ? "" : " ", (*pack)->id()); 
    }
    _out << "}";
    //fprintf(_out, "}");
  }

  if(result_root(item) != -1) {
    _out << " [" << print_name(result_root(item)) << "]";
    //fprintf(_out, " [%s]", print_name(result_root(item)));
  }
  
  if(inflrs_todo(item)) {
    /* fprintf(_out, " [");
    for(const list_int *l = inflrs_todo(item); l != 0; l = rest(l)) {
      fprintf(_out, "%s%s", print_name(first(l)), rest(l) == 0 ? "" : " ");
    }
    fprintf(_out, "]");*/
    print_inflrs(item);
  }

  print_daughters(item);
  /*
  for(list<tItem *>::const_iterator pos = item->daughters().begin();
      pos != item->daughters().end(); ++pos) {
    print(*pos);
  }
  */
  
  _out << ")";
  //fprintf(_out, ")");
}

/* ------------------------------------------------------------------------- */
/* ---------------------- DelegateDerivationPrinter ------------------------ */
/* ------------------------------------------------------------------------- */


void
tDelegateDerivationPrinter::real_print(const tInputItem *item) {
  _itemprinter.print(item);
}

void 
tDelegateDerivationPrinter::real_print(const tLexItem *item) {
  //fprintf(_out, "(");
  _itemprinter.print(item);
  print(keydaughter(item));
  // fprintf(_out, ")");
}

void 
tDelegateDerivationPrinter::real_print(const tPhrasalItem *item) {
  _itemprinter.print(item);

  for(list<tItem *>::const_iterator pos = item->daughters().begin();
      pos != item->daughters().end(); ++pos) {
    print(*pos);
  }
}

void tFSPrinter::print(const tItem *arg) {
  const_cast<tItem *>(arg)->get_fs().print(_out, _dag_printer);
}

void tFegramedPrinter::print(const dag_node *dag, const char *name) { 
  open_stream(name);
  //dag_print_fed_safe(_out, const_cast<dag_node *>(dag));
  FegramedDagPrinter fp;
  fp.print(_out, dag);
  close_stream();
}

void tFegramedPrinter::print(const tItem *arg, const char *name) { 
  if (name == NULL) {
    std::ostringstream out;
    out << arg->printname() 
        << "-" << arg->start() << "-" << arg->end() << "-";
    name = out.str().c_str();
  }
  open_stream(name);
  fp.print(_out, const_cast<tItem *>(arg)->get_fs().dag());
  close_stream();
}


void 
tJxchgPrinter::print(const tItem *arg) { arg->print_gen(this); }


/** Print the yield of an input item, recursively or not */
void tJxchgPrinter::print_yield(const tInputItem *item) {
  if (item->daughters().empty()) {
    _out << '"' << item->form() << '"';
  } else {
    itemlist_citer it = item->daughters().begin();
    print(*it);
    while(++it != item->daughters().end()) {
      _out << " ";
      print(*it++);
    }
  }
}

/** Input items don't print themselves */
void tJxchgPrinter::real_print(const tInputItem *item) { }

void 
tJxchgPrinter::real_print(const tLexItem *item) {
  _out << item->id() << " " << item->start() << " " << item->end() << " "
    // print the HPSG type this item stems from
       << print_name(item->stem()->type()) << "[";
  for(const list_int *l = inflrs_todo(item); l != 0; l = rest(l)) {
    _out << print_name(first(l));
    if (rest(l) != 0) _out << " ";
  }
  _out << "] ( "; 
  // This prints only the yield, because all daughters are tInputItems
  for(itemlist_citer it = item->daughters().begin();
      it != item->daughters().end(); it++)
    print_yield(dynamic_cast<tInputItem *>(*it));
  _out << " ) ";
  jxchgprinter.print(_out, get_fs(item).dag());
  _out << std::endl;
}

void 
tJxchgPrinter::real_print(const tPhrasalItem *item) {
  _out << item->id() << " " << item->start() << " " << item->end() << " "
    // print the rule type, if any
       << print_name(item->identity()) 
       << " (";
  for(itemlist_citer it = item->daughters().begin();
      it != item->daughters().end() ; it++) {
    _out << " " << (*it)->id();
  }
  _out << " ) ";
  jxchgprinter.print(_out, get_fs(item).dag());
  _out << std::endl;
}

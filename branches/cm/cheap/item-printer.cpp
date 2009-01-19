#include "item-printer.h"

#include <sstream>

using namespace std;
using namespace HASH_SPACE;

// ----------------------------------------------------------------------
// Default item printer, was t???Item::print(FILE *)
// ----------------------------------------------------------------------

tItemPrinter::tItemPrinter(ostream &out, bool print_derivation, bool print_fs)
  : _out(out), 
    _derivation_printer(print_derivation 
                        ? new tCompactDerivationPrinter(out)
                        : NULL),
    _dag_printer(print_fs
                 ? new ReadableDagPrinter()
                 : NULL) {}

tItemPrinter::~tItemPrinter() {
  delete _derivation_printer;
  delete _dag_printer;
}

// helper function from tItem::print
inline void tItemPrinter::print_tofill(ostream &out, const tItem *item) {
  if (item->passive()) return; 
  list_int *l;
  out << item->nextarg() ;
  for (l = item->restargs(); l != NULL; l = rest(l)) {
    out << " " << first(l);
  }
}

// helper function from tItem::print, complete
inline void tItemPrinter::print_inflrs(ostream &out, const tItem *item) {
  const list_int *l = inflrs_todo(item);
  if (l != NULL) { out << print_name(first(l)) ; l = rest(l); }
  while (l != NULL) {
    out << " " << print_name(first(l)) ; l = rest(l);
  }
}

// helper function from tItem::print
inline void tItemPrinter::print_paths(ostream &out, const tItem *item) {
  const list<int> &paths = get_paths(item).get();
  list<int>::const_iterator it = paths.begin();
  if (it != paths.end()) { out << *it; ++it; }
  while(it != paths.end()) {
    out << " " << *it; ++it;
  }
}

// former tItem::print_packed (complete)
inline void tItemPrinter::print_packed(ostream &out, const tItem *item) {
  const item_list &packed = item->packed;
  if(packed.size() == 0) return;
  
  out << " < packed: ";
  for(item_citer pos = packed.begin(); pos != packed.end(); ++pos)
    out << get_id(*pos) << " ";
  out << ">";
}

// former tItem::print_family
inline void tItemPrinter::print_family(ostream &out, const tItem *item) {
  const item_list &items = item->daughters();
  out << " < dtrs: ";
  for(item_citer pos = items.begin(); pos != items.end(); ++pos)
    out << get_id(*pos) << " ";
  out << " parents: ";
  const item_list &parents = item->parents;
  for(item_citer pos = parents.begin(); pos != parents.end(); ++pos)
    out << get_id(*pos) << " ";
  out << ">";
}

// former tItem::print
void tItemPrinter::print_common(ostream &out, const tItem *item) {
  out << "[" << item->id() 
       << " " << item->start() << "-" << item->end() << " " 
       << get_fs(item).printname() << " (" << item->trait() << ") "
       << std::setprecision(4) << item->score()
       << " {";
  print_tofill(out, item);
  out << "} {";
  print_inflrs(out, item);
  out << "} {";
  print_paths(out, item);
  out << "}]";
  print_family(out, item);
  print_packed(out, item);
  
  if(_derivation_printer != NULL) {
    _derivation_printer->print(item);
  }
}

// from tInputItem::print
void tItemPrinter::real_print(const tInputItem *item) {
  _out << "I [" << item->id() 
       << " " << item->start() << "-" << item->end() << " (" 
       << item->external_id() << ") <" 
       << item->startposition() << ":" << item->endposition() << "> \""
       << item->form() << "\" \"" << item->stem() << "\" ";

  _out << "{";
  print_inflrs(_out, item);
  _out << "} ";

  // fprintf(f, "@%d", inflpos);

  _out << "{";
  item->get_in_postags().print(_out);
  _out << "}]";
  if (_dag_printer)
    // _fix_me_
    // as i understand it, one should in principle use the following here (not
    // the explicit const_cast<>, with the correct item type):
    //   print_fs(_out, get_fs(item));
    // however, when doing so i seem to end up with completely underspecified
    // feature structures (`*top*'), possibly because the wrong virtual method
    // gets invoked?                                           (15-sep-08; oe)
    //
    print_fs(_out, const_cast<tInputItem *>(item)->get_fs());
  _out << endl;
}

inline void tItemPrinter::print_fs(ostream &out, const fs &f) {
  out << endl;
  f.print(out, *_dag_printer);
}

void tItemPrinter::real_print(const tLexItem *item) {
  _out << "L ";
  print_common(_out, item);
  if (_dag_printer != NULL) {
    print_fs(_out, get_fs(item));
  }
}

void tItemPrinter::real_print(const tPhrasalItem *item) {
  _out << "P ";
  print_common(_out, item);
  if (_dag_printer != NULL) {
    print_fs(_out, get_fs(item));
  }
}

/** default printing for chart items: use a tItemPrinter */
std::ostream &operator<<(std::ostream &out, const tItem &item) {
  tItemPrinter printer(out, verbosity > 2, verbosity > 10);
  printer.print(&item);
  return out;
}

// ---------------------------------------------------------------------- //
// ----------------------- TCL chart item printer ----------------------- //
// ---------------------------------------------------------------------- //

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
    item_citer dtr;
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

// former tInputItem::print_derivation
void
tCompactDerivationPrinter::real_print(const tInputItem *item) {
  _out << "(" 
       << (_quoted ? "\\\"" : "\"")
       << item->orth()
       << (_quoted ? "\\\" " : "\" ")
       << std::setprecision(4) << item->score() << " "
       << item->start() << " " << item->end() << " <"
       << item->startposition() << ":" << item->endposition()
       << ">)";
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
  for(item_citer pos = item->daughters().begin();
      pos != item->daughters().end(); ++pos)
    print(*pos);
}

void 
tCompactDerivationPrinter::print_daughters_same_line(const tItem* item) {
  for(item_citer pos = item->daughters().begin();
      pos != item->daughters().end(); ++pos) {
    _out << " "; (*pos)->print_gen(this);
  }
}

// former tLexItem::print_derivation
void 
tCompactDerivationPrinter::real_print(const tLexItem *item) {
  _out << "(" << item->id() << " " << stem(item)->printname() << "/"
       << print_name(item->type()) << " "
       << std::setprecision(4) << item->score() << " "
       << item->start() << " " << item->end();

  print_inflrs(item);

  // \todo this was _keydaughter->print_derivation(f, quoted). Why?
  print_daughters(item);
  // this omits one newline, like the original ::print_derivation
  //print_daughters_same_line(item);

  _out << ")";
}

// former tPhrasalItem::print_derivation
void 
tCompactDerivationPrinter::real_print(const tPhrasalItem *item) {
  _out << "(" << item->id() << " " << item->printname() << " "
       << std::setprecision(4) << item->score() << " "
       << item->start() << " " << item->end();
  if(item->packed.size() > 0) {
    _out << " {";
    item_citer pack = item->packed.begin();
    if (pack != item->packed.end()) { _out << (*pack)->id(); ++pack; }
    for(; pack != item->packed.end(); ++pack) {
      _out << " " <<  (*pack)->id();
    }
    _out << "}";
  }

  if(result_root(item) != -1) {
    _out << " [" << print_name(result_root(item)) << "]";
  }
  
  if(inflrs_todo(item) != NULL) {
    print_inflrs(item);
  }

  print_daughters(item);
  
  _out << ")";
}

/* ------------------------------------------------------------------------- */
/* ------------------------ TSDBDerivationPrinter -------------------------- */
/* ------------------------------------------------------------------------- */

void tTSDBDerivationPrinter::real_print(const tInputItem *item) {
  _out << "(\"" << escape_string(item->orth()) 
       << "\" " << item->start() << " " << item->end() << "))" << flush;
}

void tTSDBDerivationPrinter::real_print(const tLexItem *item) {
  _out << "(" << item->id() << " " << item->stem()->printname()
       << " " << item->score() << " " << item->start() <<  " " << item->end()
       << " " << "(\"" << escape_string(item->orth()) << "\" "
       << item->start() << " " << item->end() << "))" << flush; 
}

void tTSDBDerivationPrinter::real_print(const tPhrasalItem *item) {
  if(item->result_root() > -1) 
    _out << "(" << print_name(item->result_root()) << " ";
  
  _out << "(" << item->id() << " " << item->printname() << " " << item->score()
       << " " << item->start() << " " << item->end();
  
  const item_list &daughters = item->daughters();
  for(item_citer pos = daughters.begin(); pos != daughters.end(); ++pos) {
    _out << " ";
    if(_protocolversion == 1)
      (*pos)->print_gen(this);
    else
      _out << (*pos)->id();
  }

  _out << (item->result_root() > -1 ? "))" : ")") << flush;
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

  for(item_citer pos = item->daughters().begin();
      pos != item->daughters().end(); ++pos) {
    print(*pos);
  }
}

/* ------------------------------------------------------------------------- */
/* -------------------------- FS/FegramedPrinter --------------------------- */
/* ------------------------------------------------------------------------- */

void tFSPrinter::print(const tItem *arg) {
  get_fs(arg).print(_out, _dag_printer);
}

void tFegramedPrinter::print(const dag_node *dag, const char *name) { 
  open_stream(name);
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
  fp.print(_out, get_fs(arg).dag());
  close_stream();
}


/* ------------------------------------------------------------------------- */
/* ----------------------------- JxchgPrinter ------------------------------ */
/* ------------------------------------------------------------------------- */

void 
tJxchgPrinter::print(const tItem *arg) { arg->print_gen(this); }


/** Print the yield of an input item, recursively or not */
void tJxchgPrinter::print_yield(const tInputItem *item) {
  if (item->daughters().empty()) {
    _out << '"' << item->form() << '"';
  } else {
    item_citer it = item->daughters().begin();
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
  for(item_citer it = item->daughters().begin(); it != item->daughters().end()
        ; it++)
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
  for(item_citer it = item->daughters().begin(); it != item->daughters().end()
        ; it++) {
    _out << " " << (*it)->id();
  }
  _out << " ) ";
  jxchgprinter.print(_out, get_fs(item).dag());
  _out << std::endl;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------ tLUIPrinter ------------------------------ */
/* ------------------------------------------------------------------------- */

void 
tLUIPrinter::print(const tItem *item) {
  ostringstream fn;
  fn << _path << item->id() << ".lui";
  ofstream stream(fn.str().c_str());
  stream << "avm " << item->id() << " ";
  if (! stream) throw tError("File open error for file " + fn.str());
  get_fs(item).print(stream, _dagprinter);
  stream << " \"Edge # " << item->id() << "\"\f\n" ;
  stream.close();
}

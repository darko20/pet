#include "item-printer.h"
#include <sstream>

using namespace std;
using namespace HASH_SPACE;

// ----------------------------------------------------------------------
// stream operators
// ----------------------------------------------------------------------

/*
tAbstractItemPrinter &operator<<(std::ostream &out, tAbstractItemPrinter &ip) {
  ip.set_ostream(out);
  return ip;
}
std::ostream &operator<<(tAbstractItemPrinter &ip, const tItem &item) {
  ip.print(item);
  return ip.get_ostream();
}
*/

// ----------------------------------------------------------------------
// Default item printer, was t???Item::print(FILE *)
// ----------------------------------------------------------------------

tItemPrinter::tItemPrinter(ostream &out, bool print_derivation, bool print_fs)
  : tAbstractItemPrinter(out), 
    _derivation_printer(print_derivation 
                        ? new tCompactDerivationPrinter(out)
                        : NULL),
    _dag_printer(print_fs
                 ? new ReadableDagPrinter()
                 : NULL) {}

tItemPrinter::tItemPrinter(bool print_derivation, bool print_fs)
  : tAbstractItemPrinter(), 
    _derivation_printer(print_derivation 
                        ? new tCompactDerivationPrinter()
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
void tItemPrinter::real_print(const tInputItem *item, int level) {
  *_out << "[n" << item->start() << " - n" << item->end() << "] ["
       << item->startposition() << " - " << item->endposition() << "] ("
       << item->external_id() << ") \"" 
       << item->stem() << "\" \"" << item->form() << "\" ";

  *_out << "{";
  print_inflrs(*_out, item);
  *_out << "} ";

  // fprintf(f, "@%d", inflpos);

  *_out << "{";
  item->get_in_postags().print(*_out);
  *_out << "}";
  if (_dag_printer) {
    // _fix_me_
    // as i understand it, one should in principle use the following here (not
    // the explicit const_cast<>, with the correct item type):
    //   print_fs(_out, get_fs(item));
    // however, when doing so i seem to end up with completely underspecified
    // feature structures (`*top*'), possibly because the wrong virtual method
    // gets invoked?                                           (15-sep-08; oe)
    //
    print_fs(*_out, const_cast<tInputItem *>(item)->get_fs());
  }
}

inline void tItemPrinter::print_fs(ostream &out, const fs &f) {
  out << endl;
  f.print(out, *_dag_printer);
}

void tItemPrinter::real_print(const tLexItem *item, int level) {
  *_out << "L ";
  print_common(*_out, item);
  if (_dag_printer != NULL) {
    print_fs(*_out, get_fs(item));
  }
}

void tItemPrinter::real_print(const tPhrasalItem *item, int level) {
  *_out << "P ";
  print_common(*_out, item);
  if (_dag_printer != NULL) {
    print_fs(*_out, get_fs(item));
  }
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
    *_out << "draw_edge " << _chart_id << " " << _item_id 
         << " " << item->start() << " " << item->end()
         << " \"";
    print_string_escaped(*_out, item->printname(), tcl_chars_to_escape);
    *_out << "\" " << (passive ? "0" : "1") << (left_ext ? " 0" : " 1")
         << " { ";
    for(dtr = dtrs.begin(); dtr != dtrs.end(); dtr++) {
      *_out << _items[(long int) *dtr] << " ";
    }
    *_out << "}" << std::endl;
    _items[(long int) item] = _item_id;
    _item_id++;
  }
}

/* ------------------------------------------------------------------------- */
/* ----------------------- CompactDerivationPrinter ------------------------ */
/* ------------------------------------------------------------------------- */

// former tInputItem::print_derivation
void
tCompactDerivationPrinter::real_print(const tInputItem *item, int level) {
  *_out << "("
       << item->id() << " "
       << (_quoted ? "\\\"" : "\"")
       << item->orth()
       << (_quoted ? "\\\" " : "\" ")
       << std::setprecision(4) << item->score() << " "
       << item->start() << " " << item->end() << " <"
       << item->startposition() << ":" << item->endposition()
       << ">" << ")";
}

void 
tCompactDerivationPrinter::print_inflrs(const tItem* item) {
  *_out << " [";
  const list_int *l = inflrs_todo(item);
  if (l != 0) { *_out << print_name(first(l)); l = rest(l);}
  for(; l != 0; l = rest(l))
    *_out << " " << print_name(first(l));
  *_out << "]";
}

void 
tCompactDerivationPrinter::print_daughters(const tItem* item, int level) {
  for(item_citer pos = item->daughters().begin();
      pos != item->daughters().end(); ++pos)
    print(*pos, level + 1);
}

// former tLexItem::print_derivation
void 
tCompactDerivationPrinter::real_print(const tLexItem *item, int level) {
  *_out << (item->trait() == PCFG_TRAIT ? "(* " : "(")
       << item->id() << " "
       << stem(item)->printname() << "/"
       << print_name(item->type()) << " "
       << std::setprecision(4) << item->score() << " "
       << item->start() << " " << item->end();

  print_inflrs(item);
  // \todo this was _keydaughter->print_derivation(f, quoted). Why?
  print_daughters(item, level);
  *_out << ")";
}

// former tPhrasalItem::print_derivation
void 
tCompactDerivationPrinter::real_print(const tPhrasalItem *item, int level) {
  *_out << (item->trait() == PCFG_TRAIT ? "(* " : "(")
       << item->id() << " "
       << item->printname() << " "
       << std::setprecision(4) << item->score() << " "
       << item->start() << " " << item->end();
  if(item->packed.size() > 0) {
    *_out << " {";
    item_citer pack = item->packed.begin();
    if (pack != item->packed.end()) { *_out << (*pack)->id(); ++pack; }
    for(; pack != item->packed.end(); ++pack) {
      *_out << " " <<  (*pack)->id();
    }
    *_out << "}";
  }

  if(!level && result_root(item) != -1) {
    *_out << " [" << print_name(result_root(item)) << "]";
  }
  
  if(inflrs_todo(item) != NULL) {
    print_inflrs(item);
  }
 
  print_daughters(item);
  
  *_out << ")";
}

/* ------------------------------------------------------------------------- */
/* ------------------------ TSDBDerivationPrinter -------------------------- */
/* ------------------------------------------------------------------------- */

void tTSDBDerivationPrinter::real_print(const tInputItem *item, int level) {
  *_out << "(\"" << escape_string(item->orth()) 
       << "\" " << item->start() << " " << item->end() << "))" << flush;
}

void tTSDBDerivationPrinter::real_print(const tLexItem *item, int level) {
  *_out << (item->trait() == PCFG_TRAIT ? "(* " : "(")
       << item->id() << " "
       << item->stem()->printname()
       << " " << item->score() << " " << item->start() <<  " " << item->end()
       << " (\"" << escape_string(item->orth()) << "\"";

  for(item_citer token = item->daughters().begin();
      token != item->daughters().end();
      ++token) {
    ostringstream buffer;
    tItsdbPrinter ip(buffer);
    ip.print(*token);
    *_out << " " << (*token)->id() 
          << " \"" << escape_string(buffer.str()) << "\"";
  } // for
  *_out << "))" << flush; 
}

void tTSDBDerivationPrinter::real_print(const tPhrasalItem *item, int level) {
  if(!level && item->result_root() > -1) 
    *_out << (item->trait() == PCFG_TRAIT ? "(* " : "(")
         << print_name(item->result_root()) << " ";
  
  *_out << (item->trait() == PCFG_TRAIT ? "(* " : "(")
       << item->id() << " " << item->printname() << " " << item->score()
       << " " << item->start() << " " << item->end();
  
  const item_list &daughters = item->daughters();
  for(item_citer pos = daughters.begin(); pos != daughters.end(); ++pos) {
    *_out << " ";
    if(_protocolversion == 1)
      (*pos)->print_gen(this, level + 1);
    else
      *_out << (*pos)->id();
  }

  *_out << (item->result_root() > -1 ? "))" : ")") << flush;
}

/* ------------------------------------------------------------------------- */
/* ---------------------- DelegateDerivationPrinter ------------------------ */
/* ------------------------------------------------------------------------- */

void
tDelegateDerivationPrinter::real_print(const tInputItem *item, int level) {
  _itemprinter.print(item, level);
}

void 
tDelegateDerivationPrinter::real_print(const tLexItem *item, int level) {
  //fprintf(*_out, "(");
  _itemprinter.print(item, level);
  print(keydaughter(item), level);
  // fprintf(*_out, ")");
}

void 
tDelegateDerivationPrinter::real_print(const tPhrasalItem *item, int level) {
  _itemprinter.print(item, level);

  for(item_citer pos = item->daughters().begin();
      pos != item->daughters().end(); ++pos) {
    print(*pos, level + 1);
  }
}

/* ------------------------------------------------------------------------- */
/* -------------------------- FS/FegramedPrinter --------------------------- */
/* ------------------------------------------------------------------------- */

void tFSPrinter::print(const tItem *arg, int level) {
  get_fs(arg).print(*_out, _dag_printer);
}

void tFegramedPrinter::print(const dag_node *dag, 
                             const char *name, int level) { 
  open_stream(name);
  FegramedDagPrinter fp;
  fp.print(_out, dag);
  close_stream();
}

void tFegramedPrinter::print(const tItem *arg, const char *name, int level) { 
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
tJxchgPrinter::print(const tItem *arg, int level) { 
  arg->print_gen(this, level); 
}


/** Print the yield of an input item, recursively or not */
void tJxchgPrinter::print_yield(const tInputItem *item) {
  if (item->daughters().empty()) {
    *_out << '"' << item->form() << '"';
  } else {
    item_citer it = item->daughters().begin();
    print(*it);
    while(++it != item->daughters().end()) {
      *_out << " ";
      print(*it++);
    }
  }
}

/** Input items don't print themselves */
void tJxchgPrinter::real_print(const tInputItem *item, int level) { }

void 
tJxchgPrinter::real_print(const tLexItem *item, int level) {
  *_out << item->id() << " " << item->start() << " " << item->end() << " "
    // print the HPSG type this item stems from
       << print_name(item->stem()->type()) << "[";
  for(const list_int *l = inflrs_todo(item); l != 0; l = rest(l)) {
    *_out << print_name(first(l));
    if (rest(l) != 0) *_out << " ";
  }
  *_out << "] ( "; 
  // This prints only the yield, because all daughters are tInputItems
  for(item_citer it = item->daughters().begin(); it != item->daughters().end()
        ; it++)
    print_yield(dynamic_cast<tInputItem *>(*it));
  *_out << " ) ";
  jxchgprinter.print(*_out, get_fs(item).dag());
  *_out << std::endl;
}

void 
tJxchgPrinter::real_print(const tPhrasalItem *item, int level) {
  *_out << item->id() << " " << item->start() << " " << item->end() << " "
    // print the rule type, if any
       << print_name(item->identity()) 
       << " (";
  for(item_citer it = item->daughters().begin(); it != item->daughters().end()
        ; it++) {
    *_out << " " << (*it)->id();
  }
  *_out << " ) ";
  jxchgprinter.print(*_out, get_fs(item).dag());
  *_out << std::endl;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------ tLUIPrinter ------------------------------ */
/* ------------------------------------------------------------------------- */

void 
tLUIPrinter::print(const tItem *item, int level) {
  ostringstream fn;
  fn << _path << item->id() << ".lui";
  ofstream stream(fn.str().c_str());
  if (! stream) throw tError("File open error for file " + fn.str());
  stream << "avm " << item->id() << " ";
  get_fs(item).print(stream, _dagprinter);
  stream << " \"Edge # " << item->id() << "\"\f\n" ;
  stream.close();
}


void 
tItsdbPrinter::print(const tItem *item, int level) {
  get_fs(item).print(_stream, _dagprinter);
}
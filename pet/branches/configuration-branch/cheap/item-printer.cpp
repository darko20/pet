#include "item-printer.h"

#include <sstream>

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

tTclChartPrinter::tTclChartPrinter(const char *filename, int chart_id)
  : _out(filename), _chart_id(chart_id), _item_id(0) {
}

void 
tTclChartPrinter::print(const tItem *arg) { arg->print_gen(this); }

void
tTclChartPrinter::real_print(const tInputItem *item) {
  print_it(item, true, false);
}

void 
tTclChartPrinter::real_print(const tLexItem *item) {
  print_it(item, true, false);
}

void 
tTclChartPrinter::real_print(const tPhrasalItem *item) {
  print_it(item, item->passive()
           , (! item->passive()) && item->left_extending());
}
  
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


void
tCompactDerivationPrinter::real_print(const tInputItem *item) {
  fprintf (_out, "(%s\"%s%s\" %.4g %d %d)"
           , _quoted ? "\\" : "", item->orth().c_str(), _quoted ? "\\" : ""
           , item->score(), item->start(), item->end());
}

void 
tCompactDerivationPrinter::real_print(const tLexItem *item) {
  fprintf(_out, "(%d %s/%s %.4g %d %d "
           , item->id(), stem(item)->printname()
           , print_name(const_cast<tLexItem *>(item)->type()), item->score()
           , item->start(), item->end());
  
  fprintf(_out, "[");
  for(const list_int *l = inflrs_todo(item); l != 0; l = rest(l))
    fprintf(_out, "%s%s", print_name(first(l)), rest(l) == 0 ? "" : " ");
  fprintf(_out, "]");
  
  for(list<tItem *>::const_iterator pos = item->daughters().begin();
      pos != item->daughters().end(); ++pos)
    print(*pos);
  
  fprintf(_out, ")");
}

void 
tCompactDerivationPrinter::real_print(const tPhrasalItem *item) {
  fprintf(_out, "(%d %s %.4g %d %d"
          , item->id(), item->printname(), item->score()
          , item->start(), item->end());
  
  if(item->packed.size())
    {
      fprintf(_out, " {");
      for(list<tItem *>::const_iterator pack = item->packed.begin();
          pack != item->packed.end(); ++pack)
        {
          fprintf(_out, "%s%d", pack == item->packed.begin() ? "" : " "
                  , (*pack)->id()); 
        }
        fprintf(_out, "}");
    }

  if(result_root(item) != -1)
    {
      fprintf(_out, " [%s]", print_name(result_root(item)));
    }
  
  if(inflrs_todo(item))
    {
      fprintf(_out, " [");
      for(const list_int *l = inflrs_todo(item); l != 0; l = rest(l))
        {
          fprintf(_out, "%s%s", print_name(first(l)), rest(l) == 0 ? "" : " ");
        }
      fprintf(_out, "]");
    }

  for(list<tItem *>::const_iterator pos = item->daughters().begin();
      pos != item->daughters().end(); ++pos)
    {
      print(*pos);
    }
  
  fprintf(_out, ")");
}

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
      pos != item->daughters().end(); ++pos)
    {
      print(*pos);
    }
}

void tFSPrinter::print(const tItem *arg) {
  StreamPrinter sp(_out);
  const_cast<tItem *>(arg)->get_fs().print(&sp);
}

void tFegramedPrinter::print(const dag_node *dag, const char *name) { 
  open_stream(name);
  dag_print_fed_safe(_out, const_cast<dag_node *>(dag));
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
  dag_print_fed_safe(_out, const_cast<tItem *>(arg)->get_fs().dag());
  close_stream();
}


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
  dag_print_jxchg(_out, get_fs(item).dag());
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
  dag_print_jxchg(_out, get_fs(item).dag());
  _out << std::endl;
}

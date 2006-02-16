/* -*- Mode: C++ -*- */
/** \file item-printer.h
 * Detached printer classes for chart items.
 */

#ifndef _TITEMPRINTER_H
#define _TITEMPRINTER_H

#include "errors.h"
#include <fstream>

#include "item.h"

/** A virtual base class to have a generic print service for chart items.
 *
 * This class is meant to separate the many different formats and aspects in
 * which users want chart items to be printed from their implementation.
 * That way, a new print format will not require changes in item.h, but can
 * be kept completely separate as a new subclass of tItemPrinter.
 * 
 * To implement this, we use the double dispatch technique: The user calls the 
 * tItemPrinter print() method, which calls the virtual tItem method
 * print_gen(), passing itself as argument Thus, the subtype of tItem is
 * determined, and print_gen() only calls the tItemPrinter virtual function 
 * real_print() to do the concrete printing with the chosen tItemPrinter.
 */
class tItemPrinter {
public:
  virtual ~tItemPrinter() {}

  /** The top level function called by the user */
  virtual void print(const tItem *arg) = 0;

  /** @name Base print functions
   * Concrete print functions for every subclass of tItem to implement the
   * double dispatch.
   */
  /*@{*/
  /** Base printer function for a tInputItem */
  virtual void real_print(const tInputItem *item) {};
  /** Base printer function for a tLexItem */
  virtual void real_print(const tLexItem *item) {};
  /** Base printer function for a tPhrasalItem */
  virtual void real_print(const tPhrasalItem *item) {};
  /*@}*/

protected:
  /** @name Private Accessors
   * it is possible to define functions to private functionality of the items
   * here to avoid multiple friend class entries in the items. The private
   * fields or methods can then be accessed via the superclass
   */
  /*@{*/
  const fs &get_fs(const tItem *item) { return item->_fs; }
  const list_int *inflrs_todo(const tItem *item) { return item->_inflrs_todo; }
  const type_t result_root(const tItem *item) { return item->_result_root; }

  const class lex_stem *stem(const tLexItem *item) { return item->_stem; }
  const tInputItem *
  keydaughter(const tLexItem *item) { return item->_keydaughter; }
  /*@}*/

};

/** Print chart items for the tcl/tk chart display implementation.
 *  For function descriptions, \see tItemPrinter.
 */
class tTclChartPrinter : public tItemPrinter {
public:
  /** Print onto file \a filename, the \a chart_id allows several charts to be
   *  distinguished by the chart display.
   */
  tTclChartPrinter(const char *filename, int chart_id = 0);
  
  virtual ~tTclChartPrinter() {}

  virtual void print(const tItem *arg) ;

  virtual void real_print(const tInputItem *item) ;
  virtual void real_print(const tLexItem *item) ;
  virtual void real_print(const tPhrasalItem *item) ;
  
private:
  typedef hash_map<long int, unsigned int> item_map;

  void print_it(const tItem *item, bool passive, bool left_ext);

  item_map _items;
  ofstream _out;
  int _chart_id, _item_id;
};

/** virtual printer class to provide generic derivation printing with
 *  (string) indentation.
 *  For function descriptions, \see tItemPrinter.
 */
class tDerivationPrinter : public tItemPrinter {
public:
  tDerivationPrinter(FILE *f, int indent = 2) 
    : _out(f), _indentation(-indent), _indent_delta(indent) {}

  tDerivationPrinter(const char *filename, int indent = 2)
    : _indentation(-indent), _indent_delta(indent) {
    if ((_out = fopen(filename, "w")) == NULL) {
      throw(tError((string) "could not open file" + filename));
    }
  }
  
  virtual ~tDerivationPrinter() {}

  virtual void print(const tItem *arg) {
    next_level();
    newline();
    arg->print_gen(this);
    prev_level();
  };

  virtual void real_print(const tInputItem *item) = 0;
  virtual void real_print(const tLexItem *item) = 0;
  virtual void real_print(const tPhrasalItem *item) = 0;
  
protected:
  /** Print a newline, followed by an appropriate indentation */
  virtual void newline() { fprintf(_out, "\n%*s", _indentation, ""); };
  /** Increase the indentation level */
  inline void next_level(){ _indentation += _indent_delta; }
  /** Decrease the indentation level */
  inline void prev_level(){ _indentation -= _indent_delta; }

  FILE *_out;
  int _indentation;
  int _indent_delta;
};

/** Printer to replace the old tItem::print_derivation() method.
 *  For function descriptions, \see tItemPrinter.
 */
class tCompactDerivationPrinter : public tDerivationPrinter {
public:
  tCompactDerivationPrinter(FILE *f, bool quoted = false, int indent = 2) 
    : tDerivationPrinter(f, indent), _quoted(quoted) {}

  tCompactDerivationPrinter(const char *filename, bool quoted = false
                            , int indent = 2)
    : tDerivationPrinter(filename, indent), _quoted(quoted) {}
  
  virtual ~tCompactDerivationPrinter() {}

  virtual void real_print(const tInputItem *item);
  virtual void real_print(const tLexItem *item);
  virtual void real_print(const tPhrasalItem *item);
  
private:
  bool _quoted;
};

/** Print an item with its derivation, but delegate the real printing to
 *  another tItemPrinter object.
 *  For function descriptions, \see tItemPrinter.
 */
class tDelegateDerivationPrinter : public tDerivationPrinter {
public:
  tDelegateDerivationPrinter(FILE *f, tItemPrinter &itemprinter) 
    : tDerivationPrinter(f), _itemprinter(itemprinter) {}

  tDelegateDerivationPrinter(const char *filename, tItemPrinter &itemprinter)
    : tDerivationPrinter(filename), _itemprinter(itemprinter) {}
  
  virtual ~tDelegateDerivationPrinter() {}

  virtual void real_print(const tInputItem *item);
  virtual void real_print(const tLexItem *item);
  virtual void real_print(const tPhrasalItem *item);

private:
  virtual void newline() {
    fprintf(_out, "\n%*s", _indentation, "-----------------");
  }
  tItemPrinter &_itemprinter;
};

/** Just print the feature structure of an item readably to a stream or file. 
 *  For function descriptions, \see tItemPrinter.
 */
class tFSPrinter : public tItemPrinter {
public:
  tFSPrinter(FILE *f): _out(f) {}

  tFSPrinter(const char *filename) {
    if ((_out = fopen(filename, "w")) == NULL) {
      throw(tError((string) "could not open file" + filename));
    }
  }
  
  virtual ~tFSPrinter() {}

  /** We don't need the second dispatch here because everything is available
   * using functions of the superclass and we don't need to differentiate.
   */
  virtual void print(const tItem *arg);

private:
  FILE *_out;
};

/** Just print the feature structure of an item in fegramed format to a stream
 *  or file.
 *  For function descriptions, \see tItemPrinter.
 */
class tFegramedPrinter : public tItemPrinter {
public:
  tFegramedPrinter(): _out(NULL), _filename_prefix(NULL) {}

  /** Specify a \a prefix that is prepended to all filenames, e.g., a directory
   *  prefix.
   */
  tFegramedPrinter(const char *prefix) {
    _out = NULL;
    _filename_prefix = strdup(prefix);
  }
  
  virtual ~tFegramedPrinter() {
    if (_out != NULL) { 
      fclose(_out);
      _out = NULL;
    }
    if (_filename_prefix != NULL) {
      free(_filename_prefix);
    }
  }

  /** We don't need the second dispatch here because everything is available
   * using functions of the superclass and we don't need to differentiate.
   */
  virtual void print(const tItem *arg) { print(arg, NULL); }

  /** We don't need the second dispatch here because everything is available
   * using functions of the superclass and we don't need to differentiate.
   */
  void print(const tItem *arg, const char *name);

  /** This is for convenience, this printer does not do anything beyond
   *  printing the dag, so we can safely use it for dags alone.
   */
  void print(const dag_node *dag, const char *name = "fstruc");

private:
  /** This function is only useful when more than one item shall be printed
   *  with the same prefix, i.e., the constructor has been called with \c
   *  make_unique == \c true.
   *  \param name The name to append to the given prefix.
   */
  void open_stream(const char *name = "") {
    if (_filename_prefix != NULL) {
      if (_out != NULL) { 
        fclose(_out);
        _out = NULL;
      }
      char *unique = new char[strlen(_filename_prefix) + strlen(name) + 7];
      strcpy(unique, _filename_prefix);
      strcpy(unique + strlen(_filename_prefix), name);
      strcpy(unique + strlen(_filename_prefix) + strlen(name), "XXXXXX");
      int fildes = mkstemp(unique);
      if ((fildes == -1) || ((_out = fdopen(fildes, "w")) == NULL)) {
        throw(tError((string) "could not open file" + unique));
      }
      delete[] unique;
    }
  }

  /** This function is only useful when more than one item shall be printed
   *  with the same prefix, i.e., the constructor has been called with \c
   *  make_unique == \c true.
   */
  void close_stream() {
    if ((_filename_prefix != NULL) && (_out != NULL)) { 
      fclose(_out);
      _out = NULL;
    }
  }

  FILE *_out;
  char *_filename_prefix;
};

/** Print chart items for the exchange with Ulrich Krieger's jfs, mainly for
 *  use with his corpus directed approximation.
 *  For function descriptions, \see tItemPrinter.
 */
class tJxchgPrinter : public tItemPrinter {
public:
  /** Print items onto stream \a out. */
  tJxchgPrinter(ostream &out) : _out(out) {}
  
  virtual ~tJxchgPrinter() {}

  virtual void print(const tItem *arg) ;

  virtual void real_print(const tInputItem *item) ;
  virtual void real_print(const tLexItem *item) ;
  virtual void real_print(const tPhrasalItem *item) ;
  
private:
  void print_yield(const tInputItem *item);

  ostream &_out;
};

#endif

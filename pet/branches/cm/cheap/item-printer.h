/* -*- Mode: C++ -*- */
/** \file item-printer.h
 * Detached printer classes for chart items.
 */

#ifndef _TITEMPRINTER_H
#define _TITEMPRINTER_H

#include "item.h"
#include "errors.h"
#include "dagprinter.h"
#include "hashing.h"
#include <iostream>
#include <fstream>
#include <iomanip> 

/** A virtual base class to have a generic print service for chart items.
 *
 * This class is meant to separate the many different formats and aspects in
 * which users want chart items to be printed from their implementation.
 * That way, a new print format will not require changes in item.h, but can
 * be kept completely separate as a new subclass of tAbstractItemPrinter.
 * 
 * To implement this, we use the double dispatch technique: The user calls the
 * tAbstractItemPrinter print() method, which calls the virtual tItem method
 * print_gen(), passing itself as argument Thus, the subtype of tItem is
 * determined, and print_gen() only calls the tAbstractItemPrinter virtual
 * function real_print() to do the concrete printing with the chosen
 * tAbstractItemPrinter.
 */
class tAbstractItemPrinter {
public:
  tAbstractItemPrinter() : _out(NULL) {}
  tAbstractItemPrinter(std::ostream &out) : _out(&out) {}
  virtual ~tAbstractItemPrinter() {}

  std::pair<tAbstractItemPrinter *, const tItem *>
  set(const tItem &item) {
    return std::pair<tAbstractItemPrinter *, const tItem *>(this, &item);
  }

  /** The top level function called by the user */
  virtual void print(const tItem *arg, int level = 0) = 0;

  /** The top level function called by the user */
  virtual void print(std::ostream &out, const tItem *arg, int level = 0) {
    _out = &out; print(arg);
  }

  /** @name Base print functions
   * Concrete print functions for every subclass of tItem to implement the
   * double dispatch.
   */
  /*@{*/
  /** Base printer function for a tInputItem */
  virtual void real_print(const tInputItem *item, int level = 0) {}
  /** Base printer function for a tLexItem */
  virtual void real_print(const tLexItem *item, int level = 0) {}
  /** Base printer function for a tPhrasalItem */
  virtual void real_print(const tPhrasalItem *item, int level = 0) {}
  /*@}*/

protected:
  /** @name Private Accessors
   * it is possible to define functions to private functionality of the items
   * here to avoid multiple friend class entries in the items. The private
   * fields or methods can then be accessed via the superclass
   */
  /*@{*/
  int get_id(const tItem *item) { return item->_id; }
  const fs &get_fs(const tItem *item) {
    // \todo cast is not nice, but the best we can do
    return const_cast<tItem *>(item)->_fs;
  }
  const list_int *inflrs_todo(const tItem *item) { return item->_inflrs_todo; }
  const type_t result_root(const tItem *item) { return item->_result_root; }

  const class lex_stem *stem(const tLexItem *item) { return item->_stem; }
  const tInputItem * keydaughter(const tLexItem *item) {
    return item->_keydaughter;
  }
  const tPaths & get_paths(const tItem *item) { return item->_paths; }
  /*@}*/

  std::ostream *_out;
};

/** The default printer that mimics the old functionality of item.print()
 */
class tItemPrinter : public tAbstractItemPrinter {
public:
  tItemPrinter(std::ostream &out, bool print_derivation = false,
               bool print_fs = false);

  tItemPrinter(bool print_derivation = false, bool print_fs = false);

  virtual ~tItemPrinter();
  
  /** The top level function called by the user */
  virtual void print(const tItem *arg, int level = 0) { 
    arg->print_gen(this, level); 
  }

  /** @name Base print functions
   * Concrete print functions for every subclass of tItem to implement the
   * double dispatch.
   */
  /*@{*/
  /** Base printer function for a tInputItem */
  virtual void real_print(const tInputItem *item, int level = 0) ;
  /** Base printer function for a tLexItem */
  virtual void real_print(const tLexItem *item, int level = 0) ;
  /** Base printer function for a tPhrasalItem */
  virtual void real_print(const tPhrasalItem *item, int level = 0) ;
  /*@}*/
  
private:
  // Print the positions of argument positions which are still to fill (active
  // items only)
  void print_tofill(std::ostream &out, const tItem *item);

  // Print the list of inflection rules which remain to be processed
  void print_inflrs(std::ostream &out, const tItem *item); 

  // Print the lattice paths this item may belong to
  void print_paths(std::ostream &out, const tItem *item);

  // Print the ids of the items packed into this node
  void print_packed(std::ostream &out, const tItem *item);

  // Print parent and daughter ids of this item
  void print_family(std::ostream &out, const tItem *item);

  // Printing common to all subtypes of tItem
  void print_common(std::ostream &out, const tItem *item);

  // Printing the embedded fs, if required
  void print_fs(std::ostream &out, const fs &f);

  class tAbstractItemPrinter* _derivation_printer;
  class AbstractDagPrinter* _dag_printer;
};


/** Print chart items for the tcl/tk chart display implementation.
 *  For function descriptions, \see tAbstractItemPrinter.
 */
class tTclChartPrinter : public tAbstractItemPrinter {
public:
  /** Print onto stream \a out, the \a chart_id allows several charts to be
   *  distinguished by the chart display.
   */
  tTclChartPrinter(std::ostream &out, int chart_id = 0)
    : tAbstractItemPrinter(out), _chart_id(chart_id) {}
  
  virtual ~tTclChartPrinter() {}

  virtual void print(const tItem *arg, int level = 0) { 
    arg->print_gen(this, level); 
  }

  virtual void
  real_print(const tInputItem *item, int level = 0) { 
    print_it(item, true, false); 
  }
  virtual void 
  real_print(const tLexItem *item, int level = 0) { 
    print_it(item, true, false);
  }
  virtual void
  real_print(const tPhrasalItem *item, int level = 0) {
    print_it(item, item->passive()
             , (! item->passive()) && item->left_extending());
  }
  
private:
  typedef HASH_SPACE::hash_map<long int, unsigned int> item_map;

  void print_it(const tItem *item, bool passive, bool left_ext);

  item_map _items;
  int _chart_id, _item_id;
};

/** virtual printer class to provide generic derivation printing with
 *  (string) indentation.
 *  For function descriptions, \see tAbstractItemPrinter.
 */
class tAbstractDerivationPrinter : public tAbstractItemPrinter {
public:
  tAbstractDerivationPrinter(std::ostream &out, int indent = 2) 
    : tAbstractItemPrinter(out), _indentation(-indent), _indent_delta(indent) {}

  tAbstractDerivationPrinter(int indent = 2) 
    : tAbstractItemPrinter(), _indentation(-indent), _indent_delta(indent) {}

  virtual ~tAbstractDerivationPrinter() {}

  virtual void print(const tItem *arg, int level = 0) {
    next_level();
    newline();
    arg->print_gen(this, level);
    prev_level();
  };

  virtual void real_print(const tInputItem *item, int level = 0) = 0;
  virtual void real_print(const tLexItem *item, int level = 0) = 0;
  virtual void real_print(const tPhrasalItem *item, int level = 0) = 0;
  
protected:
  /** Print a newline, followed by an appropriate indentation */
  virtual void newline() { 
    *_out << std::endl ; *_out << std::setw(_indentation) << "";
  };
  /** Increase the indentation level */
  inline void next_level(){ _indentation += _indent_delta; }
  /** Decrease the indentation level */
  inline void prev_level(){ _indentation -= _indent_delta; }

  int _indentation;
  int _indent_delta;
};

/** Printer to replace the old tItem::print_derivation() method.
 *  For function descriptions, \see tAbstractItemPrinter.
 */
class tCompactDerivationPrinter : public tAbstractDerivationPrinter {
public:
  tCompactDerivationPrinter(bool quoted = false, int indent = 2) 
    : tAbstractDerivationPrinter(indent), _quoted(quoted) {}

  tCompactDerivationPrinter(std::ostream &out, bool quoted = false, int indent = 2) 
    : tAbstractDerivationPrinter(out, indent), _quoted(quoted) {}

  virtual ~tCompactDerivationPrinter() {}

  virtual void real_print(const tInputItem *item, int level = 0);
  virtual void real_print(const tLexItem *item, int level = 0);
  virtual void real_print(const tPhrasalItem *item, int level = 0);
  
private:
  void print_inflrs(const tItem *);
  void print_daughters(const tItem *, int level = 0);

  bool _quoted;
};

/** Print the derivation of an item in [incr tsdb()] compatible form,
 *  according to \a protocolversion.
 *  For function descriptions, \see tAbstractItemPrinter.
 */
class tTSDBDerivationPrinter : public tAbstractItemPrinter {
public:
  tTSDBDerivationPrinter(std::ostream &out, int protocolversion)
    : tAbstractItemPrinter(out), _protocolversion(protocolversion) {}

  virtual ~tTSDBDerivationPrinter() {}

  virtual void print(const tItem *item, int level = 0) { 
    item->print_gen(this, level);
  }

  virtual void real_print(const tInputItem *item, int level = 0);
  virtual void real_print(const tLexItem *item, int level = 0);
  virtual void real_print(const tPhrasalItem *item, int level = 0);

private:
  bool _protocolversion;
};


/** Print an item with its derivation, but delegate the real printing to
 *  another tAbstractItemPrinter object.
 *  For function descriptions, \see tAbstractItemPrinter.
 */
class tDelegateDerivationPrinter : public tAbstractDerivationPrinter {
public:
  tDelegateDerivationPrinter(std::ostream &out,
                             tAbstractItemPrinter &itemprinter,
                             int indent = 0) 
    : tAbstractDerivationPrinter(out, indent), _itemprinter(itemprinter) {}

  virtual ~tDelegateDerivationPrinter() {}

  virtual void real_print(const tInputItem *item, int level = 0);
  virtual void real_print(const tLexItem *item, int level = 0);
  virtual void real_print(const tPhrasalItem *item, int level = 0);

private:
  virtual void newline() {
    if (_indent_delta > 0) *_out << std::setw(_indentation) << "-";
  }

  bool _do_indentation;
  tAbstractItemPrinter &_itemprinter;
};

/** Just print the feature structure of an item readably to a stream or file. 
 *  For function descriptions, \see tAbstractItemPrinter.
 */
class tFSPrinter : public tAbstractItemPrinter {
public:
  tFSPrinter(std::ostream &out, AbstractDagPrinter &printer): 
    tAbstractItemPrinter(out), _dag_printer(printer) {}

  virtual ~tFSPrinter() {}

  /** We don't need the second dispatch here because everything is available
   * using functions of the superclass and we don't need to differentiate.
   */
  virtual void print(const tItem *arg, int level = 0);

private:
  AbstractDagPrinter &_dag_printer;
};

#ifdef USE_DEPRECATED
/** Just print the feature structure of an item in fegramed format to a stream
 *  or file.
 *  For function descriptions, \see tAbstractItemPrinter.
 */
class tFegramedPrinter : public tAbstractItemPrinter {
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
  virtual void print(const tItem *arg, int level = 0) { 
    print(arg, NULL, level); 
  }

  /** We don't need the second dispatch here because everything is available
   * using functions of the superclass and we don't need to differentiate.
   */
  void print(const tItem *arg, const char *name, int level = 0);

  /** This is for convenience, this printer does not do anything beyond
   *  printing the dag, so we can safely use it for dags alone.
   */
  void print(const dag_node *dag, const char *name = "fstruc", level = 0);

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
        throw(tError((std::string) "could not open file" + unique));
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
#endif

/** Just print the feature structure of an item in fegramed format to a stream
 *  or file.
 *  For function descriptions, \see tAbstractItemPrinter.
 */
class tFegramedPrinter : public tAbstractItemPrinter {
public:
  tFegramedPrinter(): _out(NULL), _filename_prefix(NULL) {}

  /** Specify a \a prefix that is prepended to all filenames, e.g., a directory
   *  prefix.
   */
  tFegramedPrinter(const char *prefix) 
    : _out(), _filename_prefix(strdup(prefix)) { }
  
  virtual ~tFegramedPrinter() {
    close_stream();
    if (_filename_prefix != NULL) {
      free(_filename_prefix);
    }
  }

  /** We don't need the second dispatch here because everything is available
   * using functions of the superclass and we don't need to differentiate.
   */
  virtual void print(const tItem *arg, int level = 0) { 
    print(arg, NULL, level); 
  }

  /** We don't need the second dispatch here because everything is available
   * using functions of the superclass and we don't need to differentiate.
   */
  void print(const tItem *arg, const char *name, int level = 0);

  /** This is for convenience, this printer does not do anything beyond
   *  printing the dag, so we can safely use it for dags alone.
   */
  void print(const dag_node *dag, const char *name = "fstruc", int level = 0);

private:
  /** This function is only useful when more than one item shall be printed
   *  with the same prefix, i.e., the constructor has been called with \c
   *  make_unique == \c true.
   *  \param name The name to append to the given prefix.
   */
  void open_stream(const char *name = "") {
    close_stream();
    if (_filename_prefix != NULL) {
      char *unique = new char[strlen(_filename_prefix) + strlen(name) + 7];
      strcpy(unique, _filename_prefix);
      strcpy(unique + strlen(_filename_prefix), name);
      strcpy(unique + strlen(_filename_prefix) + strlen(name), "XXXXXX");
      int fildes = mkstemp(unique);
      if ((fildes == -1) || (_out.open(unique) , ! _out.good())) {
        throw(tError((std::string) "could not open file" + unique));
      }
      delete[] unique;
    }
  }

  /** This function is only useful when more than one item shall be printed
   *  with the same prefix, i.e., the constructor has been called with \c
   *  make_unique == \c true.
   */
  void close_stream() {
    if (_out.is_open()) _out.close();
  }

  std::ofstream _out;
  FegramedDagPrinter fp;
  char *_filename_prefix;
};


/** Print chart items for the exchange with Ulrich Krieger's jfs, mainly for
 *  use with his corpus directed approximation.
 *  For function descriptions, \see tAbstractItemPrinter.
 */
class tJxchgPrinter : public tAbstractItemPrinter {
public:
  /** Print items onto stream \a out. */
  tJxchgPrinter(std::ostream &out)
  : tAbstractItemPrinter(out), jxchgprinter() {}
  
  virtual ~tJxchgPrinter() {}

  virtual void print(const tItem *arg, int level = 0) ;

  virtual void real_print(const tInputItem *item, int level = 0) ;
  virtual void real_print(const tLexItem *item, int level = 0) ;
  virtual void real_print(const tPhrasalItem *item, int level = 0) ;
  
private:
  void print_yield(const tInputItem *item);

  JxchgDagPrinter jxchgprinter;
};


/** Print chart items for the exchange with Ulrich Krieger's jfs, mainly for
 *  use with his corpus directed approximation.
 *  For function descriptions, \see tAbstractItemPrinter.
 */
class tLUIPrinter : public tAbstractItemPrinter {
public:
  /** Print items into files that are under \a path. */
  tLUIPrinter(const char *path) : _dagprinter(), _path(path) {}
  
  virtual ~tLUIPrinter() {}

  virtual void print(const tItem *arg, int level = 0) ;

private:
  LUIDagPrinter _dagprinter;
  std::string _path;
};

/** 
 *  For function descriptions, \see tAbstractItemPrinter.
 */
class tItsdbPrinter : public tAbstractItemPrinter {
public:
  tItsdbPrinter(std::ostream &stream) : _dagprinter(), _stream(stream) {}
  virtual ~tItsdbPrinter() {}
  virtual void print(const tItem *arg, int level = 0) ;

private:
  ItsdbDagPrinter _dagprinter;
  std::ostream &_stream;
};

/** use an item_printer like a modifier */
inline std::ostream &
operator<<(std::ostream &out,
           const std::pair<tAbstractItemPrinter *, const tItem *> &p) {
  p.first->print(out, p.second);
  return out;
}

/** default printing for chart items: use a tItemPrinter */
inline std::ostream &operator<<(std::ostream &out, const tItem &item) {
  return out << tItemPrinter().set(item);
}

#endif

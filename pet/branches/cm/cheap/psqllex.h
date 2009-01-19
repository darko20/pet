/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//
// This modules implements the link to an external (Postgres) lexical database.
//

#ifndef _PSQLLEX_H_
#define _PSQLLEX_H_

#include <libpq-fe.h>
#include <string>
#include "hashing.h"
#include <cctype>
#include <iosfwd>
#include <list>

using std::string;
using std::list;

// forward declarations for the header
struct dag_node;
struct list_int;

/** Implements the tLexicon interface accessing a Postgres database. */
// :public tLexicon
class tPSQLLex  {
private:
  enum field_t {
         COL_MIXED = 0,
         COL_SYMBOL,
         COL_STRING,
         COL_STRING_RAW_LIST,
         COL_STRING_LIST,
         COL_STRING_DIFFLIST,
         COL_MIXED_PATH_LIST,
         COL_MIXED_PATH_DIFFLIST,
         COL_MIXED_PATH_LIST_STRANGE,
         COL_MIXED_PATH_DIFFLIST_STRANGE
  } ;
  
  class field_spec {
    struct charptr_equal {
      inline bool operator()(const char *key1, const char *key2) const {
        return strcmp(key1, key2) == 0;
      }
    };

    typedef hash_map< const char *, int, hash<const char *> , charptr_equal >
      ttmap;

  private:
    static const char * const _typestrings[];
    static ttmap _type_table ;

  public:
    string name;
    list_int *path;
    field_t field_type;

    field_spec(const char *fieldname, list_int *fspath, const char *fieldtype);

    field_spec() {}

    static const char *field_name(field_t field) {
      return _typestrings[field];
    }

    std::ostream & print(std::ostream &out) const {
      out << "[" << this->name << "|" << this->path << "|" 
          << _typestrings[this->field_type] << "]";
      return out;
    }
  };

  /** The connection to the database (NULL if inactive) */
  PGconn *_conn;

  typedef vector<field_spec> field_list;
  /** The specification of the grammar fields */
  field_list grammar_field;

  /** The list of fields used in the command string to retrieve lexicon entries
   *  from the database.
   */
  string _fieldlist;

  /** The current mode (database subset) used */
  string _mode;

  /** Convert a lisp path into a list of feature ids
   *  If the lisp path contains an unknown feature, <code>(list_int *)-1</code>
   *  will be returned instead.
   */
  list_int *lisp2petpath(char *pathstring);

  /** Issue an error for a specific row from the database */
  void entry_error(const string msg, PGresult *res, int row_no);

  /** Read the table of grammar field specifications and create the _fieldlist
   *  string.
   *
   * There are three fixed fields: name, type and orthography. They always have
   * to be present in the database
   * 
   * \param mode the mode whose fields are to be read.
   */
  void read_slot_specs(const char *mode);

  /** Massage a string from the database into a list of strings.
   * \param ws_separated_strings The character string coming from the database,
   *        a whitespace separated list of strings
   * \return a list of strings
   * \todo remove eventual SQL escape sequences
   */
  list<string> split_ws(const char *ws_separated_strings);

  /** Convert a column value from the database according to the slot type 
   *  given for this column in the dfn file
   */
  dag_node *dagify_slot_value(const string slot_value, field_t slot_type);

  /** Create a modlist based on the grammar fields in the database
   *  \param res the current query result from the database for a stem form
   *  \param row_no the row number to be treated now
   *  \param startcol the column number of the first non-fixed
   *                 (i.e. grammar-specified) retrieved column
   *  \return a modlist containing the paths and the value types to be added
   *          to the lexicon entry
   */
  dag_node *build_instance_fs(PGresult *res, int row_no, int startcol);

  /** Build a lex_stem from a row of a lexdb query for some stem.
   * \param res the current query result from the database for a stem form
   * \param row_no the row number to be treated now
   */
  class lex_stem *build_lex_stem(PGresult *res, int row_no);


  list< class lex_stem * > retrieve_lex_stems(const string &plain_stem);

  void close_connection(){
    if(_conn)
      PQfinish(_conn);
    _conn = 0;
  }

public:

  /** Obtain list of stems for given orthography. */
  virtual list< class lex_stem *> operator()(const string &stem) {
    return retrieve_lex_stems(stem);
  }

  bool connected() { return _conn != 0; }

  /** Construct from a connection info string, and the names of the
      descriptor table, and the lexicon table. */
  tPSQLLex(const string &DBName, const string &mode);
  
  /** Destructor. Shuts down (possibly) open connection. */
  virtual ~tPSQLLex() { close_connection(); }
  
  /** Print a readable description of this object's state. */
  std::ostream& print(std::ostream& out);

};


#endif

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
// This module implements the link to an external lexical database.
//

/* 
 * Open issues:
 * - role of instance name?? in build_lex_stem!! continue there!!
 * - Cache functionality needed!
 * - maybe test this with full lexicon preloading (tables only vs. with fs)
 * 
 * + Massage slot values according to slot type (sym, str, mixed, ...)
 *   addressed in dagify_slot_value etc.
 */

#include "psqllex.h"
#include "errors.h"
#include <iostream>
#include "list-int.h"
#include "dag.h"
#include "lexicon.h"

void warn(const string &message) {
  fprintf(stderr, "Warning: %s\n", message.c_str());
}

#define lookup_string lookup_symbol

#define FIELDS_FIELDS "slot, field, path, type"
#define SLOT_COLUMN 0
#define NAME_COLUMN 1
#define PATH_COLUMN 2
#define TYPE_COLUMN 3

using namespace std;

#if 0 
#include<map>

type_t currtype = 0;
map<string, type_t> typecodes;
vector<string> typenames;

type_t lookup_type(string type_name) {
  map<string, type_t>::iterator it = typecodes.find(type_name);
  if (it == typecodes.end()) {
    typecodes[type_name] = currtype;
    typenames.push_back(type_name);
    return currtype++;
  } else {
    return it->second;
  }
}

string lookup_typename(type_t tcode) {
  return typenames[tcode];
}

ostream &lex_stem::print(ostream &out) {
  out << "[" << type_name(_instance_type) << ","
      << type_name(_lex_type) << "|";
  for(list<string>::iterator it = _orth.begin(); it != _orth.end(); it++){
    out << *it << " ";
  }
  out << endl;
  for(modlist::iterator it = _mods.begin(); it != _mods.end(); it++) {
    out << it->first << ":" << type_name(it->second) << endl;
  }
  out << "]" << endl;
}
#endif

const char * const tPSQLLex::field_spec::_typestrings[] = {
  "mixed",
  "sym",
  "str",
  "str-rawlst", // only for orth|orthography
  "str-lst",
  "str-dlst",
  "lst",        // unsupported
  "dlst",       // unsupported
  "lst-t",      // unsupported
  "dlst-t",     // unsupported
  // the rest is deprecated 
  "symbol",             // sym
  "string",             // str
  "string-list",        // str-rawlst  
  "string-fs",          // str-lst
  "string-diff-fs",     // str-dlst
  "mixed-fs",           // lst
  "mixed-diff-fs",      // dlst
  ""                    // end-of-list marker
};

tPSQLLex::field_spec::ttmap tPSQLLex::field_spec::_type_table;

list_int *tPSQLLex::lisp2petpath(char *pathstring) {
  string featurename;
  list_int *result = cons(-1, NULL);
  list_int *last = result;
  // seek past the opening paren
  while (*pathstring == ' ' || *pathstring == '(') pathstring++;
  while (*pathstring != '\0' && *pathstring != ')') {
    // Skip blanks at the beginning of a feature name
    if (*pathstring != '\0' && isspace(*pathstring)) pathstring++;
    // Collect feature name (converted to upper case)
    while (*pathstring != '\0' && *pathstring != ')' && !isspace(*pathstring)){
      featurename += toupper(*pathstring);
    }
    // look up feature id an check that it exists
    attr_t feature_id = lookup_attr(featurename.c_str());
    if (-1 == feature_id) {
      warn((string) "Unknown feature " + featurename 
           + " in lexdb path specification");
      free_list(result);
      return (list_int *) -1;
    }
    last->next = cons(feature_id, NULL);
    last = last->next;
    pathstring++;
  }
  // remove the first cons, which is only a placeholder to avoid extra checks
  last = result->next;
  delete result;
  return last;
}

tPSQLLex::field_spec::field_spec(const char *fieldname, list_int *fspath
                                 , const char *fieldtype) {
  if (_type_table.empty()) {
    for (int i = 0; *(_typestrings[i]) != '\0'; i++) {
      _type_table[_typestrings[i]] = i;
    }
  }

  name = fieldname;
  path = fspath;
  
  // Get the field type from the database
  ttmap::const_iterator it = _type_table.find(fieldtype);
  if (it == _type_table.end()) {
    throw tError((string) "Unknown field type " + fieldtype
                + " in lexical database ");
  } else {
    field_type = (field_t) it->second;
    if (field_type > COL_MIXED_PATH_DIFFLIST_STRANGE) {
      field_type = (field_t)(field_type - COL_MIXED_PATH_DIFFLIST_STRANGE);
      warn((string)"Field type " + fieldtype +
           " is deprecated, use " + _typestrings[field_type] + " instead!\n");
    }
    if (field_type > COL_STRING_DIFFLIST) {
      throw tError((string) "Unsupported field type " + fieldtype
                + " in lexical database ");
    }
  }
}

ostream& tPSQLLex::print(ostream& out) {
  for(field_list::iterator fspecit = grammar_field.begin();
      fspecit != grammar_field.end(); fspecit++) {
    fspecit->print(out);
    out << endl;
  }
  out << "<" << _fieldlist << ">" << endl;
  return out;
}

/** Create a new PostgreSQL lexical database.
 * @param connection_info a string containing database name, host, user name
 *               and password
 * @param mode the current mode (database subset) to use
 */
tPSQLLex::tPSQLLex(const string &connection_info, const string &mode)
  : _conn(0), _mode(mode) {
  // Connect to database.
  _conn = PQconnectdb(connection_info.c_str());
  if (PQstatus(_conn) == CONNECTION_BAD)
    {
      fprintf(stderr, "Connection to lexical database [%s] failed.\n",
              connection_info.c_str());
      fprintf(stderr, "%s", PQerrorMessage(_conn));
      close_connection();
      return ;
    }
    
  // Obtain descriptor table.
  read_slot_specs(_mode.c_str());
}

void tPSQLLex::entry_error(const string msg, PGresult *res, int row_no) {
  string errstr = msg;
  int nFields = PQnfields(res);
  for (int j = 0; j < nFields; j++)
    errstr += (string)" |" + PQgetvalue(res, row_no, j) + "|";
  throw tError(errstr);
}


void tPSQLLex::read_slot_specs(const char *mode) {
  char *realmode = new char[2*(strlen(mode) + 1)];
  PQescapeString (realmode, mode, strlen(mode));
    
  string command = 
    (string) "SELECT slot,field,path,type FROM dfn WHERE mode='" 
    + realmode + "' OR mode IS NULL";
  PGresult *res = PQexec(_conn, command.c_str());
  delete[] realmode;

  if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
      fprintf(stderr, "SELECT dfn fields command failed\n");
      PQclear(res);
      return;
    }
  
  _fieldlist = "";
  
  // These fields are fixed. `type' has to be a unifs field with empty path,
  // `orthography' will also be passed directly to the constructor of lex_stem.
  // Because of this, they are put into fixed positions
  grammar_field.resize(3);

  // These are the names of the fixed fields
  string instance_name_field, orthography_field, type_name_field;

  /* build the field list to be retrieved for lexicon entries and 
   * the field specs
   */
  for (int row = 0; row < PQntuples(res); row++) {
    char *slotname = PQgetvalue(res, row, NAME_COLUMN);
    // Is it the `name' field, i.e., the instance name of the lexicon entry?
    if (strcmp(PQgetvalue(res, row, SLOT_COLUMN), "id") == 0) {
      grammar_field[0] =
        field_spec(slotname, NULL, PQgetvalue(res, row, TYPE_COLUMN));
      instance_name_field = slotname;
    } else {
      // Is it the `orthography' field? if so, save its name for later
      if (strcmp(PQgetvalue(res, row, SLOT_COLUMN), "orth") == 0) {
        orthography_field = slotname;
      } else {
        if (strcmp(slotname, orthography_field.c_str()) == 0) {
          // Make the orthography field the last special field, so it will
          // end up at position two
          list_int *path = lisp2petpath(PQgetvalue(res, row, PATH_COLUMN));
          if (path == (list_int *) -1) {
            entry_error((string)"Illegal Path in Orthography specification",
                        res, row);
          }
          grammar_field[2] = field_spec(slotname, path,
                                        PQgetvalue(res, row, TYPE_COLUMN));
        } else {
          if (strcmp(PQgetvalue(res, row, PATH_COLUMN), "nil") == 0) {
            // This is the field for the lexical type of the entry
            grammar_field[1] 
              = field_spec(slotname, NULL, PQgetvalue(res, row, TYPE_COLUMN));
            type_name_field = slotname;
          } else {
            // this is an ordinary `unifs' field
            list_int *path = lisp2petpath(PQgetvalue(res, row, PATH_COLUMN));
            if (path == (list_int *) -1) {
              warn((string)"Ignoring dfn column " + slotname);
            } else {
              field_spec newspec(slotname, path,
                                 PQgetvalue(res, row, TYPE_COLUMN));
              _fieldlist = _fieldlist + ", " + string(slotname);
              grammar_field.push_back(newspec);
            }
          }
        }
      }
    }
  }

  _fieldlist = instance_name_field + ", "
    + type_name_field + ", "
    + orthography_field + _fieldlist;

  PQclear(res);
}

/** Massage a string from the database into a list of strings.
 * \param ws_separated_strings The character string coming from the database, a
 *        whitespace separated list of strings
 * \returns a list of words as strings
 * \todo remove eventual SQL escape sequences
 */
list<string> tPSQLLex::split_ws(const char *ws_separated_strings) {
  // massage ws_separated_strings into a list of strings
  const string WS(" \t");
  string wholestring = ws_separated_strings;
  string::size_type start, end;
  start = end = 0;
  list<string> result;
  while (start != string::npos) {
    // skip whitespace
    start = wholestring.find_first_not_of(WS, start);
    // read until end of substring
    end = wholestring.find_first_of(WS, start);
    // push substring onto orth list
    result.push_back(wholestring.substr(start, end));
    // try next piece
    start = end;
  }
  return result;
}

/** Turn a whitespace separated list of typenames into a fs list of those
 *  types. 
 */
dag_node * dag_listify_internal(const char *typenamelist, dag_node **last) {
  int len = strlen(typenamelist);
  // Copy the string so it can be modified safely
  char *typenames = new char[len + 1];
  strcpy(typenames, typenamelist);

  *last = new_dag(BI_TOP);
  dag_node *current = *last;
  // Walk through the list of types in reverse order and build the list from
  // last to first, so the result will be in correct order again
  char *stringptr = typenames + len;
  while (stringptr >= typenames) {
    // skip trailing white space
    while (stringptr >= typenames && (*stringptr == ' ' || *stringptr == '\t'))
      stringptr--;
    stringptr[1] = '\0';
    // now look for the next whitespace char
    while (stringptr >= typenames && *stringptr != ' ' && *stringptr == '\t')
      stringptr--;
    if (stringptr[1] != '\0') {
      dag_node *new_cons = new_dag(BI_CONS);
      dag_add_arc(new_cons, new_arc(BIA_FIRST,
                                    new_dag(lookup_string(stringptr + 1))));
      dag_add_arc(new_cons, new_arc(BIA_REST, current));
      current = new_cons;
    }
  }
  delete[] typenames;
  return current;
}


dag_node *dag_listify_string(const char *typenamelist) {
  dag_node *last;
  dag_node *list = dag_listify_internal(typenamelist, &last);
  dag_set_type(last, BI_NIL);
  return list;
}

dag_node *dag_difflistify_string(const char *typenamelist) {
  dag_node *last;
  dag_node *list = dag_listify_internal(typenamelist, &last);
  dag_node *result = new_dag(BI_DIFF_LIST);
  dag_add_arc(result, new_arc(BIA_LIST, list));
  dag_add_arc(result, new_arc(BIA_LAST, last));
  return result;  
}


dag_node *tPSQLLex::dagify_slot_value(string slot_value, field_t slot_type) {
  // _fix_me_ massage slot_value according to current field's type
  type_t fstype;
  switch(slot_type) {
  case COL_SYMBOL:
    // lookup the
    fstype = lookup_type(slot_value.c_str());
    return (fstype == -1) ? NULL : new_dag(fstype);
  case COL_STRING:
    return new_dag(lookup_string(slot_value.c_str()));
  case COL_MIXED:
    if (slot_value[0] == '"') {
      // This is like a string slot, but remove the double quotes first
      string sub_value = string(slot_value, 1, slot_value.size() - 2);
      return new_dag(lookup_string(slot_value.c_str()));
    } else {
      // This is like a symbol slot
      fstype = lookup_type(slot_value.c_str());
      return (fstype == -1) ? NULL : new_dag(fstype);
    }
  case COL_STRING_LIST:
    return dag_listify_string(slot_value.c_str());
  case COL_STRING_DIFFLIST:
    return dag_difflistify_string(slot_value.c_str());
  default:
    warn((string) "Unhandled slot type occurs in database: " 
         + field_spec::field_name(slot_type) 
         + " (or deprecated equivalent)");
    break;
  }
  return NULL;
}


/** Employ the least  possible effort to add the path/value into the root
 * feature structure destructively. 
 */
dag_node *dag_add_path(dag_node *root, list_int *path, dag_node *value) {
  if (path == NULL) {
    return dag_unify(root, root, value, NULL) ;
  } else {
    dag_node *current_node = root;
    dag_arc *current_arc;
    do {
      current_arc = dag_find_attr(current_node->arcs, path->val);
      if (current_arc == NULL) {
        current_arc = new_arc(path->val, new_dag(apptype[path->val]));
        dag_add_arc(current_node, current_arc);
      }
      current_node = current_arc->val;
      path = path->next;
    } while (path != NULL);
    current_arc->val = dag_unify(current_node, current_node, value, NULL);
    return root;
  }
}


/** Build an `instance' feature structure based on the grammar fields in the
 *  database.
 *  \param res the current query result from the database for a stem form
 *  \param row_no the row number to be treated now
 *  \param col the column number of the first non-fixed
 *                 (i.e. grammar-specified) retrieved column
 *  \returns a permanent feature structure containing the paths and value types
 *           specific for this lexicon entry
 */
dag_node *
tPSQLLex::build_instance_fs(PGresult *res, int row_no, int col) {
  assert(grammar_field.size() == (unsigned) PQnfields(res));

  char *slot_value;
  field_list::iterator it = grammar_field.begin() + col;
  // turn the non-empty grammar fields into a feature structure
  struct dag_alloc_state s;
  dag_alloc_mark(s);

  dag_node *instance_fs = new_dag(BI_TOP);
  while (it != grammar_field.end()) {
    slot_value = PQgetvalue(res, row_no, col);
    // if empty, continue
    if (slot_value != NULL && *slot_value != '\0') {
      dag_node *value = dagify_slot_value(slot_value, it->field_type);
      if (value != NULL) {
        // (almost) unchecked build of instance fs, failures should in any case
        // report during instantiate
        instance_fs = dag_add_path(instance_fs, it->path, value);
        if (instance_fs == FAIL)
          throw tError((string) "Inconsistent lexicon entry " 
                       + PQgetvalue(res, row_no, 0));
      }
      else
        throw tError((string) "Unknown type in lexicon entry " 
                     + PQgetvalue(res, row_no, 0));
      
    }
    col++;
    it++;
  }
  dag_node *result = dag_full_p_copy(instance_fs);
  dag_alloc_release(s);
  return result;
}

/** Build a lex_stem from a row of a lexdb query for some stem.
 * \param res the current query result from the database for a stem form
 * \param row_no the row number to be treated now
 */
lex_stem *tPSQLLex::build_lex_stem(PGresult *res, int row_no) {
  // copy pre-build fs template for lex entries or construct new?
  // disadvantage of using full template: lots of empty fields in database
  lex_stem *result = NULL;
  try {
    // get the instance name of the lexicon entry
    char *currval = PQgetvalue(res, row_no, 0);
    if (currval == NULL) 
      entry_error("Wrong entry in lexdb: no name", res, row_no);
    // The instance type is potentially a dynamic type
    type_t instance_type = lookup_symbol(currval);

    // get the type name and retrieve the type code
    currval = PQgetvalue(res, row_no, 1);
    if (currval == NULL) 
      entry_error("Wrong entry in lexdb: no stem type", res, row_no);
      
    type_t stem_type = lookup_type(currval);
    if (stem_type == -1)
      entry_error("Unknown stem type in lexdb", res, row_no);
      
    char *currorth = PQgetvalue(res, row_no, 2);
    if (currorth == NULL || currorth[0] == '\0')
      entry_error("No orthography in entry", res, row_no);
    
    // This fs would be the `type dag of instance_type', if it were in the
    // ordinary grammar
    dag_node *instance_fs = build_instance_fs(res, row_no, 2);

    // register this type fs.

    // _fix_me_ do we need the hierarchy link? If the instance name is only
    // needed for printing or treebanking, the current solution should be OK.
    // Otherwise:
    // Add instance_type as leaf type with parent stem_type and typedag
    // instance_fs to the hierarchy, if necessary and possible
    result = new lex_stem(instance_type, stem_type, split_ws(currorth));
  }
  catch (tError e) {
    fprintf(stderr, "%s\n", e.getMessage().c_str()) ;
    return NULL;
  }
  return result;
}

/** Retrieve the lex_stem structures for a particular stem from the database
 */
list< lex_stem * > tPSQLLex::retrieve_lex_stems(const string &plain_stem) {
  list< lex_stem * > result;

  /* start a transaction block */
  PGresult *res = PQexec(_conn, "BEGIN");
  if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
      fprintf(stderr, "BEGIN command failed\n");
      PQclear(res);
      return result;
    }
  /*
   * should PQclear PGresult whenever it is no longer needed to avoid
   * memory leaks
   */
  PQclear(res);
    
  // Quote the stem string
  char *stem = new char[2*(plain_stem.size() + 1)];
  PQescapeString(stem, plain_stem.c_str(), plain_stem.size());

  /*
   * fetch all rows that are compatible with the stem
   */
  string command = (string) "SELECT " + _fieldlist +
    " FROM (SELECT lex.* FROM lex JOIN lex_key USING (name,userid,modstamp) "
    "WHERE lex_key.key LIKE '" + stem + "') AS foo";
  delete[] stem;

  res = PQexec(_conn, command.c_str());
  if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "%s command failed\n", command.c_str());
    PQclear(res);
    return result;
  }
  // treat the resulting rows from the database
  lex_stem *rowresult;
  for (int row = 0; row < PQntuples(res); row++) {
    rowresult = build_lex_stem(res, row);
    if (rowresult != NULL) result.push_back(rowresult);
  }

  // free the current result
  PQclear(res);
    
  /* commit the transaction */
  res = PQexec(_conn, "COMMIT");
  PQclear(res);

  return result;
}


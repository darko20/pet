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

/** Implements the tLexicon interface accessing a Postgres database. */
class tPSQLLex : public tLexicon
{
 public:
    /** Construct from a connection info string, and the names of the
        descriptor table, and the lexicon table. */
    tPSQLLex(const string &connectionInfo,
             const string &descTable,
             const string &lexTable);

    /** Destructor. Shuts down open connection. */
    ~tPSQLLex();

    /** Obtain list of stems for given orthography. */
    list<tLexStem>
    lookupWord(const string &orth);
    
 private:
    PGconn *_conn;

}


#endif

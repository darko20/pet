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
// This modules implements the link to an external lexical database.
//

#include "pet-system.h"
#include "lexdb.h"

tPSQLLex::tPSQLLex(const string &connection,
                   const string &descTable,
                   const string &lexTable)
    : _conn(0)
{
    // Connect to database.
    _conn = PQconnectdb(connection.c_str());
    if (PQstatus(_conn) == CONNECTION_BAD)
    {
        fprintf(stderr, "Connection to lexical database [%s] failed.\n",
                connection.c_str());
        fprintf(stderr, "%s", PQerrorMessage(_conn));
        PQfinish(_conn);
        _conn = 0;
    }
    
    // Obtain descriptor table.
    
 



}

tPSQLLex::~tPSQLLex()
{
    if(_conn)
        PQfinish(_conn);
}

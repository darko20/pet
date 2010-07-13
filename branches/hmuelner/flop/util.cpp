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

/* utility functions */

#include "flop.h"
#include "hierarchy.h"
#include "lex-io.h"
#include "utility.h"

using namespace std;

map<string, int> name_type;

int lookup(const string &s)
{
    map<string, int>::iterator it = name_type.find(s);
    if(it != name_type.end())
        return it->second;
    else
        return -1;
}

/** the constructor of the type struct */
Type *new_type(const string &name, bool is_inst, bool define)
{
    Type* t = new Type();

    if(define) {
        t->tdl_instance = is_inst;
        t->id = types.add(name);
        types[t->id] = t;
        register_type(t->id);
    }
    else {
        t->id = -1;
    }
    return t;
}

/** Register a new builtin type with name \a name */
int
new_bi_type(const char *name)
{
    Type *t = new_type(name, false);
    t->def.assign("builtin", 0, 0);
    return t->id;
}

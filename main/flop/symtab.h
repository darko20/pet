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

/** \file symtab.h
 * Symbol tables: mappings from strings to information via unique numeric IDs.
 */

#include <string>
#include <map>

extern int Hash(const string &s);

/** Class template for symbol tables: tables that associate names (strings)
 *  with some other data via a unique id, that is maintained by the symbol
 *  table.
 * \pre info must be <em> default constructible </em>
 */
template <class info> class symtab
{
 private:
    vector<string> names;
    map<string, int> ids;
    
    vector<info> infos;
    int n;
    
 public:
    
    symtab()
        : names(), ids(), infos(), n(0)
    { }

    /** Add a new symbol \a name to the table */
    int
    add(const string& name)
    {
        names.push_back(name);
        infos.push_back(info());
        
        ids[name] = n;
        return n++;
        
    }
    
    /** Return the id for \a name, or -1 if it does not exist in the table. */
    int id(const string &name) const
    {
        map<string, int>::const_iterator it = ids.find(name);
        if(it != ids.end())
            return it->second;
        else
            return -1;
    }
    
    /** \brief Return the name associated with \a id. Will be the empty string
     *  if \a id is greater or equal to number().
     */
    const string &name(int id) const
    {
        return names[id];
    }
    
    /** Read/Write access to the info associated with id.
     * \pre \a id must be less than number()
     */
    info &operator[](int id)
    {
        return infos[id];
    }
    
    /** The number of symbols registered in this table */
    int number()
    {
        return n;
    }
};

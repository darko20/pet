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

/* support functions for dumping/undumping of the grammar */

#include "pet-system.h"
#include "errors.h"
#include "grammar-dump.h"

void
dump_header(dumper *f, char *desc)
{
    f->dump_int(DUMP_MAGIC);
    f->dump_int(DUMP_VERSION);
    f->dump_string(desc);
}

char *
undump_header(dumper *f, int &version)
{
    int magic;
    char *desc;
    
    magic = f->undump_int();
    
    if(magic != DUMP_MAGIC)
        throw tError("invalid grammar file");
    
    version = f->undump_int();
    if(version != DUMP_VERSION)
    {
        // From version 16 on we try to be backwards compatible,
        // but print a warning anyway.
        if(DUMP_VERSION >= 16 && version >= 15)
        {
            fprintf(stderr, "warning: loading grammar with backwards "
                    "compatible format (version %d, current %d)\n",
                    version, DUMP_VERSION); 
        }
        else
            throw tError("grammar file has incompatible version");
    }
    
    desc = f->undump_string();
    
    return desc;
}

dump_toc::dump_toc(dumper *dump)
    : _dump(dump)
{
    sectiontype s;
    long int offset;
    
    do {
      s = sectiontype(_dump->undump_int());
      if(s != SEC_NOSECTION)
      {
          offset = _dump->undump_int();
          _toc[s] = offset;
      }
    } while (s != SEC_NOSECTION);
}

bool
dump_toc::goto_section(sectiontype s)
{
    if(_toc.find(s) != _toc.end())
    {
        try 
        {
            _dump->seek(_toc[s]);
        }
        catch (tError &e) 
        {
            throw tError("grammar file is corrupted");
        }
        
        sectiontype actual = sectiontype(_dump->undump_int());
        if(actual != s)
            throw tError("grammar file is corrupted");
        
        return true;
    }
    return false;
}

dump_toc_maker::dump_toc_maker(dumper *dump)
    : _dump(dump), _open(true)
{
}

void
dump_toc_maker::add_section(sectiontype s)
{
    if(!_open)
        throw tError("TOC already closed");
    
    _dump->dump_int(s);
    _where[s] = _dump->dump_int_variable();
}

void
dump_toc_maker::close()
{
    if(!_open)
        throw tError("TOC already closed");
    
    _dump->dump_int(SEC_NOSECTION);
    _open = false;
}

void
dump_toc_maker::start_section(sectiontype s)
{
    if(_open)
        throw tError("TOC still open");
    
    if(_where.find(s) == _where.end())
        throw tError("Trying to start a section that is not in the TOC");
    
    _toc[s] = _dump->tell();
    _dump -> dump_int(s);
}

dump_toc_maker::~dump_toc_maker()
{
    if(_open)
        throw tError("TOC still open");
    
    for(map<sectiontype, long int>::iterator iter = _where.begin();
        iter != _where.end(); ++iter)
    {
        sectiontype s = iter->first;
        long int pos = iter->second;
        
        if(_toc.find(s) == _toc.end())
            throw tError("Undefined section in TOC");
        
        _dump->set_int_variable(pos, _toc[s]);
    }
}

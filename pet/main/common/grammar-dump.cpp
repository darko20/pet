/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* support functions for dumping/undumping of the grammar */

#include "pet-system.h"
#include "errors.h"
#include "grammar-dump.h"

void dump_header(dumper *f, char *desc)
{
  f->dump_int(DUMP_MAGIC);
  f->dump_int(DUMP_VERSION);
  f->dump_string(desc);
}

char *undump_header(dumper *f)
{
  int magic, version;
  char *desc;

  magic = f->undump_int();

  if(magic != DUMP_MAGIC)
    throw error("invalid grammar file");

  version = f->undump_int();
  if(version != DUMP_VERSION)
    throw error("grammar file has incompatible version");
  
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

bool dump_toc::goto_section(sectiontype s)
{
  if(_toc.find(s) != _toc.end())
    {
      _dump->seek(_toc[s]);
      sectiontype actual = sectiontype(_dump->undump_int());
      if(actual != s)
	throw error("grammar file is corrupted");

      return true;
    }
  return false;
}

dump_toc_maker::dump_toc_maker(dumper *dump)
  : _dump(dump), _open(true)
{
}

void dump_toc_maker::add_section(sectiontype s)
{
  if(!_open)
    throw error("TOC already closed");
  
  _dump->dump_int(s);
  _where[s] = _dump->dump_int_variable();
}

void dump_toc_maker::close()
{
  if(!_open)
    throw error("TOC already closed");

  _dump->dump_int(SEC_NOSECTION);
  _open = false;
}

void dump_toc_maker::start_section(sectiontype s)
{
  if(_open)
    throw error("TOC still open");
  
  if(_where.find(s) == _where.end())
    throw error("Trying to start a section that is not in the TOC");

  _toc[s] = _dump->tell();
  _dump -> dump_int(s);
}

dump_toc_maker::~dump_toc_maker()
{
  if(_open)
    throw error("TOC still open");

  for(map<sectiontype, long int>::iterator iter = _where.begin();
      iter != _where.end(); ++iter)
    {
      sectiontype s = iter->first;
      long int pos = iter->second;
     
      if(_toc.find(s) == _toc.end())
	throw error("Undefined section in TOC");
      
      _dump->set_int_variable(pos, _toc[s]);
    }
}

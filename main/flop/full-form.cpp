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

/* read a full form table */

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <fstream>

#include "flop.h"

list<ff_entry> fullforms;
list<irreg_entry> irregforms;

bool opt_inst_affixes = false;

int compare(const ff_entry &A, const ff_entry &B)
{
  return compare(A._preterminal, B._preterminal);
}

ostream& operator<<(ostream& O, const ff_entry& C)
{
  O << "[" << C._preterminal << "+" << C._affix << "@" << C._inflpos << " \""
    << C._form << "\" (" << C._fname << ":" << C._line << ")]";
  
  return O;
}

string get_string(string &line, int &pos, istream &I)
{
  while(isspace(line[pos])) pos++;
  
  if(line[pos] == '"')
    {
      int p2 = line.find('"', pos + 1);
      string s = line.substr(pos + 1, p2 - 1 - pos);
      pos = p2;
      return s;
    }
  else if(line.substr(pos, 4) == "NULL")
    {
      pos += 4;
      return string();
    }
  else
    {
      fprintf(ferr, "ill formed morph entry `%s'...\n", line.c_str());
      I.clear(ios::badbit);
      return string();
    }
}

int get_int(string &line, int &pos, istream &I)
{
  while(isspace(line[pos])) pos++;
  
  if(isdigit(line[pos]))
    {
      int p2 = pos;
      while(isdigit(line[p2])) p2++;
      string s = line.substr(pos, p2 - pos);
      int n = strtoint(s.c_str(), "full form table entry");
      return n;
    }
  else
    {
      fprintf(ferr, "ill formed morph entry `%s'...\n", line.c_str());
      I.clear(ios::badbit);
      return -1;
    }
}

istream& operator>>(istream& I, ff_entry& C)
{
  string line;
  /* expecting lines like:  {"allright_root", "all", NULL, NULL, 1, 2}, */
  
  getline(I, line);

  if(I.good())
    {
      int i;
      
      i = line.find('{', 0) + 1;
      C._preterminal = get_string(line, i, I);
      if(!C._preterminal.empty())
          C._preterminal = string("$") + C._preterminal;
      
      i = line.find(',', i) + 1;
      C._form = get_string(line, i, I);
      
      i = line.find(',', i) + 1;
      get_string(line, i, I);
      
      i = line.find(',', i) + 1;
      C._affix = get_string(line, i, I);
      if(opt_inst_affixes && !C._affix.empty())
          C._affix = string("$") + C._affix;

      i = line.find(',', i) + 1;
      C._inflpos = get_int(line, i, I);

      i = line.find(',', i) + 1;
      get_int(line, i, I);
    }
  return I;
}

void ff_entry::dump(dumper *f)
{
  int preterminal;
  int affix;
  int inflpos;

  preterminal = types.id(_preterminal);
  if(preterminal == -1)
    {
      fprintf(ferr, "unknown preterminal `%s'\n", _preterminal.c_str());
    }

  preterminal = leaftype_order[preterminal];

  affix = types.id(_affix);
  if(affix != -1)
    affix = leaftype_order[affix];

  if(!_affix.empty() && affix == -1)
    {
      fprintf(ferr, "unknown affix `%s' for `%s'\n", _affix.c_str(),
	      _preterminal.c_str());
    }

  inflpos = _inflpos == 0 ? 0 : _inflpos - 1;
  
  f->dump_int(preterminal);
  f->dump_int(affix);
  f->dump_char(inflpos);
  f->dump_string(_form.c_str());
}

void read_morph(string fname)
{
  int linenr = 0;
  std::ifstream f(fname.c_str());

  if(!f)
    {
      fprintf(ferr, "file `%s' not found. skipping...\n", fname.c_str());
      return;
    }

  if(flop_settings->lookup("affixes-are-instances"))
    opt_inst_affixes = true;

  fprintf(fstatus, "reading full form entries from `%s': ", fname.c_str());

  while(!f.eof())
  {
      ff_entry e;
      
      if(f >> e)
      {
          linenr++;
          e.setdef(fname, linenr);
          fullforms.push_front(e);
          if(verbosity > 4)
          {
              cerr << e << endl;
          }
          
      }
      else if(f.bad())
          f.clear();
  }
  
  fprintf(fstatus, "%d entries.\n", fullforms.size());
}

bool parse_irreg(string line)
{
  static char *suf = flop_settings->value("lex-rule-suffix");
  
  string s1, s2, s3;
  unsigned int p = 0, p1;

  while(isspace(line[p]) && p < line.length()) p++;
  if(p >= line.length()) return false; else p1 = p;
  while(!isspace(line[p]) && p < line.length()) p++;
  s1 = line.substr(p1, p - p1);

  while(isspace(line[p]) && p < line.length()) p++;
  if(p >= line.length()) return false; else p1 = p;
  while(!isspace(line[p]) && p < line.length()) p++;
  if(opt_inst_affixes) s2 = "$";
  s2 += line.substr(p1, p - p1);
  if(suf) s2 += string(suf);

  while(isspace(line[p]) && p < line.length()) p++;
  if(p >= line.length()) return false; else p1 = p;
  while(!isspace(line[p]) && p < line.length()) p++;
  s3 = line.substr(p1, p - p1);

  irregforms.push_back(irreg_entry(s1, s2, s3));

  return true;
}

void read_irregs(string fname)
{
  char buff[255];
  FILE *f = fopen(fname.c_str(), "r");

  if(f == 0)
    throw tError("Could not open irregular form file `" + fname + "'");

  if(flop_settings->lookup("affixes-are-instances"))
    opt_inst_affixes = true;

  while(fgets(buff, sizeof(buff), f))
    parse_irreg(buff);

  fclose(f);
}

void irreg_entry::dump(dumper *f)
{
  f->dump_string(_form.c_str());
  f->dump_string(_infl.c_str());
  f->dump_string(_stem.c_str());
}

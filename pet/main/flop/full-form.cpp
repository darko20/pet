/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* read a full form table */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <fstream>

#include "flop.h"
#include "utility.h"

list<morph_entry> vocabulary;

int compare(const morph_entry &A, const morph_entry &B)
{
  return compare(A.preterminal, B.preterminal);
}

ostream& operator<<(ostream& O, const morph_entry& C)
{
  O << "[" << C.preterminal << "," << C.form << "," << C.stem << "," << C.affix <<
    "," << C.inflpos << "," << C.nstems <<
    " @ " << C.fname << ":" << C.line << "}";
  
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

istream& operator>>(istream& I, morph_entry& C)
{
  string line;
  /* expecting lines of the form:  {"allright_root", "all", NULL, NULL, 1, 2}, */
  
  getline(I, line);

  if(I.good())
    {
      int i;
      
      i = line.find('{', 0) + 1;
      C.preterminal = get_string(line, i, I);
      if(!C.preterminal.empty())
	C.preterminal = string("$") + C.preterminal;
      
      i = line.find(',', i) + 1;
      C.form = get_string(line, i, I);
      
      i = line.find(',', i) + 1;
      C.stem = get_string(line, i, I);
      
      i = line.find(',', i) + 1;
      C.affix = get_string(line, i, I);

      i = line.find(',', i) + 1;
      C.inflpos = get_int(line, i, I);

      i = line.find(',', i) + 1;
      C.nstems = get_int(line, i, I);
    }
  return I;
}

bool read_morph(string fname)
{
  int linenr = 0;
  std::ifstream f(fname.c_str());

  if(!f)
    {
      fprintf(ferr, "file `%s' not found. skipping...\n", fname.c_str());
      return false;
    }
  
  fprintf(fstatus, "reading morphology entries from `%s': ", fname.c_str());

  while(!f.eof())
    {
      morph_entry e;

      if(f >> e)
	{
	  linenr++;
	  e.setdef(fname, linenr);
	  vocabulary.push_front(e);
	}
      else if(f.bad())
	f.clear();
    }

  fprintf(fstatus, "%d entries.\n", vocabulary.size());

  return true;
}

/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class to represent symbol tables */

#include <string>

#include <LEDA/array.h>
#include <LEDA/h_array.h>

extern int Hash(const string &s);

template <class info> class symtab {

  static const int INITIAL_SIZE = 128;

  leda_array<string> names;
  leda_h_array<string, int> ids;

  leda_array<info> infos;
  int n;

 public:
  
  symtab() : names(INITIAL_SIZE), ids(-1), infos(INITIAL_SIZE), n(0)
    { }

  int add(const string& name)
    {
      if(n > names.high())
	{
	  names.resize(0,n + INITIAL_SIZE);
	  infos.resize(0,n + INITIAL_SIZE);
	}

      names[n] = name;
      ids[name] = n;

      return n++;
    }

  int id(const string &name)
    {
      return ids[name];
    }
  
  const string &name(int id)
    {
      return names[id];
    }

  info &operator[](int id)
    {
      return infos[id];
    }

  int number()
    { return n; }
  
};

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

/* parse the settings file */

#include "pet-system.h"
#include "lex-tdl.h"
#include "settings.h"
#include "utility.h"
#include "types.h"

#ifdef WINDOWS
#define strcasecmp stricmp
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

char *settings::basename(const char *base)
{
  // return part between last slash and first dot after that
  char *slash = strrchr((char *) base, PATH_SEP[0]);
  if(slash == 0) slash = (char *) base; else slash++;
  char *dot = strchr(slash, '.');
  if(dot == 0) dot = slash + strlen(slash);;
  
  char *res = (char *) malloc(dot - slash + 1);
  strncpy(res, slash, dot - slash);
  res[dot-slash] = '\0';
  return res;
}

settings::settings(const char *name, const char *base, char *message)
  : _li_cache()
{
  _n = 0;
  _set = New setting*[SET_TABLE_SIZE];

  _prefix = 0;
  _fname = 0;

  if(base)
    {
      char *slash = strrchr((char *) base, PATH_SEP[0]);
      _prefix = (char *) malloc(strlen(base) + 1 + strlen(SET_SUBDIRECTORY) + 1);
      if(slash)
	{
	  strncpy(_prefix, base, slash - base + 1);
	  _prefix[slash - base + 1] = '\0';
	}
      else
	{
	  strcpy(_prefix, "");
	}

      char *fname = (char *) malloc(strlen(_prefix) + 1 + strlen(name) + 1);
      strcpy(fname, _prefix);
      strcat(fname, name);
      
      _fname = find_file(fname, SET_EXT, true);
      if(!_fname)
	{
	  strcat(_prefix, SET_SUBDIRECTORY);
	  strcat(_prefix, PATH_SEP);
          fname = (char *) realloc(fname, strlen(_prefix) + 1 + strlen(name) + 1);
	  strcpy(fname, _prefix);
	  strcat(fname, name);
	  _fname = find_file(fname, SET_EXT, true);
          free(fname);
	}
    }
  else
    {
      _fname = (char *) malloc(strlen(name) + 1);
      strcpy(_fname, name);
    }
  
  if(_fname)
    {
      push_file(_fname, message);
      free(_fname);
      parse();
    }
  _lloc = 0;
}


settings::~settings()
{
  for(int i = 0; i < _n; i++)
  {
    for(int j = 0; j < _set[i]->n; j++)
      free(_set[i]->values[j]);
    free(_set[i]->values);
    free(_set[i]->t_values);
    delete _set[i];
  }

  delete[] _set;

  free(_prefix);
  free(_fname);
}

setting *settings::lookup(const char *name)
{
  for(int i = 0; i < _n; i++)
    if(strcmp(_set[i]->name, name) == 0)
      {
	if(i != 0)
	  { // put to front, so further lookup is faster
	    setting *tmp;
	    tmp = _set[i]; _set[i] = _set[0]; _set[0] = tmp;
	  }

	_lloc = _set[0]->loc;
	return _set[0];
      }

  _lloc = 0;
  return 0;
}

char *settings::value(const char *name)
{
  setting *s;

  s = lookup(name);
  if(s == 0) return 0;

  return s->values[0];
}

char *settings::req_value(const char *name)
{
  char *v = value(name);
  if(v == 0)
    {
      fprintf(ferr, "\nno definition for required parameter `%s'\n", name);
      throw error("no definition for required parameter `" + string(name) + "'");
    }
  return v;
}

bool settings::member(const char *name, const char *value)
/* is value in the list for name? */
{
  setting *set = lookup(name);

  if(set == 0) return false;

  for(int i = 0; i < set->n; i++)
    if(strcasecmp(set->values[i], value) == 0)
      return true;
  
  return false;
}

char *settings::assoc(const char *name, const char *key, int arity, int nth)
{
  setting *set = lookup(name);
  if(set == 0) return 0;

  assert(nth <= arity && arity > 1);

  for(int i = 0; i < set->n; i+=arity)
    {
      if(i+nth >= set->n) return 0;
      if(strcasecmp(set->values[i], key) == 0)
	return set->values[i+nth];
    }

  return 0;
}

// initialize type name cache in settings (temporary solution)
void t_initialize(setting *set)
{
  for(int i = 0; i < set->n; i++)
    {
      char *v_lc = strdup(set->values[i]);
      strtolower(v_lc);
      set->t_values[i] = lookup_type(v_lc);
      free(v_lc);
    }
  set->t_initialized = true;
}

char *settings::sassoc(const char *name, int key_t, int arity, int nth)
{
  setting *set = lookup(name);
  if(set == 0) return 0;
  if(key_t == -1) return 0;

  assert(nth <= arity && arity > 1);

  if(!set->t_initialized)
    t_initialize(set);

  for(int i = 0; i < set->n; i+=arity)
    {
      if(i+nth >= set->n) return 0;
      int v_t = set->t_values[i];
      if(v_t == -1)
	{
	  fprintf(ferr, "warning: ignoring unknown type `%s' in setting `%s'\n",
		  set->values[i], name);
	  continue;
	}

      if(subtype(key_t, v_t))
	return set->values[i+nth];
    }

  return 0;
}

#ifndef FLOP
// subtype based map
set<string> settings::smap(const char *name, int key_type)
{
  set<string> res;

  setting *set = lookup(name);
  if(set == 0) return res;

  if(key_type == -1) return res;

  for(int i = 0; i < set->n; i+=2)
    {
      if(i+2 > set->n)
	{
	  fprintf(ferr, "warning: incomplete last entry in "
		  "`%s' mapping - ignored\n", name);
	  break;
	}
      
      char *lhs = set->values[i], *rhs = set->values[i+1];
      int id = lookup_type(lhs);
      if(id != -1)
	{
	  if(subtype(key_type, id))
	    res.insert(rhs);
	}
      else
	fprintf(ferr, "warning: unknown type `%s' in "
		"`%s' mapping - ignored\n", name, lhs);
      
    }

  return res;
}
#endif

bool settings::statusmember(const char *name, type_t key)
{
  list_int *l = _li_cache[string(name)];
  if(l == 0)
    {
      setting *set = lookup(name);
      if(set != 0)
	{
	  for(int i = 0; i < set->n; i++)
	    {
	      int v = lookup_status(set->values[i]);
	      if(v == -1)
		{
		  fprintf(ferr, "ignoring unknown status `%s' in setting "
			  "`%s'\n", set->values[i], name);
		}
	      else
		l = cons(v, l);
	    }
	  _li_cache[string(name)] = l;
	}
    }
  return contains(l, key);
}

void settings::parse_one()
{
  char *option;
  struct setting *set;
  option = LA(0)->text; LA(0)->text = NULL;
  consume(1);

  set = lookup(option);
  if(set)
    {
      fprintf(ferr, "warning: more than one definition for setting `%s'...\n", option);
    }
  else
    {
      assert(_n < SET_TABLE_SIZE);
      set = _set[_n++] = New setting;
      set->name = option;
      set->loc = LA(0)->loc; LA(0)->loc = NULL;
      set->n = 0;
      set->allocated = SET_TABLE_SIZE;
      set->values = (char **) malloc(set->allocated * sizeof(char *));
      set->t_values = (int *) malloc(set->allocated * sizeof(int));
      set->t_initialized = false;
    }

  if(LA(0)->tag != T_DOT)
    {
      match(T_ISEQ, "option setting", true);
      
      while(LA(0)->tag != T_DOT && LA(0)->tag != T_EOF)
	{
	  if(LA(0)->tag == T_ID || LA(0)->tag == T_KEYWORD ||
	     LA(0)->tag == T_STRING)
	    {
	      if(set->n >= set->allocated)
		{
		  set->allocated += SET_TABLE_SIZE;
		  set->values = (char **) realloc(set->values, set->allocated * sizeof(char *));
		  set->t_values = (int *) realloc(set->t_values, set->allocated * sizeof(int));

		}
	      set->values[set->n++] = LA(0)->text; LA(0)->text=NULL;
	    }
	  else
	    {
	      fprintf(ferr, "ignoring `%s' at %s:%d...\n", LA(0)->text,
		      LA(0)->loc->fname, LA(0)->loc->linenr);
	    }
	  
	  consume(1);
	}
    }

  match(T_DOT, "end of option setting", true);
}

void settings::parse()
{
  do
    {
      if(LA(0)->tag == T_ID)
        {
          parse_one();
        }
      else if(LA(0)->tag == T_COLON)
	{
	  consume(1);
	}
      else if(LA(0)->tag == T_KEYWORD && strcmp(LA(0)->text, "include") == 0)
	{
	  consume(1);

	  if(LA(0)->tag != T_STRING)
	    {
	      fprintf(ferr, "expecting include file name at %s:%d...\n",
		      LA(0)->loc->fname, LA(0)->loc->linenr);
	    }
	  else
	    {
	      char *ofname, *fname;

	      ofname = (char *) malloc(strlen(_prefix) + strlen(LA(0)->text) + 1);
              strcpy(ofname, _prefix);
	      strcat(ofname, LA(0)->text);
	      consume(1);

	      match(T_DOT, "`.'", true);

	      fname = find_file(ofname, SET_EXT, true);
	  
	      if(!fname)
		{
		  fprintf(ferr, "file `%s' not found. skipping...\n", ofname);
		}
	      else
		{
		  push_file(fname, "including");
		}
	    }
	}
      else
        {
          syntax_error("expecting identifier", LA(0));
	  consume(1);
        }
    } while(LA(0)->tag != T_EOF);

  consume(1);
}

/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* parse the settings file */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "lex-tdl.h"
#include "settings.h"
#include "utility.h"
#include "types.h"

#ifdef WINDOWS
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

settings::settings(const char *name, char *base, char *message)
{
  _n = 0;
  _set = new setting*[SET_TABLE_SIZE];

  _prefix = 0;
  _fname = 0;

  if(base)
    {
      char *slash = strrchr(base, PATH_SEP[0]);
      _prefix = (char *) malloc(strlen(base));
      if(slash)
	{
	  strncpy(_prefix, base, slash - base + 1);
	  _prefix[slash - base + 1] = '\0';
	}
      else
	{
	  strcpy(_prefix, "");
	}

      char *fname = (char *) malloc(strlen(_prefix) + strlen(SET_SUBDIRECTORY) + 1 + strlen(name) + 1);
      strcpy(fname, _prefix);
      strcat(fname, name);
      
      _fname = find_file(fname, SET_EXT, true);
      if(!_fname)
	{
	  strcpy(fname, _prefix);
	  strcat(fname, SET_SUBDIRECTORY);
	  strcat(fname, PATH_SEP);
	  strcat(fname, name);
	  _fname = find_file(fname, SET_EXT, true);
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
      char *sv = lexer_idchars;
      lexer_idchars = "_+-*?$";
      parse();
      lexer_idchars = sv;
    }
  _lloc = 0;
}


settings::~settings()
{
  for(int i = 0; i < _n; i++)
    delete _set[i];

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
      exit(0);
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

char *settings::sassoc(const char *name, const char *key, int arity, int nth)
{
  setting *set = lookup(name);
  if(set == 0) return 0;

  int key_t = lookup_type(key);
  if(key_t == -1)
    {
      fprintf(stderr, "warning: unknown type `%s' [while checking `%s']\n",
	      key, name);
      return 0;
    }

  assert(nth <= arity && arity > 1);

  for(int i = 0; i < set->n; i+=arity)
    {
      if(i+nth >= set->n) return 0;
      char *v_lc = strdup(set->values[i]);
      strtolower(v_lc);
      int v_t = lookup_type(v_lc);
      if(v_t == -1)
	{
	  fprintf(stderr, "warning: ignoring unknown type `%s' in setting `%s'\n",
		  v_lc, name);
	  continue;
	}

      if(subtype(key_t, v_t))
	return set->values[i+nth];
    }

  return 0;
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
      set = _set[_n++] = (struct setting *) malloc(sizeof(struct setting));
      set->name = option;
      set->loc = LA(0)->loc; LA(0)->loc = NULL;
      set->n = 0;
      set->allocated = SET_TABLE_SIZE;
      set->values = (char **) malloc(set->allocated * sizeof(char *));
    }

  if(LA(0)->tag != T_DOT)
    {
      match(T_ISEQ, "option setting", true);
      
      while(LA(0)->tag != T_DOT && LA(0)->tag != T_EOF)
	{
	  if(LA(0)->tag == T_ID || LA(0)->tag == T_KEYWORD ||
	     LA(0)->tag == T_STRING || LA(0)->tag == T_INT ||
	     LA(0)->tag == T_FLOAT)
	    {
	      if(set->n >= set->allocated)
		{
		  set->allocated+=SET_TABLE_SIZE;
		  set->values = (char **) realloc(set->values, set->allocated * sizeof(char *));
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
	      strcpy(ofname, LA(0)->text);
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

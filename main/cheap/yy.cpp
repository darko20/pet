//
//      The entire contents of this file, in printed or electronic form, are
//      (c) Copyright YY Technologies, Mountain View, CA 1991,1992,1993,1994,
//                                                       1995,1996,1997,1998,
//                                                       1999,2000,2001
//      Unpublished work -- all rights reserved -- proprietary, trade secret
//

// YY input format and server mode (oe, uc)

#ifdef SOCKET_INTERFACE
#include <unistd.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#endif

#include "pet-system.h"
#include "mfile.h"
#include "../common/utility.h"
#include "../common/errors.h"
#include "lex-tdl.h"
#include "tokenizer.h"
#include "parse.h"
#include "agenda.h"
#include "chart.h"
#include "cheap.h"
#include "inputchart.h"
#include "typecache.h"
#include "tsdb++.h"
#include "k2y.h"
#include "yy.h"
#ifdef ONLINEMORPH
#include "morph.h"
#endif
#ifdef ICU
#include "unicode.h"
#endif

//
// parsing of YY input tokens
// the form of one token is (id, start, end, path+, stem surface, ipos, irule+, {tag prob}*)
//

class yy_token
{
public:
  int id;
  int start, end;
  list<int> paths;
  string stem;
  string surface;
  int inflpos;
  vector<string> inflrs;
  vector<string> tags;
  vector<double> probs;

  void print(FILE *f);
};

void yy_token::print(FILE *f)
{
  fprintf(f, "[%d - %d] (%d) \"%s\" \"%s\" ", start, end, id, stem.c_str(), surface.c_str());

  for(vector<string>::const_iterator it = inflrs.begin(); it != inflrs.end(); ++it)
    fprintf(f, "+%s", it->c_str());

  fprintf(f, "@%d", inflpos);
}

void
yy_tokenizer::add_tokens(input_chart *i_chart)
{

#ifdef CREATE_INPUT_LOG
    FILE *fx = fopen("f.x", "a");
#endif    

    if(verbosity > 4)
        fprintf(ferr, "yy_tokenizer:");

    yy_token *tok = 0;

    while((tok = read_token()) != 0)
    {
        auto_ptr<yy_token> tok_owner(tok);

        postags poss(tok->tags, tok->probs);

        if(verbosity > 4)
        {
            tok->print(ferr);
            fprintf(ferr, " {");
            poss.print(ferr);
            fprintf(ferr, "}");
        }

#ifdef CREATE_INPUT_LOG
        for(vector<string>::reverse_iterator it = tok->inflrs.rbegin();
            it != tok->inflrs.rend();
            ++it)
        {
            if(*it == "zero")
            {
                if(tok->start + 1 == tok->end
                   && !poss.contains("SpellCorrection"))
                    fprintf(fx, "%s ", tok->surface.c_str());
                break;
            }
        }
#endif

        // skip empty tokens and punctuation
        if(tok->stem.empty() || Grammar->punctuationp(tok->stem))
        {
            if(verbosity > 4)
                fprintf(ferr, " - punctuation");

            continue;
        }

        // Unless signalled by a special "null" token, we expect stems and
        // inflectional rules. If we find a "null" token, the flag
        // `formlookup' will be set, and we do morphological analysis in house.
        bool formlookup = false;

        // We want to ignore tokens containing unknown infl rules.
        bool ignore;
        ignore = false;
        
        list_int *inflrs = 0;
        for(vector<string>::reverse_iterator it = tok->inflrs.rbegin();
            it != tok->inflrs.rend();
            ++it)
        {
            if(*it == "zero")
                continue;

            if(*it == "null")
            {
                formlookup = true;
                break;
            }
	  
            int rule = lookup_type(it->c_str());
            if(rule == -1)
            {
                if(verbosity > 4)
                    fprintf(ferr, "Ignoring token containing unknown "
                            "infl rule %s.\n", it->c_str());
                ignore = true;
                break;
            }
            
            inflrs = cons(rule, inflrs);
        }

        if(ignore)
        {
            free_list(inflrs);
            continue;
        }
        
        if(formlookup)
        {
            list<full_form> forms = Grammar->lookup_form(tok->stem);

            if(forms.empty())
            {
                i_chart->add_token(tok->id, tok->start, tok->end,
                                   full_form(), tok->surface, 0, poss,
                                   tok->paths);
            }
            else
            {
                for(list<full_form>::iterator currf = forms.begin();
                    currf != forms.end(); ++currf)
                {
                    i_chart->add_token(tok->id, tok->start, tok->end,
                                       *currf, tok->surface, currf->priority(),
                                       poss, tok->paths);
                }
            }
        }
        else
        {
            list<lex_stem *> stems = Grammar->lookup_stem(tok->stem);

            if(stems.empty())
            {
                i_chart->add_token(tok->id, tok->start, tok->end,
                                   full_form(0, modlist(), inflrs),
                                   tok->surface, 0, poss, tok->paths);
            }
            else
            {
                for(list<lex_stem *>::iterator currs = stems.begin();
                    currs != stems.end(); ++currs)
                {
                    full_form f(*currs, modlist(), inflrs);
                    i_chart->add_token(tok->id, tok->start, tok->end,
                                       f, tok->surface, f.priority(), poss,
                                       tok->paths);
                }
            }
        }

        free_list(inflrs);
    }

#ifdef CREATE_INPUT_LOG
    fprintf(fx, "\n");
    fclose(fx);
#endif

    if(verbosity > 4)
    {
        fprintf(ferr, "\n");
    }
}

bool yy_tokenizer::eos()
{
  return _yypos >= _yyinput.length();
}

char yy_tokenizer::cur()
{
  if(eos())
    throw error("yy_tokenizer: trying to read beyond end of string");
  return _yyinput[_yypos];
}

const char *yy_tokenizer::res()
{
  if(eos())
    throw error("yy_tokenizer: trying to read beyond end of string");
  return _yyinput.c_str() + _yypos;
}

bool yy_tokenizer::adv(int n)
{
  while(n > 0)
    {
      if(eos())
	return false;
      _yypos++; n--;
    }
  return true;
}

bool yy_tokenizer::read_ws()
{
  if(eos())
    return false;
  
  while(!eos() && isspace(cur())) adv();

  return true;
}

bool yy_tokenizer::read_special(char c)
{
  read_ws();

  if(eos())
    return false;

  if(cur() != c)
    return false;

  adv();

  return true;
}

bool yy_tokenizer::read_int(int &target)
{
  read_ws();

  if(eos())
    return false;
  
  const char *begin = res();
  char *end;

  target = strtol(begin, &end, 10);
  if(begin == end)
    return false;

  adv(end-begin);

  return true;
}

bool yy_tokenizer::read_double(double &target)
{
  read_ws();

  if(eos())
    return false;
  
  const char *begin = res();
  char *end;

  target = strtod(begin, &end);
  if(begin == end)
    return false;

  adv(end-begin);

  return true;
}

bool yy_tokenizer::read_string(string &target, bool quotedp, bool downcasep)
{
  target = "";

  read_ws();

  if(eos())
    return false;

  if(quotedp)
    {
      if(cur() != '"')
	return false;
      else
	adv();

      char last = (char)0;
      while(!eos() && (cur() != '"' || last == '\\'))
	{
          if(cur() != '\\' || last == '\\') {
            if(downcasep && (unsigned char)cur() < 127)
              target += tolower(cur());
            else
              target += cur();
          } // if
          last = (last == '\\' ? 0 : cur());
	  adv();
	}

      if(cur() != '"')
	return false;
      else
	adv();
    }
  else
    {
      while(!eos() && (is_idchar(cur())))
	{
	  target +=
            (downcasep && (unsigned char)cur() < 127 ? tolower(cur()) : cur());
	  adv();
	}
      if(target.empty())
	return false;
    }

  return true;
}

bool yy_tokenizer::read_pos(string &tag, double &prob)
{
  tag = ""; prob = 0;

  read_ws();

  if(eos())
    return false;

  if(read_string(tag, true))
    {
      read_ws();

      if(eos())
	return false;

      if(read_double(prob))
	return true;
      else
	return false;
    }
  else
    return false;
}

class yy_token *
yy_tokenizer::read_token()
{
    auto_ptr<yy_token> res(New yy_token);
    
    read_ws();
    
    if(eos())
        return 0;
  
    if(!read_special('('))
        throw error("yy_tokenizer: ill-formed token (expected '(')");

    if(!read_int(res->id) || !read_special(','))
        throw error("yy_tokenizer: ill-formed token (expected id)");

    if(!read_int(res->start) || !read_special(','))
        throw error("yy_tokenizer: ill-formed token (expected start pos)");
 
    if(!read_int(res->end) || !read_special(','))
        throw error("yy_tokenizer: ill-formed token (expected end pos)");
 
    int path;
    while(read_int(path))
        res->paths.push_back(path);

    if(res->paths.empty() || !read_special(','))
        throw error("yy_tokenizer: ill-formed token (expected paths)");

    bool downcasep = true;
    if(!read_string(res->stem, true, downcasep))
        throw error("yy_tokenizer: ill-formed token (expected stem)");

    // Read (optional) surface string.
    read_string(res->surface, true);

    if(!read_special(','))
        throw error("yy_tokenizer: ill-formed token (expected , after stem)");

    if(!read_int(res->inflpos) || !read_special(','))
        throw error("yy_tokenizer: ill-formed token (expected inflpos)");

    string inflr;
    while(read_string(inflr, true))
        res->inflrs.push_back(inflr);
    
    if(res->inflrs.empty())
        throw error("yy_tokenizer: ill-formed token (expected inflrs)");

    if(read_special(','))
    {
        string tag; double prob;
        while(read_pos(tag, prob))
        {
            res->tags.push_back(tag);
            res->probs.push_back(prob);
        }
    }
    
    if(!read_special(')'))
        throw error("yy_tokenizer: ill-formed token (expected ')')");
    
    return res.release();
}

//
// library based interface to language server
//

// all strings coming in over this interface are UTF8 encoded. Ignore this
// for now on filenames. Do appropriate conversion for rest.

#include "../l2lib.h"

// initialize parser with specified grammar

void l2_parser_init(const string& grammar_path, const string& log_file_path,
		      int k2y_segregation_p)
{
  fstatus = fopen(log_file_path.c_str(), "w");
  if(fstatus == 0)
    throw l2_error("Unable to open log file " + log_file_path);
  ferr = fstatus;

  try {
    fprintf(fstatus, "[%s] l2_parser_init(\"%s\", \"%s\", %d)\n",
	    current_time(), grammar_path.c_str(), log_file_path.c_str(),
	    k2y_segregation_p);

    cheap_settings = New settings(settings::basename(grammar_path.c_str()),
				  grammar_path.c_str(), "reading");
    fprintf(fstatus, "\n");

    options_from_settings(cheap_settings);

    //
    // only reset K2Y modifier segregation if explicity requested from language
    // server `.ini' file (hence the three-valued logix).
    //
    if(k2y_segregation_p >= 0) opt_k2y_segregation = k2y_segregation_p;

#ifndef DONT_EDUCATE_USERS
    if(opt_yy == false)
      {
	fprintf(fstatus, "you want YY mode\n");
	opt_yy = true;
      }

    if(opt_k2y == 0)
      {
	fprintf(fstatus, "you want k2y's - turning on opt_k2y\n");
	opt_k2y = 50;
      }

    if(opt_default_les == false)
      {
	fprintf(fstatus, "you want default lexical entries  - turning on opt_default_les\n");
	opt_default_les = true;
      }

    if(pedgelimit == 0)
      {
	fprintf(fstatus, "you don't want to parse forever - setting edge limit\n");
	pedgelimit = 10000;
      }

    if(memlimit == 0)
      {
	fprintf(fstatus, "you don't want to use up all memory - setting mem limit\n");
	memlimit = 50 * 1024 * 1024;
      }
#endif

    timer t_start;
    fprintf(fstatus, "loading `%s' ", grammar_path.c_str());
    Grammar = New grammar(grammar_path.c_str());

    fprintf(fstatus, "\n%d types in %0.2g s\n",
	    ntypes, t_start.convert2ms(t_start.elapsed()) / 1000.);

    initialize_version();

    fflush(fstatus);
  }

  catch(error &e)
    {
      fprintf(fstatus, "\nL2 error: `");
      e.print(fstatus); fprintf(fstatus, "'.\n");
      fflush(fstatus);

      throw l2_error(e.msg());
    }
}

extern typecache glbcache;
extern int total_cached_constraints;

#ifdef TSDBFILEAPI
tsdb_parse *TsdbParse = 0;
#endif

// parse one input item; return string of K2Y's
string l2_parser_parse(const string &inputUTF8, int nskip)
{
    static int item_id = 0;

    fs_alloc_state FSAS;

    string input = Conv->convert(ConvUTF8->convert(inputUTF8));

    fprintf(fstatus, "[%s] (%d) l2_parser_parse(\"%s\", %d)\n", current_time(),
            stats.id, input.c_str(), nskip);

    fprintf(fstatus, " Permanent dag storage: %d*%dk\n Dynamic dag storage: %d*%dk\n GLB cache: %d*%dk, Constraint cache: %d entries\n",
            p_alloc.nchunks(), p_alloc.chunksize() / 1024,
            t_alloc.nchunks(), t_alloc.chunksize() / 1024,
            glbcache.alloc().nchunks(), glbcache.alloc().chunksize() / 1024,
            total_cached_constraints);

    opt_nth_meaning = nskip + 1;

    string result;
    chart *Chart = 0;

#ifdef TSDBFILEAPI
    if(TsdbParse)
        delete TsdbParse;
    TsdbParse = new tsdb_parse;
    TsdbParse->set_input(input);
#endif

    try
    {
        item_id++;

        input_chart i_chart(New end_proximity_position_map);

        analyze(i_chart, input, Chart, FSAS, item_id);

        fprintf(fstatus," (%d) [%d] --- %d (%.1f|%.1fs) <%d:%d> (%.1fK) [%.1fs]\n",
                stats.id, pedgelimit, stats.readings,
                stats.first / 1000., stats.tcpu / 1000.,
                stats.words, stats.pedges, stats.dyn_bytes / 1024.0,
                TotalParseTime.elapsed_ts() / 10.);

        if(verbosity > 1) stats.print(fstatus);
        fflush(fstatus);

#ifdef TSDBFILEAPI
        TsdbParse->set_i_length(Chart->length());
        cheap_tsdb_summarize_item(*Chart, i_chart.max_position(), -1, 0, 0,
                                  *TsdbParse);
#endif

        struct MFILE *mstream = mopen();
    
        int nres = 1, skipped = 0;
        for(vector<item *>::iterator iter = Chart->Roots().begin();
            iter != Chart->Roots().end(); ++iter)
        {	
            mflush(mstream);
            int n = construct_k2y(nres++, *iter, false, mstream);
            if(n >= 0)
            {
                if(skipped >= nskip)
                {
                    if(verbosity > 0)
                    {
                        fprintf(fstatus, "syntax:");
                        (*iter)->print_derivation(fstatus, false);
                        fprintf(fstatus, "\nsemantics:\n%s\n",
                                mstring(mstream));
                        fflush(fstatus);
                    }

                    result += string(mstring(mstream));
                    if(skipped == nskip)
                        break;
                }
                else
                    skipped++;
            }
        }
        mclose(mstream);
    } /* try */

    catch(error_ressource_limit &e)
    {
        fprintf(fstatus, " (%d) [%d] --- edge limit exceeded "
                "(%.1f|%.1fs) <%d:%d> (%.1fK)\n",
                stats.id, pedgelimit,
                stats.first / 1000., stats.tcpu / 1000.,
                stats.words, stats.pedges, stats.dyn_bytes / 1024.0);
        fflush(fstatus);

#ifdef TSDBFILEAPI
        if(Chart)
            TsdbParse->set_i_length(Chart->length());
        cheap_tsdb_summarize_error(e, -1, *TsdbParse);
#endif
    }

    catch(error &e)
    {
        fprintf(fstatus, " L2 error: `");
        e.print(fstatus); fprintf(fstatus, "'.\n");
        fflush(fstatus);

#ifdef TSDBFILEAPI
        if(Chart)
            TsdbParse->set_i_length(Chart->length());
        cheap_tsdb_summarize_error(e, -1, *TsdbParse);
#endif

        delete Chart;
        fflush(fstatus);
        throw l2_error(e.msg());
    }

    delete Chart;
    fflush(fstatus);
    return ConvUTF8->convert(Conv->convert(result));
}

void l2_tsdb_write(FILE *f_parse, FILE *f_result, FILE *f_item, const string& roletable)
{
#ifdef TSDBFILEAPI
  if(TsdbParse == 0)
    return;

  TsdbParse->set_rt(roletable);

  TsdbParse->file_print(f_parse, f_result, f_item);
  delete TsdbParse;
  TsdbParse = 0;
#endif
}

vector<l2_morph_analysis> l2_morph_analyse(const string& formUTF8)
{
  vector<l2_morph_analysis> results;

  string form = Conv->convert(ConvUTF8->convert(formUTF8));

#ifndef ONLINEMORPH
  throw l2_error("L2 morphology component not available");
#else
  try
  {
    list<morph_analysis> res = Grammar->morph()->analyze(form, false);
    for(list<morph_analysis>::iterator it = res.begin(); it != res.end(); ++it)
      {
	l2_morph_analysis a;

	for(list<string>::iterator f = it->forms().begin();
            f != it->forms().end();
            ++f)
	  a.forms.push_back(ConvUTF8->convert(Conv->convert(*f)));

	for(list<type_t>::iterator r = it->rules().begin();
            r != it->rules().end();
            ++r)
	  a.rules.push_back(ConvUTF8->convert(Conv->convert(string(typenames[*r]))));

	results.push_back(a);
      }
  }

  catch(error &e)
  {
    fprintf(fstatus, " L2 error: `");
    e.print(fstatus); fprintf(fstatus, "'.\n");
    fflush(fstatus);

    throw l2_error(e.msg());
  }
#endif

  return results;
}

string l2_morph_analyse_imp(const string& formUTF8)
{
  string results = "";
  string form = Conv->convert(ConvUTF8->convert(formUTF8));

#ifndef ONLINEMORPH
  throw l2_error("L2 morphology component not available");
#else
  try
  {
    list<morph_analysis> res = Grammar->morph()->analyze(form, false);
    for(list<morph_analysis>::iterator it = res.begin(); it != res.end(); ++it)
      {
	for(list<string>::iterator f = it->forms().begin();
            f != it->forms().end();
            ++f) {
          results += ConvUTF8->convert(Conv->convert(*f));
          results += "\f+\nF";
        }

	for(list<type_t>::iterator r = it->rules().begin();
            r != it->rules().end();
            ++r) {
          results += ConvUTF8->convert(Conv->convert(string(typenames[*r])));
          results += "\f+\nR";
        }
        results += "\f+\nA";
       }
    results += "\f+\nL";
  }

  catch(error &e)
  {
    fprintf(fstatus, " L2 error: `");
    e.print(fstatus); fprintf(fstatus, "'.\n");
    fflush(fstatus);

    throw l2_error(e.msg());
  }
#endif

  return results;
}

bool
l2_parser_punctuationp(const string &input)
{
  try
  {
    return Grammar->punctuationp(input);
  }
  catch(error &e)
  {
    fprintf(fstatus, " L2 error: `");
    e.print(fstatus); fprintf(fstatus, "'.\n");
    fflush(fstatus);

    throw l2_error(e.msg());
  }
}

// free resources of parser
void l2_parser_exit_imp()
{
  fprintf(fstatus, "[%s] l2_parser_exit_imp()\n", current_time());

  try // destructors should not throw, but we want to be safe
  { 
    delete Grammar;
    delete cheap_settings;
#ifdef TSDBFILEAPI
    if(TsdbParse)
    {
      delete TsdbParse;
      TsdbParse = 0;
    }
#endif
  }

  catch(error &e)
  {
    fprintf(fstatus, " L2 error: `");
    e.print(fstatus); fprintf(fstatus, "'.\n");
    fflush(fstatus);
    
    throw l2_error(e.msg());
  }
  
  fclose(fstatus);
}

#ifdef SOCKET_INTERFACE

//
// socket-based server mode for cheap parser
//

#define SOCKETDEBUG
#define FOREGROUND

//
// it seems signal delivery on child death is unreliable in CygWin, so have to
// disable multi-threaded mode in CygWin for now            (15-arp-01  -  oe)
//
#if defined(__CYGWIN__)
# define NOFORK
#endif
#define NOFORK
#if defined(NOFORK)
# define FOREGROUND
#endif

#if defined(unix)
#  if defined(sun) && defined(__svr4__)
#    define __SOLARIS__
#  elif defined(sun) && !defined(__svr4__) && !defined(__SUNOS__)
#    define __SUNOS__
#  elif defined(linux) && !defined(__LINUX__)
#    define __LINUX__
#  elif defined(_OSF_SOURCE) && !defined(__OSF__)
#    define __OSF__
#  endif
#endif

#if !defined(SIGCHLD) && defined(SIGCLD)
# define SIGCHLD SIGCLD
#endif

static void _sigchld(int);

static list<FILE *> _log_channels;

int cheap_server_initialize(int port) {

  _log_channels.clear();
#if defined(SOCKETDEBUG) && defined(FOREGROUND)
  _log_channels.push_front(ferr);
#endif
  if(flog != NULL) _log_channels.push_front(flog);

#if !defined(FOREGROUND)

#ifdef SIGTTOU
  signal(SIGTTOU, SIG_IGN);
#endif
#ifdef SIGTTIN
  signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTSTP
  signal(SIGTSTP, SIG_IGN);
#endif
#ifdef SIGHUP
  signal(SIGHUP, SIG_IGN);
#endif

  int i;
  if((i = fork()) < 0) {
    fprintf(ferr,
            "server_initialize(): unable to fork(2) server [%d].\n",
            errno);
    fprintf(flog,
            "server_initialize(): "
            "unable to change process group [%d].\n",
            errno);
    fflush(flog)
    return -1;
  } /* if */
  else if(i > 0) {
    exit(0);
  } /* if */

#if defined(__SUNOS__)
  if(setpgrp(0, getpid()) == -1) {
    fprintf(ferr,
            "server_initialize(): "
            "unable to change process group [%d].\n",
            errno);
    fprintf(flog,
            "server_initialize(): "
            "unable to change process group [%d].\n", errno);
    fflush(flog);
    return -1;
  } /* if */
  if((i = open("/dev/tty", O_RDWR)) >= 0) {
    ioctl(i, TIOCNOTTY, (char *)NULL);
    close(i);
  } /* if */
#else
  if(setsid() == -1) {
    fprintf(ferr,
            "server_initialize(): "
            "unable to change process group [%d].\n", errno);
    fprintf(flog,
            "server_initialize(): "
            "unable to change process group [%d].\n", errno);
    fflush(flog);
    return -1;
  } /* if */
  if((i = fork()) < 0) {
    fprintf(ferr,
            "server_initialize(): unable to fork(2) server [%d].\n",
            errno);
    fprintf(flog,
            "server_initialize(): unable to fork(2) server [%d].\n",
            errno);
    fflush(flog);
    return -1;
  } /* if */
  else if(i > 0) {
    exit(0);
  } /* if */
#endif

  fclose(stdin);
  fclose(stdout);

  for(i = 0; i < NOFILE; i++) {
#if defined(SOCKETDEBUG)
    if(i == fileno(ferr)) {
      continue;
    } /* if */
#endif
    close(i);
  } /* for */
#endif /* !defined(FOREGROUND) */

#if !defined(__CYGWIN__)
  chdir("/tmp");
  umask(0);
#endif

#if !defined(NOFORK) && defined(SIGCHLD)
  signal(SIGCHLD, _sigchld);
#endif

  errno = 0;
  return 0;

} /* cheap_server_initialize() */

void cheap_server(int port) {

  int server, client;

  unsigned int n;
  struct sockaddr_in server_address, client_address;
  struct linger linger;
  struct hostent *host;

  if((server = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    throw error("unable to create server socket.");
  } /* if */

#ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);
#endif
#ifdef SIGHUP
  signal(SIGHUP, SIG_IGN);
#endif

  n = 1;
  setsockopt(server, SOL_SOCKET, SO_KEEPALIVE, (char *) &n, sizeof(n));
  setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *) &n, sizeof(n));
  linger.l_onoff = 1;
  linger.l_linger = 2;
  setsockopt(server,
             SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));

  bzero((char *)&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(port);
  if(bind(server,
          (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
    throw error("server(): unable to bind to server port.");
  } /* if */

  listen(server, SOMAXCONN);

#if defined(__CYGWIN__) || defined(__svr4__ )
#  define socklen_t int
#endif

  while(true) {
    n = sizeof(client_address);
    if((client = accept(server,
                        (struct sockaddr *)&client_address, 
                        (socklen_t *)&n)) < 0) {
      
      for(list<FILE *>::iterator log = _log_channels.begin();
          log != _log_channels.end();
          log++) {
        fprintf(*log,
                "[%d] server(): failed (invalid) accept(2) [%d].\n",
                getpid(), errno);
        fflush(*log);
      } /* for */
      continue;
    } /* if */

    for(list<FILE *>::iterator log = _log_channels.begin();
        log != _log_channels.end();
        log++) {
      if((host = gethostbyaddr((char *)&client_address.sin_addr.s_addr,
                               4, AF_INET)) != NULL
         && host->h_name != NULL) {
        fprintf(*log,
                "[%d] server(): connect from `%s' (%s).\n",
                getpid(), host->h_name, current_time());
      } /* if */
      else {
        char *address = inet_ntoa(client_address.sin_addr);
        fprintf(*log,
                "[%d] server(): connect from `%s' (%s) .\n",
                getpid(),
                (address != NULL ? address : "?.?.?.?"),
                current_time());
      } /* else */
      fflush(*log);
    } /* for */

    n = 1;
    setsockopt(client, SOL_SOCKET, SO_KEEPALIVE, (char *)&n, sizeof(n));
    setsockopt(client, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof(n));
    linger.l_onoff = 1;
    linger.l_linger = 2;
    setsockopt(client, SOL_SOCKET, SO_LINGER,
               (char *)&linger, sizeof(linger));

#if defined(NOFORK)
    if(!cheap_server_child(client)) {
      close(client);
      break;
    } /* if */
#else
    int child;
    if((child = fork()) < 0) {
      exit(-1);
    } /* if */
    else if(child > 0) {
      close(client);
    } /* else */
    else {
      int nitems = cheap_server_child(client);
      close(client);
      exit(nitems);
    } /* else */
#endif
    close(client);
  } /* while */

} /* cheap_server() */

int cheap_server_child(int socket) {

  int ntsdbitems = 1;
  static int size = 4096;
  static char *tsdbitem, *input = (char *)malloc(size);
  assert(input != NULL);
  FILE *stream = fdopen(socket, "r");
  assert(stream != NULL);

  char *yy_stream;
  FILE *yy_output = (FILE *)NULL;

  struct timeval tstart, tend; int treal = 0;

  socket_write(socket, "\f");

  bool errorp = false, kaerb = false;
  for(tsdbitem = (char *)NULL; 
      !feof(stream) && !kaerb; 
      ntsdbitems++, errorp = false) {

    fs_alloc_state FSAS;
    chart *Chart = 0;

    input_chart i_chart(New end_proximity_position_map);

    try {
      int status = socket_readline(socket, input, size);

      while(status > size) {
        status = size;
        size += size;
        input = (char *)realloc(input, size);
        assert(input != NULL);
        status += socket_readline(socket, &input[status], size - status);
      } /* if */

      if(status <= 0) {
        for(list<FILE *>::iterator log = _log_channels.begin();
            log != _log_channels.end();
            log++) {
          fprintf(*log,
                  "[%d] server_child(): client disconnect (%s).\n",
                  getpid(), current_time());
          fflush(*log);
        } /* for */
        break;
      } /* if */

      if(status == 1) {
        for(list<FILE *>::iterator log = _log_channels.begin();
            log != _log_channels.end();
            log++) {
          fprintf(*log,
                  "[%d] server_child(): shutdown on client request (%s).\n",
                  getpid(), current_time());
          fflush(*log);
        } /* for */
        fclose(stream);
        close(socket);
        return 0;
      } /* if */

      string foo = Conv->convert(ConvUTF8->convert(string(input)));

      for(list<FILE *>::iterator log = _log_channels.begin();
          log != _log_channels.end();
          log++) {
        fprintf(*log,
                "[%d] server_child(): got `%s'.\n",
                getpid(), foo.c_str());
        fflush(*log);
      } /* for */

      if(opt_yy
         && (yy_stream = getenv("CHEAP_YY_STREAM")) != NULL
         && (yy_output = fopen(yy_stream, "a")) != NULL) {
        fprintf(yy_output, "%s\f\n", foo.c_str());
      } /* if */

      gettimeofday(&tstart, NULL);

      tsdbitem = New char[strlen(input) + 1];
      assert(tsdbitem != NULL);
      strcpy(tsdbitem, input);

      analyze(i_chart, foo, Chart, FSAS, ntsdbitems);

      gettimeofday(&tend, NULL);
      treal = (tend.tv_sec - tstart.tv_sec) * 1000 
	+ (tend.tv_usec - tstart.tv_usec) / (MICROSECS_PER_SEC / 1000);
      
      for(list<FILE *>::iterator log = _log_channels.begin();
	  log != _log_channels.end();
	  log++) {
	fprintf(*log,
		"[%d] server_child(): "
		"(%d) [%d] --- %d (%.1f|%.1fs) <%d:%d> (%.1fK)\n",
		getpid(),
		stats.id, pedgelimit, stats.readings, 
		stats.first / 1000., stats.tcpu / 1000.,
		stats.words, stats.pedges, stats.dyn_bytes / 1024.0);
	fflush(*log);
      } /* for */
        
      struct MFILE *mstream = mopen();
      int nres = 1, skipped = 0;

      for(vector<item *>::iterator iter = Chart->Roots().begin();
          iter != Chart->Roots().end(); 
          ++iter)
	{
	  mflush(mstream);
	  int n = construct_k2y(nres++, *iter, false, mstream);
	  if(n >= 0)
	    {
	      if(skipped >= (opt_nth_meaning-1))
		{
		  socket_write(socket, mstring(mstream));
		  if((opt_nth_meaning-1) != 0 &&
		     skipped == (opt_nth_meaning-1))
		    break;
		}
	      else
		skipped++;
	    }
	}
      mclose(mstream);
    } /* try */
    
    catch(error_ressource_limit &e) {
      errorp = true;
      for(list<FILE *>::iterator log = _log_channels.begin();
          log != _log_channels.end();
          log++) {
        fprintf(*log,
                "[%d] server_child(): "
                "(%d) [%d] --- edge limit exceeded "
                "(%.1f|%.1fs) <%d:%d> (%.1fK)\n",
                getpid(),
                stats.id, pedgelimit, 
                stats.first / 1000., stats.tcpu / 1000.,
                stats.words, stats.pedges, stats.dyn_bytes / 1024.0);
        fflush(*log);
      } /* for */
#ifdef TSDBAPI
      if(opt_tsdb) 
        yy_tsdb_summarize_error(input, i_chart.max_position(), e);
#endif
    } /* catch */

    catch(error &e) {
      errorp = true;
      for(list<FILE *>::iterator log = _log_channels.begin();
          log != _log_channels.end();
          log++) {
        fprintf(*log, "[%d] server_child(): error: `", getpid());
        e.print(*log); fprintf(*log, "'.\n");
        fflush(*log);
      } /* for */

#ifdef TSDBAPI
      if(opt_tsdb) 
        yy_tsdb_summarize_error(input, i_chart.max_position(), e);
#endif
    } /* catch */

    socket_write(socket, "\f");

    input[0] = (char)0;

    int status = socket_readline(socket, input, size);
    while(status > size) {
      status = size;
      size += size;
      input = (char *)realloc(input, size);
      assert(input != NULL);
      status += socket_readline(socket, &input[status], size - status);
    } /* if */
    
    if(status <= 0) {
      for(list<FILE *>::iterator log = _log_channels.begin();
          log != _log_channels.end();
          log++) {
        fprintf(*log,
                "[%d] server_child(): {RT} client disconnect (%s).\n",
                getpid(), current_time());
        fflush(*log);
      } /* for */
      input[0] = (char)0;
      kaerb = true;
    } /* if */
    else {
      string foo = Conv->convert(ConvUTF8->convert(string(input)));

      if(opt_yy && yy_output != NULL) {
        fprintf(yy_output, "%s\f\n", foo.c_str());
        fclose(yy_output);
      } /* if */
      for(list<FILE *>::iterator log = _log_channels.begin();
          log != _log_channels.end();
          log++) {
        if(status == 1)
          fprintf(*log,
                  "[%d] server_child(): {RT} null role table.\n",
                  getpid());
        else
          fprintf(*log,
                  "[%d] server_child(): {RT} got `%s'.\n",
                  getpid(), foo.c_str());
        fflush(*log);
      } /* for */
#ifdef TSDBAPI
      if(!errorp && opt_tsdb && Chart != NULL) {
        yy_tsdb_summarize_item(*Chart, tsdbitem, i_chart.max_position(), 
                               treal, foo.c_str());
      } /* if */
#endif
    } /* else */

    if(!kaerb) socket_write(socket, "\f");

    if(tsdbitem != 0) delete[] tsdbitem; tsdbitem = 0;
    if(Chart != 0) delete Chart; Chart = 0;

  } /* for */

  fclose(stream);
  close(socket);
  return --ntsdbitems;

} /* cheap_server_child() */

int socket_write(int socket, char *string) {

  int written, left, n;

  for(written = 0, left = n = strlen(string);
      left > 0;
      left -= written, string += written) {
    if((written = write(socket, string, left)) == -1) {
      for(list<FILE *>::iterator log = _log_channels.begin();
          log != _log_channels.end();
          log++) {
        fprintf(*log,
                "[%d] socket_write(): write() error [%d].\n",
                getpid(), errno);
        fflush(*log);
      } /* for */
      return(-1);
    } /* if */
  } /* for */
  return(n - left);

} /* socket_write() */

int socket_readline(int socket, char *string, int length) {

  int i, n;
  char c;

  for(i = n = 0; n < length && (i = read(socket, &c, 1)) == 1;) {
    if(c == EOF) {
      return -1;
    } /* if */
    if(c == '\r') {
      (void)read(socket, &c, 1);
    } /* if */
    if(c == '\n') {
      if(!opt_yy) {
        string[n] = (char)0;
        return n + 1;
      } /* if */
    } /* if */
    else if(c == '\f') {
      if(opt_yy) {
        string[n] = (char)0;
        return n + 1;
      } /* if */
    } /* if */
    else {
      string[n++] = c;
    } /* else */
  } /* for */
  if(i == -1) {
    return -1;
  } /* if */
  else if(!n && !i) {
    return 0;
  } /* if */
  if(n < length) {
    string[n] = 0;
  } /* if */
  return n + 1;
  
} /* socket_readline() */

static void _sigchld(int foo) {

  int status, pid;

#if defined(SIGCHLD)
  signal(SIGCHLD, _sigchld);
#endif

  while((pid = wait3(&status, WNOHANG, (struct rusage *)NULL)) > 0) {
    for(list<FILE *>::iterator log = _log_channels.begin();
        log != _log_channels.end();
        log++) {
      if(WIFEXITED(status))
        fprintf(*log, 
                "[%d] sigchld(): relieved child # %d (exit: %d).\n", 
                getpid(), pid, WEXITSTATUS(status));
      else if(WIFSIGNALED(status))
        fprintf(*log, 
                "[%d] sigchld(): relieved child # %d (signal: %d).\n", 
                getpid(), pid, WTERMSIG(status));
      else
        fprintf(*log, 
                "[%d] sigchld(): relieved child # %d (mysterious exit).\n", 
                getpid(), pid);
      fflush(*log);
    } /* for */
  } /* while */
} /* _sigchld() */

#ifdef TSDBAPI
int yy_tsdb_summarize_item(chart &Chart, const char *item,
			   int length, int treal, const char *rt) {

  if(!client_open_item_summary()) {
    capi_printf("(:run . (");
    cheap_tsdb_summarize_run();
    capi_printf("))\n");

    capi_printf("(:i-length . %d) (:i-input . \"%s\")", length,
                escape_string(item == 0 ? "" : item).c_str());

    tsdb_parse T;
    cheap_tsdb_summarize_item(Chart, length, treal, 0, rt, T);
    T.capi_print();
    
    return client_send_item_summary();
  } /* if */

  for(list<FILE *>::iterator log = _log_channels.begin();
      log != _log_channels.end();
      log++) {
    fprintf(*log,
            "[%d] yy_tsdb_summarize_item(): "
            "unable to locate [incr tsdb()] server.\n",
            getpid());
    fflush(*log);
  } /* for */
  return -1;

} /* yy_tsdb_summarize_item() */

int yy_tsdb_summarize_error(const char *item, int length, error &condition) {

  if(!client_open_item_summary()) {
    capi_printf("(:i-length . %d) (:i-input . \"%s\")", length,
                escape_string(item == 0 ? "" : item).c_str());

    tsdb_parse T;
    cheap_tsdb_summarize_error(condition, 0, T);
    T.capi_print();

    return client_send_item_summary();
  } /* if */
  for(list<FILE *>::iterator log = _log_channels.begin();
      log != _log_channels.end();
      log++) {
    fprintf(*log,
            "[%d] yy_tsdb_summarize_error(): "
            "unable to locate [incr tsdb()] server.\n",
            getpid());
    fflush(*log);
  } /* for */
  return -1;

} /* yy_tsdb_summarize_error() */
#endif
#endif

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
#include "errors.h"
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

string _yyfilteredinput;

void yy_tokenizer::add_tokens(input_chart *i_chart)
{
  int start, end;
  string word, surface;
  set<string> tags;

  if(verbosity > 4)
    fprintf(ferr, "yy_tokenizer:");

  FILE *token_output = 0;
  if(opt_yy)
    {
      char *token_stream = getenv("CHEAP_TOKEN_STREAM");
      if(token_stream)
	token_output = fopen(token_stream, "a");
    }

  char *save; _yyfilteredinput = string();
  while(save = _yyinput, read_token(start, end, word, surface, tags))
    {
      if(start < 0 || end <= start || (start % 100) != 0 || (end % 100) != 0)
	{
	  ostrstream msg;
	  msg << "tokenizer(): illegal token span ("
	      << start << " - " << end << ")";
	  throw error(msg.str());
	}

      start /= 100; end /= 100;
      postags poss(tags);

      if(verbosity > 4)
	{
	  fprintf(ferr, " [%d - %d] <%s> {", start, end, word.c_str());
	  poss.print(ferr);
	  fprintf(ferr, "}");
	}

      // log token
      if(token_output)
	{
	  fprintf(token_output, "%s\t\t", surface.c_str());
	  poss.print(token_output);
	  fprintf(token_output, "\n");
	}

      // skip empty tokens and punctuation
      if(word.empty() || Grammar->punctuationp(word))
	{
	  _yyfilteredinput += string(save, _yyinput - save);
	  continue;
	}

      if(i_chart->contains(start, end, word, poss))
	{
	  if(verbosity > 4)
	    fprintf(ferr, " - duplicate");

	  continue;
	}

      _yyfilteredinput += string(save, _yyinput - save);
      list<full_form> forms = Grammar->lookup_form(word);

      if(forms.empty())
	{
	  i_chart->add_token(start, end, full_form(), word, 0, poss);
	}
      else
	{
	  for(list<full_form>::iterator currf = forms.begin();
	      currf != forms.end(); ++currf)
	    {
	      i_chart->add_token(start, end, *currf, word, currf->priority(),
				 poss);
	    }
	}
    }

  if(token_output)
    {
      fprintf(token_output, "\f\n");
      fclose(token_output);
    }

  if(verbosity > 4)
    {
      fprintf(ferr, "\n");
      fprintf(ferr, "Filtered input: <%s>\n", _yyfilteredinput.c_str());
    }
}

bool yy_tokenizer::read_int(int &target)
{
  int n;

  if(_yyinput == 0 || _yyinput[0] == '\0')
    {
      target = -1;
      return false;
    }

  for(; _yyinput[0] && !isdigit(_yyinput[0]); _yyinput++) ;

  for(target = n = 0; _yyinput[0] && isdigit(_yyinput[0]); _yyinput++, n++)
    {
      target = _yyinput[0] - '0' + (10 * target);
    }

  if(!n)
    {
      target = -1;
      return false;
    }
 
 return true;
}

bool yy_tokenizer::read_string(string &target, string &surface)
{
  target = "";
  surface = "";

  if(_yyinput == 0 || _yyinput[0] == '\0')
    return false;

  for(; _yyinput[0] && _yyinput[0] != '"'; _yyinput++);
  if(_yyinput[0] != '"') return false;

  //
  // skip leading `#' sign in concept names, though not as an isolated token
  //
  if(_yyinput[1] == '#' && _yyinput[2] != '"') _yyinput++;

  bool downcasep = true;
  char last;
  for(_yyinput++, last = 0; 
      _yyinput[0] && (_yyinput[0] != '"' || last == '\\');
      last = (last == '\\' ? 0 : _yyinput[0]), _yyinput++)
    {
      if(_yyinput[0] != '\\' || last == '\\') {
        target += (downcasep ? tolower(_yyinput[0]) : _yyinput[0]);
        surface += _yyinput[0];
      } /* if */
    }

  if(_yyinput[0] == '"')
    {
      _yyinput++;
      return true;
    }

  target = "";
  surface = "";
  return false;

}

bool yy_tokenizer::read_comma()
{
  if(_yyinput == 0 || _yyinput[0] == '\0')
    return false;

  for(; _yyinput[0] && isspace(_yyinput[0]); _yyinput++) ;
  if(_yyinput[0] != ',') return false;
  _yyinput++;
  return true;
}

bool yy_tokenizer::read_pos(string &target)
{
  target = "";

  if(_yyinput == 0 || _yyinput[0] == '\0')
    return false;

  for(; _yyinput[0] && isspace(_yyinput[0]); _yyinput++) ;
  if(!is_idchar(_yyinput[0])) return false;

  while(is_idchar(_yyinput[0]))
    {
      target += _yyinput[0];
      _yyinput++;
    }

  if(isspace(_yyinput[0]))
    {
      _yyinput++;
      return true;
    }
  else if(_yyinput[0] == ')' || _yyinput[0] == ',')
    {
      return true;
    }

  target = "";
  return false;

}

bool yy_tokenizer::read_token(int &start, int &end,
                              string &word, string &surface, set<string> &tags)
{
  if(_yyinput == 0 || _yyinput[0] == '\0') return false;

  for(; _yyinput[0] && _yyinput[0] != '('; _yyinput++);

  if(_yyinput[0]) _yyinput++;

  tags.clear();
  if(read_int(start) && read_int(end) && read_string(word, surface))
    {
      // read optional POS tags
      if(read_comma())
	{
	  string tag;
	  while(read_pos(tag))
	    tags.insert(tag);
	}
      
      for(; _yyinput[0] != '\0' && _yyinput[0] != ')'; _yyinput++);
      return (*_yyinput++ == ')');
    }

  return false;
}

// library based interface to language server

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

#ifdef __BORLANDC__
extern unsigned long int Mem_CurrUser;
#endif

extern typecache glbcache;
extern int total_cached_constraints;

#ifdef TSDBFILEAPI
tsdb_parse *TsdbParse = 0;
#endif

// parse one input item; return string of K2Y's
string l2_parser_parse(const string& input, int nskip)
{
  static int item_id = 0;

  fs_alloc_state FSAS;

  fprintf(fstatus, "[%s] (%d) l2_parser_parse(\"%s\", %d)\n", current_time(),
          stats.id, input.c_str(), nskip);

  fprintf(fstatus, " Permanent dag storage: %d*%dk\n Dynamic dag storage: %d*%dk\n GLB cache: %d*%dk, Constraint cache: %d entries\n",
          p_alloc.nchunks(), p_alloc.chunksize() / 1024,
          t_alloc.nchunks(), t_alloc.chunksize() / 1024,
          glbcache.alloc().nchunks(), glbcache.alloc().chunksize() / 1024,
          total_cached_constraints);

#ifdef __BORLANDC__
  fprintf(fstatus, " LanguageServer storage: %lu\n", Mem_CurrUser);
#endif

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
    for(list<item *>::iterator iter = Chart->Roots().begin();
        iter != Chart->Roots().end(); ++iter)
    {
      mflush(mstream);
      int n = construct_k2y(nres++, *iter, false, mstream);
      if(n >= 0)
      {
        if(skipped >= nskip)
        {
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

    throw l2_error(e.msg());
  }

  delete Chart;

  if(verbosity > 4)
    fprintf(fstatus, " result: %s\n", result.c_str());

  fflush(fstatus);

  return result;
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

vector<l2_morph_analysis> l2_morph_analyse(const string& form)
{
  vector<l2_morph_analysis> results;
  
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
        a.forms.push_back(*f);
      for(list<type_t>::iterator r = it->rules().begin(); 
          r != it->rules().end(); 
          ++r)
        a.rules.push_back(printnames[*r]);
      
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

// free resources of parser
void l2_parser_exit()
{
  fprintf(fstatus, "[%s] l2_parser_exit()\n", current_time());

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

      for(list<FILE *>::iterator log = _log_channels.begin();
          log != _log_channels.end();
          log++) {
        fprintf(*log,
                "[%d] server_child(): got `%s'.\n",
                getpid(), input);
        fflush(*log);
      } /* for */

      if(opt_yy
         && (yy_stream = getenv("CHEAP_YY_STREAM")) != NULL
         && (yy_output = fopen(yy_stream, "a")) != NULL) {
        fprintf(yy_output, "%s\f\n", input);
      } /* if */

      gettimeofday(&tstart, NULL);

      tsdbitem = New char[strlen(input) + 1];
      assert(tsdbitem != NULL);
      strcpy(tsdbitem, input);
      _yyfilteredinput = string(tsdbitem);

      analyze(i_chart, input, Chart, FSAS, ntsdbitems);

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

      for(list<item *>::iterator iter = Chart->Roots().begin(); iter != Chart->Roots().end(); ++iter)
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
        yy_tsdb_summarize_error(_yyfilteredinput.c_str(), 
                                i_chart.max_position(), e);
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
        yy_tsdb_summarize_error(_yyfilteredinput.c_str(), 
                                i_chart.max_position(), e);
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
      if(opt_yy && yy_output != NULL) {
        fprintf(yy_output, "%s\f\n", input);
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
                  getpid(), input);
        fflush(*log);
      } /* for */

    } /* else */

#ifdef TSDBAPI
    if(!errorp && opt_tsdb && Chart != NULL) {
      yy_tsdb_summarize_item(*Chart, _yyfilteredinput.c_str(), 
                             i_chart.max_position(), treal, input);
    } /* if */
#endif
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
			   int length, int treal, char *rt) {

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

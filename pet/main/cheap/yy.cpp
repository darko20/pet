//
// The entire contents of this file, in printed or electronic form, are
// (c) Copyright YY Software Corporation, La Honda, CA 2000, 2001.
// Unpublished work -- all rights reserved -- proprietary, trade secret
//

// YY input format and server mode (oe, uc)

#include <stdlib.h>          
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

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

#include <string>
using namespace std;

extern "C" {
#  include "mfile.h"
}

#include "../common/utility.h"
#include "errors.h"
#include "tokenlist.h"
#include "parse.h"
#include "cheap.h"
#include "tsdb++.h"
#include "k2y.h"
#include "yy.h"

yy_tokenlist::yy_tokenlist(const char *in)
{
  int start, end;
  string word, surface;
  set<string> tags;
  
  _input = (char *)in;

  _ntokens = 0; _tokens.clear(); _surface.clear();
  while(read_token(start, end, word, surface, tags))
    {
      if(start < 0 || end != start + 100)
	{
	  _ntokens = 0;
	  throw error("tokenlist(): unexpected multi-word token.");
	}
      unsigned int pos = start / 100;
      if(pos >= _tokens.size()) _tokens.resize(pos + 1);
      _tokens[pos] = word;
      if(pos >= _surface.size()) _surface.resize(pos + 1);
      _surface[pos] = surface;
      if(pos >= _postags.size()) _postags.resize(pos + 1);
      _postags[pos] = tags;
      _ntokens++;
    } /* while */

  char *token_stream;
  FILE *token_output;

  if(opt_yy
     && (token_stream = getenv("CHEAP_TOKEN_STREAM")) != NULL
     && (token_output = fopen(token_stream, "a")) != NULL) {
    for(int i = 0; i < _ntokens; i++) {
      fprintf(token_output, "%s\t\t", _surface[i].c_str());
      postags poss = postags(_postags[i]);
      poss.print(token_output);
      fprintf(token_output, "\n");
    } /* for */
    fprintf(token_output, "\f\n");
    fclose(token_output);
  } /* if */

}

bool yy_tokenlist::read_int(int &target)
{
  int n;

  if(_input == 0 || _input[0] == '\0')
    {
      target = -1;
      return false;
    }
  
  for(; _input[0] && !isdigit(_input[0]); _input++) ;

  for(target = n = 0; _input[0] && isdigit(_input[0]); _input++, n++)
    {
      target = _input[0] - '0' + (10 * target);
    }

  if(!n)
    {
      target = -1;
      return false;
    }
 
 return true;
}

bool yy_tokenlist::read_string(string &target, string &surface)
{
  target = "";
  surface = "";

  if(_input == 0 || _input[0] == '\0')
    return false;

  for(; _input[0] && _input[0] != '"'; _input++);
  if(_input[0] != '"') return false;

  //
  // skip trailing `#' sign in concept names, though not as an isolated token
  //
  if(_input[1] == '#' && _input[2] != '"') _input++;

  char last;
  for(_input++, last = 0; 
      _input[0] && (_input[0] != '"' || last == '\\');
      last = (last == '\\' ? 0 : _input[0]), _input++)
    {
      if(_input[0] != '\\' || last == '\\') {
        target += tolower(_input[0]);
        surface += _input[0];
      } /* if */
    }

  if(_input[0] == '"')
    {
      _input++;
      return true;
    }

  target = "";
  surface = "";
  return false;

}

bool yy_tokenlist::read_comma()
{
  if(_input == 0 || _input[0] == '\0')
    return false;

  for(; _input[0] && isspace(_input[0]); _input++) ;
  if(_input[0] != ',') return false;
  _input++;
  return true;
}

bool yy_tokenlist::read_pos(string &target)
{
  target = "";

  if(_input == 0 || _input[0] == '\0')
    return false;

  for(; _input[0] && isspace(_input[0]); _input++) ;
  if(!isalnum(_input[0])) return false;

  while(isalnum(_input[0]))
    {
      target += _input[0];
      _input++;
    }

  if(isspace(_input[0]))
    {
      _input++;
      return true;
    }
  else if(_input[0] == ')' || _input[0] == ',')
    {
      return true;
    }

  target = "";
  return false;

}

bool yy_tokenlist::read_token(int &start, int &end,
                              string &word, string &surface, set<string> &tags)
{
  if(_input == 0 || _input[0] == '\0') return false;

  for(; _input[0] && _input[0] != '('; _input++);

  if(_input[0]) _input++;

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
      
      for(; _input[0] != '\0' && _input[0] != ')'; _input++);
      return (*_input++ == ')');
    }

  return false;
}

postags yy_tokenlist::POS(int i)
{
  return postags(_postags[i]);
}

// library based interface to language server

// initialize parser with specified grammar
int l2_parser_init(string grammar_path)
{
  fstatus = fopen("l2dbg.log", "w");
  if(fstatus == 0)
    { // xx think about error reporting
      fstatus = stderr;
    }
  ferr = fstatus;

  fprintf(fstatus, "[%s] l2_parser_init(\"%s\")\n", current_time(), grammar_path.c_str());

  cheap_settings = new settings(settings::basename(grammar_path.c_str()), grammar_path.c_str(), "reading");
  fprintf(fstatus, "\n");

  options_from_settings(cheap_settings);

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

  if(opt_one_meaning == false)
    {
      fprintf(fstatus, "you want only one meaning - turning on opt_one_meaning\n");
      opt_one_meaning = true;
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
  try { Grammar = new grammar(grammar_path.c_str()); }

  catch(error &e)
    {
      fprintf(fstatus, "- aborted\n");
      e.print(ferr);
      return 1;
    }

  fprintf(fstatus, "- %d types in %0.2g s\n",
	  ntypes, t_start.convert2ms(t_start.elapsed()) / 1000.);

  initialize_version();

  fflush(fstatus);

  return 0;
}

#ifdef __BORLANDC__
extern unsigned long int Mem_CurrUser;
#endif

// parse one input item; return string of K2Y's
string l2_parser_parse(string input)
{
  static int item_id = 0;

  fprintf(fstatus, "[%s] l2_parser_parse(\"%s\")\n", current_time(),
          input.c_str());

#ifdef __BORLANDC__
  fprintf(fstatus, " LanguageServer allocated: %lu\n", Mem_CurrUser);
#endif

  fprintf(fstatus, " L2 allocated: %d*%dk permanent, %d*%dk dynamic\n",
	  p_alloc.nchunks(), p_alloc.chunksize(), 
	  t_alloc.nchunks(), t_alloc.chunksize());

  tokenlist *Input = 0;
  chart *Chart = 0;
  string result;

  try
    {
      item_id++;

      Input = new yy_tokenlist(input.c_str());

      if(Input == 0 || Input->length() <= 0)
	{
          throw error("malformed input");
	}

      Chart = new chart(Input->length());
      parse(*Chart, Input, item_id);

      fprintf(fstatus,"(%d) [%d] --- %d (%.1f|%.1fs) <%d:%d> (%.1fK)\n",
	      stats.id, pedgelimit, stats.readings,
	      stats.first / 1000., stats.tcpu / 1000.,
	      stats.words, stats.pedges, stats.dyn_bytes / 1024.0);
      if(verbosity > 1) stats.print(fstatus);
      fflush(fstatus);

      struct MFILE *mstream = mopen();
      int nres = 1;
      for(chart_iter it(*Chart); it.valid(); it++) {
	if(it.current()->result_root()) {
	  mflush(mstream);
	  int n = construct_k2y(nres++, it.current(), false, mstream);
	  if(n >= 0) {
	    result += string(mstring(mstream));
	    if(opt_one_meaning) break;
	  } /* if */
	} /* if */
      } /* for */
      mclose(mstream);
    } /* try */

  catch(error_ressource_limit &e)
    {
      fprintf(fstatus, "(%d) [%d] --- edge limit exceeded "
	      "(%.1f|%.1fs) <%d:%d> (%.1fK)\n",
	      stats.id, pedgelimit,
	      stats.first / 1000., stats.tcpu / 1000.,
	      stats.words, stats.pedges, stats.dyn_bytes / 1024.0);
      fflush(fstatus);
    }

    catch(error &e)
      {
	fprintf(fstatus, "error: `");
	e.print(fstatus); fprintf(fstatus, "'.\n");
        fflush(fstatus);
      }

  if(verbosity > 4)
    fprintf(fstatus, "result: %s\n", result.c_str());

  if(Chart != 0) delete Chart;
  if(Input != 0) delete Input;

  fflush(fstatus);

  return result;
}

// free resources of parser
void l2_parser_exit()
{
  fprintf(fstatus, "[%s] l2_parser_exit()\n", current_time());

  delete Grammar;
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

  int nitems = 1;
  static int size = 4096;
  static char *item, *input = (char *)malloc(size);
  assert(input != NULL);
  FILE *stream = fdopen(socket, "r");
  assert(stream != NULL);

  char *yy_stream;
  FILE *yy_output = (FILE *)NULL;

  struct timeval tstart, tend; int treal = 0;

  tokenlist *Input = (tokenlist *)NULL;
  chart *Chart = (chart *)NULL;

  socket_write(socket, "\f");

  bool errorp = false, kaerb = false;
  for(item = (char *)NULL, Chart = (chart *)NULL, Input = (tokenlist *)NULL; 
      !feof(stream) && !kaerb; 
      nitems++, errorp = false) {
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

      item = new char[strlen(input) + 1];
      assert(item != NULL);
      strcpy(item, input);

      if(opt_yy)
	Input = new yy_tokenlist(input);
      else
	Input = new lingo_tokenlist(input);
      
      if(Input == 0 || Input->length() <= 0) {
        for(list<FILE *>::iterator log = _log_channels.begin();
            log != _log_channels.end();
            log++) {
          fprintf(*log,
                  "[%d] server_child(): ignoring malformed input.\n",
                  getpid());
          fflush(*log);
        } /* for */
      } /* if */
      else {
        
        Chart = new chart(Input->length());
        parse(*Chart, Input, nitems);

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
        int nres = 1;
        for(chart_iter it(*Chart); it.valid(); it++) {
          if(it.current()->result_root()) {
            mflush(mstream);
            int n = construct_k2y(nres++, it.current(), false, mstream);
            if(n >= 0) {
              socket_write(socket, mstring(mstream));
              if(opt_one_meaning) break;
            } /* if */
          } /* if */
        } /* for */
        mclose(mstream);
      } /* if */
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
      if(opt_tsdb) yy_tsdb_summarize_error(item, Input->length(), e);
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
      if(opt_tsdb) yy_tsdb_summarize_error(item, Input->length(), e);
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

#ifdef TSDBAPI
      if(!errorp && opt_tsdb && Chart != NULL) {
        yy_tsdb_summarize_item(*Chart, item, Input->length(), treal, input);
      } /* if */
#endif
      socket_write(socket, "\f");

    } /* else */

    if(item != NULL) delete[] item; item = (char *)NULL;
    if(Input != NULL) delete Input;
    Input = (tokenlist *)NULL;
    if(Chart != NULL) delete Chart;
    Chart = (chart *)NULL;
    
  } /* for */

  fclose(stream);
  close(socket);
  return --nitems;

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
int yy_tsdb_summarize_item(chart &Chart, char *item, int length,
                           int treal, char *rt) {

  if(!client_open_item_summary()) {
    capi_printf("(:run . (");
    cheap_tsdb_summarize_run();
    capi_printf("))\n");
    capi_printf("(:i-length . %d) (:i-input . \"", length);
    for(char *foo = item; *foo; foo++) {
      if(*foo == '"' || *foo == '\\') capi_putc('\\');
      capi_putc(*foo);
    } /* for */
    capi_printf("\")");
    cheap_tsdb_summarize_item(Chart, length, treal, 0, rt);
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

int yy_tsdb_summarize_error(char *item, int length, error &condition) {

  if(!client_open_item_summary()) {
    capi_printf("(:i-length . %d) (:i-input . \"", length);
    if(item != NULL) {
      for(char *foo = item; *foo; foo++) {
        if(*foo == '"' || *foo == '\\') capi_putc('\\');
        capi_putc(*foo);
      } /* for */
    } /* if */
    capi_printf("\")");
    cheap_tsdb_summarize_error(condition);
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




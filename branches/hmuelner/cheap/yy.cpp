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

// YY input format and server mode (oe, uc)

#include "pet-config.h"

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
#include <strings.h>
#ifdef __SUNOS__
#include <sys/ioctl.h>
#endif
#endif

#include "mfile.h"
#include "utility.h"
#include "errors.h"
#include "lex-tdl.h"
#include "parse.h"
#include "chart.h"
#include "cheap.h"
#include "typecache.h"
#include "tsdb++.h"
#include "yy.h"
#include "configs.h"
#ifdef HAVE_ICU
#include "unicode.h"
#define massageUTF8(str) Conv->convert(ConvUTF8->convert(str))
#define massagetoUTF8(str) ConvUTF8->convert(Conv->convert(str))
#else
#define massageUTF8(str) str
#define massagetoUTF8(str) str
#endif

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
  _log_channels.push_front(cerr);
#endif

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
    LOG(logAppl, ERROR,
        "server_initialize(): unable to fork(2) server [" << errno << "].");
    LOG(logServer, ERROR,
        "server_initialize(): unable to change process group ["
        << errno << "].");
    return -1;
  } /* if */
  else if(i > 0) {
    exit(0);
  } /* if */

#if defined(__SUNOS__)
  if(setpgrp(0, getpid()) == -1) {
    LOG(logAppl, ERROR, "server_initialize(): "
        "unable to change process group [" << errno << "].");
    LOG(logServer, ERROR, "server_initialize(): "
        "unable to change process group [" << errno << "].");
    return -1;
  } /* if */
  if((i = open("/dev/tty", O_RDWR)) >= 0) {
    ioctl(i, TIOCNOTTY, (char *)NULL);
    close(i);
  } /* if */
#else
  if(setsid() == -1) {
    LOG(logAppl, ERROR, "server_initialize(): "
        "unable to change process group [" << errno << "].", errno);
    LOG(logServer, ERROR, "server_initialize(): "
        "unable to change process group [" << errno << "].", errno);
    return -1;
  } /* if */
  if((i = fork()) < 0) {
    LOG(logAppl, ERROR,
        "server_initialize(): unable to fork(2) server [" << errno << "].");
    LOG(logServer, ERROR,
        "server_initialize(): unable to fork(2) server [" << errno << "].");
    return -1;
  } /* if */
  else if(i > 0) {
    exit(0);
  } /* if */
#endif

  fclose(stdin);
  fclose(stdout);

  for(i = 0; i < NOFILE; i++) {
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
    throw tError("unable to create server socket.");
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
    throw tError("server(): unable to bind to server port.");
  } /* if */

  listen(server, SOMAXCONN);

#if defined(__CYGWIN__)
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

      string foo = massageUTF8(string(input));

      for(list<FILE *>::iterator log = _log_channels.begin();
          log != _log_channels.end();
          log++) {
        fprintf(*log,
                "[%d] server_child(): got `%s'.\n",
                getpid(), foo.c_str());
        fflush(*log);
      } /* for */

      if(get_opt_bool("opt_yy")
         && (yy_stream = getenv("CHEAP_YY_STREAM")) != NULL
         && (yy_output = fopen(yy_stream, "a")) != NULL) {
        fprintf(yy_output, "%s\f\n", foo.c_str());
      } /* if */

      gettimeofday(&tstart, NULL);

      tsdbitem = new char[strlen(input) + 1];
      assert(tsdbitem != NULL);
      strcpy(tsdbitem, input);

      list<tError> errors;
      analyze(foo, Chart, FSAS, errors, ntsdbitems);
      if(!errors.empty())
          throw errors.front();
                
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
                stats.id, get_opt_int("opt_pedgelimit"), stats.readings, 
                stats.first / 1000., stats.tcpu / 1000.,
                stats.words, stats.pedges, stats.dyn_bytes / 1024.0);
        fflush(*log);
      } /* for */

#if 0
      struct MFILE *mstream = mopen();
      int nres = 1, skipped = 0;

      for(vector<item *>::iterator iter = Chart->readings().begin();
          iter != Chart->readings().end(); 
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
#endif
    } /* try */

    catch(tError &e) {
      errorp = true;
      for(list<FILE *>::iterator log = _log_channels.begin();
          log != _log_channels.end();
          log++) {
          fprintf(*log, "[%d] server_child(): (%d) [%d] --- error `%s'",
                  stats.id, get_opt_int("opt_pedgelimit"),
                  getpid(), e.getMessage().c_str());
          fprintf(*log, " (%.1f|%.1fs) <%d:%d> (%.1fK)\n",
                  stats.first / 1000., stats.tcpu / 1000.,
                  stats.words, stats.pedges, stats.dyn_bytes / 1024.0);
          fflush(*log);
      } /* for */

#ifdef TSDBAPI
      if(get_opt_int("opt_tsdb")) 
        yy_tsdb_summarize_error(input, Chart->rightmost(), e);
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
      string foo = massageUTF8(string(input));

      if(get_opt_bool("opt_yy") && yy_output != NULL) {
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
      if(!errorp && get_opt_int("opt_tsdb") && Chart != NULL) {
        yy_tsdb_summarize_item(*Chart, tsdbitem, Chart->rightmost(), 
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
      if(!get_opt_bool("opt_yy")) {
        string[n] = (char)0;
        return n + 1;
      } /* if */
    } /* if */
    else if(c == '\f') {
      if(get_opt_bool("opt_yy")) {
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
    cheap_tsdb_summarize_item(Chart, length, treal, 0, T);
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

int yy_tsdb_summarize_error(const char *item, int length, tError &condition) {

  if(!client_open_item_summary()) {
    capi_printf("(:i-length . %d) (:i-input . \"%s\")", length,
                escape_string(item == 0 ? "" : item).c_str());

    tsdb_parse T;
    list<tError> errors;
    errors.push_back(condition);
    cheap_tsdb_summarize_error(errors, 0, T);
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

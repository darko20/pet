/* -*- Mode: C++ -*- */

#ifndef _SESSIONMANAGER_H
#define _SESSIONMANAGER_H

#include "fs.h"
#include <list>

#define NO_SUCH_SESSION -2
#define NO_ERRORS 0
#define ERRORS_PRESENT -1
#define RUNTIME_ERROR -3

class SessionManager {
private:
  class Session {
  public:
    static int next_id;

    int id;
    struct chart *chart;
    fs_alloc_state FSAS;
    std::list<tError> errors;
    std::string input;

    Session(const std::string &in) : id(++next_id), chart(NULL), input(in) { }

    ~Session();
  };

  std::list<Session *> _sessions;

  /** return a new session, or NULL, if there are no more sessions available */
  Session *new_session(const std::string &input);

  /** return the session with the given \c session_id, or NULL, if there is no
   *  such session.
   */
  Session *find_session(int session_id);

  /** remove the session with id \c session_id.
   *  \return \c true if the session exists and was deleted, \c false otherwise
   */
  bool remove_session(int session_id);

  /** the singleton instance */
  static SessionManager *_instance;

  /** private constructor: to be used is a singleton */
  SessionManager() { }

public:

  static SessionManager &getManager() {
    if (_instance == NULL) {
      _instance = new SessionManager();
    }
    return *_instance;
  }

  ~SessionManager();

  int start_parse(const std::string &input);

  /** Run the parser and collect the results according to the specified options
   *  \todo it would be nice if it would be possible to run the parser up to
   *        the first result, restart it, etc., maybe even add new input, or
   *        activate other robustness processing in case it can find no result
   */
  int run_parser(int session_id);

  /** Return the number of results found */
  int results(int session_id);

  /** Get result item number \c result_no of session \c session_id
   *  \return NULL if either the session id is not valid or there is no such
   *          result
   */
  class tItem * get_result_item(int session_id, size_t no);

  /** Get result number \c result_no of session \c session_id in the specified
   *  \c format, which is one of:
   *  "fs-readable", "fs-compact", "fs-jxchg"
   *  "derivation-default", "derivation-tsdb", "derivaton-xmllabel"
   *  "mrs-new", "mrs-simple"
   */
  std::string get_result(int session_id, size_t result_no, std::string format);

  /** Get the number of errors of session \c session_id */
  int errors(int session_id);

  /** Get error number \c error_no of session \c session_id */
  std::string get_error(int session_id, size_t error_no);

  /** close this session. */
  int end_parse(int session_id) {
    return (remove_session(session_id) ? NO_ERRORS : NO_SUCH_SESSION);
  }
};


#endif

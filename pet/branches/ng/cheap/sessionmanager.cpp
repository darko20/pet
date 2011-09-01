

#include "sessionmanager.h"
#include "parse.h"
#include "chart.h"
#include "logging.h"

#include <vector>

using namespace std;

extern void print_result_as(string format, tItem *reading, ostream &out);

int SessionManager::Session::next_id = 0;
SessionManager *SessionManager::_instance = NULL;

typedef list<SessionManager::Session *>::iterator session_it;

SessionManager::Session::~Session() {
  delete chart;
}

SessionManager::Session *SessionManager::new_session(const string &input) {
  if (_sessions.empty()) {
    Session* new_session = new Session(input);
    _sessions.push_back(new_session);
    return new_session;
  }
  return NULL;
}

SessionManager::Session *SessionManager::find_session(int session_id) {
  for (session_it it = _sessions.begin(); it != _sessions.end(); ++it) {
    if ((*it)->id == session_id) {
      return *it;
    }
  }
  return NULL;
}

bool SessionManager::remove_session(int session_id) {
  for (session_it it = _sessions.begin(); it != _sessions.end(); ++it) {
    if ((*it)->id == session_id) {
      delete *it;
      _sessions.erase(it);
      return true;
    }
  }
  return false;
}

SessionManager::~SessionManager() {
  for (session_it it = _sessions.begin(); it != _sessions.end(); ++it) {
    delete *it;
  }
  _sessions.clear();
}

int SessionManager::start_parse(const string &input) {
  if (_sessions.empty()) {
    Session* new_session = new Session(input);
    _sessions.push_back(new_session);
    return new_session->id;
  }
  return NO_SUCH_SESSION;
}

/** Run the parser and collect the results according to the specified options
 *  \todo it would be nice if it would be possible to run the parser up to
 *        the first result, restart it, etc., maybe even add new input, or
 *        activate other robustness processing in case it can find no result
 */
int SessionManager::run_parser(int session_id) {
  Session *curr = find_session(session_id);
  if (curr == NULL) return NO_SUCH_SESSION;
  try {
    analyze(curr->input, curr->chart, curr->FSAS, curr->errors, curr->id);
  }
  catch(tError err) {
    //LOG(logAppl, ERROR, err.getMessage());
    curr->errors.push_back(err.getMessage());
    if (err.severe()) return RUNTIME_ERROR;
  }
  // check errors caught elsewhere
  for (list<tError>::iterator it = curr->errors.begin();
       it != curr->errors.end();
       ++it) {
    if ((*it).severe()) return RUNTIME_ERROR;
  }
  return (curr->errors.empty()) ? NO_ERRORS : ERRORS_PRESENT;
}

/** Return the number of results found */
int SessionManager::results(int session_id) {
  Session *curr = find_session(session_id);
  if (curr == NULL) return NO_SUCH_SESSION;
  return curr->chart->readings().size();
}

/** return specified result item, or NULL, if illegal */
class tItem * SessionManager::get_result_item(int session_id, size_t no) {
  Session *curr = find_session(session_id);
  if (curr != NULL && no < curr->chart->readings().size()) {
    return curr->chart->readings()[no];
  }
  return NULL;
}


/** Get result number \c result_no of session \c session_id in the specified
 *  \c format, which is one of:
 *  "fs-readable", "fs-compact", "fs-jxchg"
 *  "derivation-default", "derivation-tsdb", "derivaton-xmllabel"
 *  "mrs-new", "mrs-simple"
 */
string SessionManager::get_result(int session_id, size_t no, string format) {
  tItem *result = get_result_item(session_id, no);
  if (result != NULL) {
    ostringstream out;
    print_result_as(format, result, out);
    return out.str();
  }
  return string();
}

/** Get the number of errors of session \c session_id */
int SessionManager::errors(int session_id) {
  Session *curr = find_session(session_id);
  if (curr == NULL) return NO_SUCH_SESSION;
  return curr->errors.size();
}

/** Get error number \c error_no of session \c session_id */
string SessionManager::get_error(int session_id, size_t error_no) {
  Session *curr = find_session(session_id);
  if (curr == NULL) return "";
  list<tError>::iterator it = curr->errors.begin();
  for (; error_no > 0 && it != curr->errors.end(); --error_no) ++it;
  return it != curr->errors.end() ? (*it).getMessage() : string();
}


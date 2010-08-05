/* -*- Mode: C++ -*- */

#ifndef _API_H
#define _API_H

#include "sessionmanager.h"
#include <string>

/** general setup of globals, etc. Parses \c argc and \c argv like command
 *  line options. If there is a non-option argument at the end of argv, it
 *  will be returned by this function, and is used as grammar file name.
 *  If \c logging_dir is \c NULL, this non-option argument will be used to
 *  determine \c logging_dir, too.
 */
const char *init(int argc, char* argv[], char *logging_dir);

/** Try to load the grammar specified by the given file name, and initialize
 *  all specified components that depend on it.
 *  \return \c false if everything went smoothly, \true in case of error
 */
bool load_grammar(std::string grammar_file_name);

/** clean up internal data structures, return zero if everything went smoothly
 */
int shutdown();

/** Start a new parse by providing a new input std::string. The method return a
 *  parse session ID that is used to identify the data structures associated
 *  with this parse. If the session ID is zero, no parse session could be
 *  starteed.
 *  Currently, it is not possible to have more than one active parse session,
 *  which means that you have to end the last session before starting a new
 *  one.
 */
int start_parse(std::string input) {
  return SessionManager::getManager().start_parse(input);
}

/** Run the parser and collect the results according to the specified options
 *  \todo it would be nice if it would be possible to run the parser up to
 *        the first result, restart it, etc., maybe even add new input, or
 *        activate other robustness processing in case it can find no result
 */
int run_parser(int session_id) {
  return SessionManager::getManager().run_parser(session_id);
}

/** Return the number of results found */
int results(int session_id) {
  return SessionManager::getManager().results(session_id);
}

/** Get result number \c result_no of session \c session_id in the specified
 *  \c format, which is one of:
 *  "fs-readable", "fs-compact", "fs-jxchg"
 *  "derivation-default", "derivation-tsdb", "derivaton-xmllabel"
 *  "mrs-new", "mrs-simple"
 */
std::string get_result(int session_id, size_t result_no, std::string format) {
  return SessionManager::getManager().get_result(session_id, result_no, format);
}

/** Get the number of errors of session \c session_id */
int errors(int session_id) {
  return SessionManager::getManager().errors(session_id);
}

/** Get error number \c error_no of session \c session_id */
std::string get_error(int session_id, size_t error_no) {
  return SessionManager::getManager().get_error(session_id, error_no);
}

/** close the session with id \c session_id. */
int end_parse(int session_id) {
  return SessionManager::getManager().end_parse(session_id);
}

#endif

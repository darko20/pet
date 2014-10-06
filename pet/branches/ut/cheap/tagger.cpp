/* ex: set expandtab ts=2 sw=2: */
/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
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
#include "tagger.h"
#include <cstring>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sys/types.h>
#include <signal.h>

#include "cheap.h"
#include "settings.h"
#include "options.h"
#include "logging.h"
#include "mfile.h"

using namespace std;

tTntCompatTagger::tTntCompatTagger()
  : _settings(NULL), _out(-1), _in(-1)
{

  //
  // the '-tagger' command line option takes an optional argument, which can be
  // a settings file to be read (which, for convenience, will be installed as
  // an overlay) to the global settings object.  much like all settings files,
  // the file can be located in either the 'base' directory (where the grammar
  // file resides), or the general settings directory ('pet/').
  //
  string name = get_opt_string("opt_tagger");
  if(!name.empty() && name != "null") {
    _settings = new settings(name, cheap_settings->base(), "reading");

    if(!_settings->valid())
      throw tError("Unable to read tagger configuration '" + name + "'.");

    cheap_settings->install(_settings);
  } // if

  struct setting *foo;
  if ((foo = cheap_settings->lookup("tnt-command")) == NULL) {
    throw tError("No tagger command found. "
                 "Please check the 'tnt-command' setting.");
  }

  if ((foo = cheap_settings->lookup("tnt-utterance-start")) != NULL )
    _utterance_start = foo->values[0];
  if ((foo = cheap_settings->lookup("tnt-utterance-end")) != NULL )
    _utterance_end = foo->values[0];

  int fd_read[2], fd_write[2];
  if (pipe(fd_write) < 0) 
    throw tError("Can't open a writing pipe.");
  if (pipe(fd_read) < 0) {
    close(fd_write[0]);
    close(fd_write[1]);
    throw tError("Can't open reading pipe.");
  }
  _taggerpid = fork();
  if (_taggerpid < 0) {
    close(fd_write[0]);
    close(fd_write[1]);
    close(fd_read[0]);
    close(fd_read[1]);
    throw tError("Couldn't fork() for tagger.");
  } 
  if (_taggerpid == 0) { //child process
    close(fd_write[1]);
    close(fd_read[0]);
    dup2(fd_write[0], 0); //map writing pipe to tagger stdin
    close(fd_write[0]);
    dup2(fd_read[1], 1); //map reading pipe to tagger stdout
    close(fd_read[1]);

    string cmd(string("exec ") + cheap_settings->value("tnt-command"));
    if ((foo = cheap_settings->lookup("tnt-arguments")) != NULL) {
      for (int i = 0; i < foo->n; ++i) {
        cmd += " ";
        cmd += foo->values[i];
      }
    }
    //
    // we want to go through a shell to expand environment variables; prefix 
    // with exec(1) (see above), to streamline the process hierarchy.
    // 
    execl("/bin/sh", "sh", "-c", cmd.c_str(), (char *) 0);
    // we should never get here
    perror("tnt");
    throw tError("Tagger command failed.");
  } 
  else { //parent
    close(fd_write[0]);
    close(fd_read[1]);
    _out = fd_write[1];
    _in = fd_read[0];
  }
}

tTntCompatTagger::~tTntCompatTagger()
{
  if(_out >= 0) close(_out);
  if(_in >= 0) close(_in);
  _out = _in = -1;

  if (_taggerpid > 0) kill(_taggerpid, SIGTERM);

  if(_settings != NULL) {
    if(cheap_settings != NULL) cheap_settings->uninstall(_settings);
    delete _settings;
    _settings = NULL;
  } // if
}

void tTntCompatTagger::compute_tags(myString s, inp_list &tokens_result)
{
  struct MFILE *mstream = mopen();
  if (!_utterance_start.empty()) //input sentence start sentinel
    mprintf(mstream, "%s\n", _utterance_start.c_str());
  //one token per line
  for (inp_iterator iter = tokens_result.begin();
       iter != tokens_result.end();
       ++iter)
    mprintf(mstream, "%s\n", map_for_tagger((*iter)->orth().c_str()));
  if (!_utterance_end.empty()) //input sentence end sentinel
    mprintf(mstream, "%s\n", _utterance_end.c_str());
  mprintf(mstream, "\n");
  socket_write(_out, mstring(mstream));
  mclose(mstream);

  static int size = 4096;
  static char *input = (char *)malloc(size);
  assert(input != NULL);

  bool seen_sentinel = false;
  inp_iterator token = tokens_result.begin();
  while(token != tokens_result.end()) {

    //
    // read one line of text from our input pipe, increasing our input buffer
    // as needed.
    //
    int status = socket_readline(_in, input, size);
    while(status > size) {
      status = size;
      size += size;
      input = (char *)realloc(input, size);
      assert(input != NULL);
      status += socket_readline(_in, &input[status], size - status);
    } /* if */

    if(status <= 0)
      throw tError("low-level communication failure with tagger process");

    //
    // skip over empty lines (which in principle are sentence separators, but
    // in our loosely sychronized coupling (to work around what appears a TnT
    // bug for some inputs, e.g. 'the formal chair'), these can either occur
    // initially or finally, i.e. we align inputs and outputs at the token
    // level, instead of at the sentence level.
    // 
    if(status == 1 && input[0] == (char)0) continue;

    if ((! _utterance_start.empty()) && token == tokens_result.begin() 
      && input[0] == _utterance_start.at(0) && seen_sentinel == false) {
      int len = _utterance_start.length();
      if (status >= len && _utterance_start.compare(0, len, input, len) == 0) {
          seen_sentinel = true;
          continue; //sentence start sentinel
      }
    }

    istringstream line(input);
    string form, tag;
    double probability;
    line >> form;
#if 0
    // _fix_me_
    // with the addition of a 'tnt-mapping' mechanism (to be implemented),
    // the surface form given to TnT and coming back can differ from what was
    // in the original input; possibly, we should record the mapping result in
    // our input items, but with (potentially) multiple taggers and (as of now)
    // no downstream need to access the mapped strings, why bother?
    //                                                           (6-aug-11; oe)
    if (form.compare((*token)->orth()))
      throw tError("Tagger error: |" 
                   + form + "| vs. |" + (*token)->orth() + "|.");
#endif

    postags poss;
    while (!line.eof()) {
      line >> tag >> probability;
      if (line.fail())
        throw tError("Malformed tagger ouput: " + string(input) + ".");
      poss.add(tag, probability);
      (*token)->set_in_postags(poss);
    } // while
    ++token;

    if(token == tokens_result.end() && (! _utterance_end.empty())) {
      //read utterance end token
      status = socket_readline(_in, input, size);
      while(status > size) {
        status = size;
        size += size;
        input = (char *)realloc(input, size);
        assert(input != NULL);
        status += socket_readline(_in, &input[status], size - status);
      } /* if */

      if(status <= 0)
        throw tError("low-level communication failure with tagger process");

      int len = _utterance_end.length();
      if (status >= len && _utterance_end.compare(0, len, input, len) == 0) 
        continue; //sentence end sentinel
      else
        LOG(logAppl, WARN, "Got '" << input << "' instead of utterance end.");
    }
  } // while
}

const char *tTntCompatTagger::map_for_tagger(const string form)
{
  setting *set = cheap_settings->lookup("tnt-mapping");
  if (set == NULL) return form.c_str();
  for (int i = 0; i < set->n; i+=2) {
    if(i+2 > set->n) {
      LOG(logAppl, WARN, "incomplete last entry in tagger mapping - ignored");
      break;
    }
    if (form.compare(set->values[i]) == 0) return set->values[i+1];
  }
  return form.c_str();
}


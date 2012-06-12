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
#include <errno.h>

#include "cheap.h"
#include "settings.h"
#include "options.h"
#include "logging.h"
#include "mfile.h"

using namespace std;

tComboPOSTagger::tComboPOSTagger() : _settings(NULL)
{
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
  if ((foo = cheap_settings->lookup("taggers")) == NULL) 
    throw tError("No taggers defined. Please check the 'taggers' setting.");

  // initialise each requested tagger
  for (int i=0; i < foo->n; ++i) {
    run_tagger(foo->values[i]);
  }
}

tComboPOSTagger::~tComboPOSTagger()
{
  for(map<tagger_name, int>::iterator it = _taggerout.begin();
    it != _taggerout.end(); ++it) {
      if (it->second >= 0) close(it->second);
  }
  for(map<tagger_name, int>::iterator it = _taggerin.begin();
    it != _taggerin.end(); ++it) {
      if (it->second >= 0) close(it->second);
  }
  for(map<tagger_name, pid_t>::iterator it = _taggerpids.begin();
    it != _taggerpids.end(); ++it) {
      if (it->second > 0) kill(it->second, SIGTERM);
  }

  if(_settings != NULL) {
    if(cheap_settings != NULL) cheap_settings->uninstall(_settings);
    delete _settings;
    _settings = NULL;
  } // if
}

void tComboPOSTagger::run_tagger(tagger_name tagger)
{
  //check for required settings
  vector<string> req;
  req.push_back("-command");
  req.push_back("-tok_sep");
  req.push_back("-utt_sep");
  req.push_back("-tag_format");
  struct setting *foo;
  for (vector<string>::iterator it = req.begin(); it != req.end(); ++it) {
    string set = string(tagger + *it);
    if ((foo = cheap_settings->lookup(set.c_str())) == NULL)
      throw tError("Missing required setting for " + tagger +". "
                   "Please check the " + set + " setting.");
  }

  //open pipes and fork
  int fd_read[2], fd_write[2];
  if (pipe(fd_write) < 0) 
    throw tError("Can't open a writing pipe: " + string(strerror(errno)));
  if (pipe(fd_read) < 0) {
    close(fd_write[0]);
    close(fd_write[1]);
    throw tError("Can't open reading pipe: " + string(strerror(errno)));
  }
  pid_t _taggerpid = fork();
  if (_taggerpid < 0) {
    close(fd_write[0]);
    close(fd_write[1]);
    close(fd_read[0]);
    close(fd_read[1]);
    throw tError("Couldn't fork() for tagger: " + string(strerror(errno)));
  } 
  if (_taggerpid == 0) { //child process
    close(fd_write[1]);
    close(fd_read[0]);
    dup2(fd_write[0], 0); //map writing pipe to tagger stdin
    close(fd_write[0]);
    dup2(fd_read[1], 1); //map reading pipe to tagger stdout
    close(fd_read[1]);

    string cmd(string("exec ") + get_tagger_setting(tagger, "-command"));
    string arg_setting = tagger + "-arguments";
    if ((foo = cheap_settings->lookup(arg_setting.c_str())) != NULL) {
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
    perror(tagger.c_str());
    throw tError("Tagger command failed.");
  } 
  else { //parent
    _taggerpids[tagger] = _taggerpid;
    close(fd_write[0]);
    close(fd_read[1]);
    _taggerout[tagger] = fd_write[1];
    _taggerin[tagger] = fd_read[0];
  }
}

void tComboPOSTagger::compute_tags(myString s, inp_list &tokens_result)
{
  setting *foo = cheap_settings->lookup("taggers");
  for (int i=0; i < foo->n; ++i) {
    string tagger = foo->values[i];
    signal(SIGPIPE, SIG_IGN); //Handle SIGPIPE, don't die if tagger dies
    write_to_tagger(tagger, tokens_result);
    read_from_tagger(tagger, tokens_result);
    signal(SIGPIPE, SIG_DFL);
  }
}

void tComboPOSTagger::write_to_tagger(tagger_name tagger, 
  inp_list &tokens_result)
{
  string utt_start, utt_end, tok_sep, pos_sep, utt_sep;
  utt_start = get_tagger_setting(tagger, "-utterance-start");
  utt_end = get_tagger_setting(tagger, "-utterance-end");
  utt_sep = get_tagger_setting(tagger, "-utt_sep");
  tok_sep = get_tagger_setting(tagger, "-tok_sep");
  pos_sep = get_tagger_setting(tagger, "-pos_sep");

  struct MFILE *mstream = mopen();
  if (!utt_start.empty()) {//input sentence start sentinel
    mprintf(mstream, "%s", utt_start.c_str());
    mprintf(mstream, "%s", tok_sep.c_str());
  }
  for (inp_iterator iter = tokens_result.begin();
       iter != tokens_result.end(); ++iter) {
    if (iter != tokens_result.begin()) {
      mprintf(mstream, "%s", tok_sep.c_str());
    }
    mprintf(mstream, "%s", map_for_tagger(tagger, (*iter)->orth().c_str()));
    if (pos_sep != "") {
      postags oldposs = (*iter)->get_in_postags();
      if (oldposs.empty())
        throw tError("No assigned POS tags to be used as input to " + tagger
          + " tagger.");
      vector<string> taglist;
      vector<double> problist;
      oldposs.tagsnprobs(taglist, problist);
      mprintf(mstream, "%s%s", pos_sep.c_str(), taglist[0].c_str());
    }
  }
  if (!utt_end.empty()) { //input sentence end sentinel
    mprintf(mstream, "%s", tok_sep.c_str());
    mprintf(mstream, "%s", utt_end.c_str());
  }
  mprintf(mstream, "%s", utt_sep.c_str());
  socket_write(_taggerout[tagger], mstring(mstream));
  mclose(mstream);
}

void tComboPOSTagger::read_from_tagger(tagger_name tagger, 
  inp_list &tokens_result)
{
  string utt_start, utt_end, tag_format, tag_combine, namedentities, tag_prefix;
  utt_start = get_tagger_setting(tagger, "-utterance-start");
  utt_end = get_tagger_setting(tagger, "-utterance-end");
  tag_format = get_tagger_setting(tagger, "-tag_format");
  tag_combine = get_tagger_setting(tagger, "-tag_combine");
  tag_prefix = get_tagger_setting(tagger, "-tag_prefix");
  namedentities = get_tagger_setting(tagger, "-namedentities");

  string input; int status; bool seen_sentinel = false;
  inp_list ne_tokens; //any new NE tokens
  string ne;
  tInputItem *ne_start = NULL, *ne_end = NULL;

  inp_iterator token = tokens_result.begin();
  while(token != tokens_result.end()) {
    try {
      status = get_next_line(_taggerin[tagger], input); 
    }
    catch (tError e) {
      LOG(logAppl, WARN, e.getMessage().c_str());
      break;
    }
    if ((! utt_start.empty()) && token == tokens_result.begin() 
      && input.at(0) == utt_start.at(0) && seen_sentinel == false) {
      int len = utt_start.length();
      if (status >= len && utt_start.compare(0, len, input.c_str(), len) == 0) {
          seen_sentinel = true;
          continue; //sentence start sentinel
      }
    }

    istringstream line(input.c_str());
    string form, base, tag, chunktag, netag;
    double probability;
    postags poss, oldposs;
    if (tag_combine == "add") {
      poss = (*token)->get_in_postags();
    }

    if (tag_format == "multi") {
      line >> form;
      while (!line.eof()) {
        line >> tag >> probability;
        if (line.fail())
          throw tError("Malformed tagger ouput: " + input + ".");
        tag = string(tag_prefix+tag);
        poss.add(tag, probability);
      } // while
    } else if (tag_format == "single") {
      line >> form >> tag;
      if (line.fail())
        throw tError("Malformed tagger ouput: " + input + ".");
      tag = selective_tag_replace(tagger, *token, tag);
      tag = string(tag_prefix+tag);
      poss.add(tag, 1);
    } else if (tag_format == "genia") {
      line >> form >> base >> tag >> chunktag >> netag;
      tag = selective_tag_replace(tagger, *token, tag);
      tag = string(tag_prefix+tag);
      poss.add(tag, 1);
    }
    (*token)->set_in_postags(poss);
    if (namedentities == string("yes")) {
      if (!ne.empty() && (netag.at(0) == 'O' || netag.at(0) == 'B')) {
        //last line finished an NE
        tInputItem *tok = new tInputItem("", ne_start->start(), 
          ne_end->end(), 
          ne_start->startposition(),
          ne_end->endposition(), ne, ne);
        postags neposs;
        neposs.add("NNP", 1);
        tok->set_in_postags(neposs);
        ne_tokens.push_back(tok);
        ne.clear();
      }
      if (netag.at(0) == 'B') {
        ne = string(form);
        ne_start = *token;
        ne_end = *token;
      } else {
        if (netag.at(0) == 'I') {
          ne += " ";
          ne += form;
          ne_end = *token;
        }
      }
    }
    ++token;

    //read utterance end token
    if(token == tokens_result.end() && (!utt_end.empty())) {
      status = get_next_line(_taggerin[tagger], input);
      int len = utt_end.length();
      if (status >= len && utt_end.compare(0, len, input.c_str(), len) == 0) 
        continue; //sentence end sentinel
      else
        LOG(logAppl, WARN, "Got '" << input << "' instead of utterance end.");
    }
  } // while
  if (namedentities == string("yes")) {
    for (inp_iterator netoken = ne_tokens.begin();
      netoken != ne_tokens.end(); ++netoken) {
      tokens_result.push_back(*netoken);
    }
  }
}

string tComboPOSTagger::get_tagger_setting(tagger_name tagger, string set) {
  struct setting *foo;
  string val;
  if ((foo = cheap_settings->lookup(string(tagger+set).c_str())) != NULL)
    val = foo->values[0];
  return val;
}
    
const char *tComboPOSTagger::map_for_tagger(tagger_name tagger, 
  const string form)
{
  setting *set = cheap_settings->lookup(string(tagger+"-mapping").c_str());
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

string tComboPOSTagger::selective_tag_replace(tagger_name tagger, 
  tInputItem *token, string tag) {
  string newtag = tag;
  setting *foo;
  if ((foo = cheap_settings->lookup(string(tagger+"-fallback").c_str())) 
    != NULL) {
    postags oldposs = token->get_in_postags();
    if (!oldposs.empty()) { 
      for (int i = 0; i < foo->n; i+=2) {
        if(i+2 > foo->n) {
          LOG(logAppl, WARN, "incomplete last entry in fallback - ignored");
          break;
        }
        if (oldposs.contains(foo->values[i]) && 
          tag.compare(foo->values[i+1]) == 0) {
          newtag = foo->values[i];
          break;
        }
      }
    }
  } 
  return newtag;
}

int tComboPOSTagger::get_next_line(int fd, string &input)
{
  static int size = 4096;
  static char *buffer = (char *)malloc(size);
  assert(buffer != NULL);

  int status = socket_readline(fd, buffer, size);

  //
  // skip over empty lines (which in principle are sentence separators, but
  // in our loosely sychronized coupling (to work around what appears a TnT
  // bug for some inputs, e.g. 'the formal chair'), these can either occur
  // initially or finally, i.e. we align inputs and outputs at the token
  // level, instead of at the sentence level.
  // 
  while (status == 1 && buffer[0] == (char)0)
    status = socket_readline(fd, buffer, size);
  while (status > size) {
    status = size;
    size += size;
    buffer = (char *)realloc(buffer, size);
    assert(buffer != NULL);
    status += socket_readline(fd, &buffer[status], size - status);
  }

  if(status <= 0)
    throw tError("Low-level communication failure with tagger process: " + 
      string(strerror(errno)) + ". Tags may not be used.");

  input = string(buffer);
  return status;
}

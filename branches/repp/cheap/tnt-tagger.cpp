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
#include "tnt-tagger.h"
#include <cstring>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sys/types.h>
#include <signal.h>

#include "cheap.h"
#include "settings.h"
#include "logging.h"

using namespace std;

tTntCompatTagger::tTntCompatTagger(const string conf, const string
  &grammar_file_name) :_tnt_settings(NULL) 
{
  if (!conf.empty()) { //update cheap settings using conf file
    string conffname(find_set_file(conf, SET_EXT, grammar_file_name));
    if (conffname.empty()) 
      throw tError("Tagger settings file `" + conf + "' not found.");
    string  tntset, line;
    ifstream conffile(conffname.c_str());
    if (conffile.is_open()) {
      while (getline(conffile, line)){
        tntset += line + "\n"; //newline seems necessary with tagger.set
      }
    } else {
      throw tError("Couldn't open tagger settings file `" + conf + "'.");
    }
    _tnt_settings = new settings(tntset);
    cheap_settings->install(_tnt_settings);
  }

  struct setting *tntsetting;
  if ((tntsetting = cheap_settings->lookup("tagger-command")) == NULL) {
    throw tError("No tagger command found. Check tagger-command setting.");
  }

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

    string cmd = cheap_settings->lookup("tagger-command")->values[0];
    if ((tntsetting = cheap_settings->lookup("tagger-arguments")) != NULL) {
      for (int i = 0; i < tntsetting->n; ++i) {
        cmd += " ";
        cmd += tntsetting->values[i];
      }
    }
    //need to run from the shell to expand environment variables
    //can't use popen(3) since it's unidirectional and we want to read and write
    //from the tagger, hence execl()
    execl("/bin/sh", "sh", "-c", cmd.c_str(), (char *) 0);
//    execl("/home/rdridan/delphin/logon/bin/tnt", "tnt", "-z100", "-v0", 
//      "/home/rdridan/delphin/logon/coli/tnt/models/wsj", "-", (char *) 0);
    perror("tnt");
    throw tError("Tagger command failed."); //should never get here
  } else { //parent
    close(fd_write[0]);
    close(fd_read[1]);

    if (! (_taggerwriter_fp = fdopen(fd_write[1], "w"))) {
      close(fd_write[1]);
      throw tError("Couldn't open pipe to tagger.");
    }
    if (! (_taggerreader_fp = fdopen(fd_read[0], "r"))) {
      fclose(_taggerwriter_fp);
      close(fd_read[0]);
      throw tError("Couldn't open pipe from tagger.");
    }
  }
}

tTntCompatTagger::~tTntCompatTagger()
{
  if (_taggerwriter_fp)
    fclose(_taggerwriter_fp);
  if (_taggerreader_fp)
    fclose(_taggerreader_fp);
  if (_taggerpid > 0)
    kill(_taggerpid, SIGTERM);

  if (cheap_settings != NULL && _tnt_settings != NULL) {
    cheap_settings->uninstall(_tnt_settings);
    delete _tnt_settings;
  }
}

void tTntCompatTagger::compute_tags(myString s, inp_list &tokens_result)
{
  //one token per line
  for (inp_iterator iter = tokens_result.begin();
    iter != tokens_result.end(); ++iter)
    fprintf(_taggerwriter_fp, "%s\n", (*iter)->orth().c_str());
  if (!fprintf(_taggerwriter_fp, "\n")) {
    perror("fprintf");
    exit(1);
  }
  fflush(_taggerwriter_fp);

  string line;
  int c,line_count = 0;
  inp_iterator tokenit = tokens_result.begin();
  while ((c = fgetc(_taggerreader_fp)) != EOF) {
//    cerr << "c is `" << c << "'" << endl;
    if (c == '\n') {
      if (line.empty()) {
 //       cerr << "breaking" << endl;
        break; // empty line == end of sentence
      }
  //    cerr << "line is `" << line << "'" << endl;
      istringstream tagline(line);
      string tok, tag;
      double prob;
      tagline >> tok;
      if (tokenit != tokens_result.end()) {
        if (tok.compare((*tokenit)->orth()) != 0)
          throw tError("Tagger error: " + tok + " vs " + (*tokenit)->orth());
      } else
          throw tError("Tagger error: no input item for " + tok +".");
      postags poss;
      while (!tagline.eof()) {
        tagline >> tag >> prob;
        if (tagline.fail())
          throw tError("Malformed tagger ouput: " + line + ".");
        poss.add(tag, prob);
        (*tokenit)->set_in_postags(poss);
      }
      ++line_count;
      ++tokenit;
      line.clear();
//      cerr << "cleared line" << endl;
    } else {
      line += c;
    }
  }
//  cerr << "got here" << endl;
}


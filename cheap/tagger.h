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
#ifndef _TAGGER_H_
#define _TAGGER_H_

#include <vector>
#include <string>
#include <map>
#include "input-modules.h"
#include "settings.h"

class tComboPOSTagger : public tPOSTagger {
  public:
    tComboPOSTagger();
    ~tComboPOSTagger();

    typedef std::string tagger_name;

    virtual void compute_tags(myString s, inp_list &tokens_result);
    virtual std::string description() { return "Combo tagger"; }

  private:
    void run_tagger(tagger_name tagger);
    void write_to_tagger(tagger_name tagger, inp_list &tokens_result);
    void read_from_tagger(tagger_name tagger, inp_list &tokens_result);
    const char *map_for_tagger(tagger_name tagger, const std::string form);
    std::string get_tagger_setting(tagger_name tagger, std::string set);
    int get_next_line(int fd, std::string &input);
    
    settings *_settings;
    std::map<tagger_name, pid_t> _taggerpids;
    std::map<tagger_name, int> _taggerout;
    std::map<tagger_name, int> _taggerin;
};

#endif


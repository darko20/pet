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
#ifndef _REPP_H_
#define _REPP_H_

#include <vector>
#include <string>
#include <map>
#include <set>
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include "input-modules.h"

#define RPP_EXT ".rpp"

// defines three related classes: 
// * tReppTokenizer as the PET tokenizer class
// * tRepp as a class for each repp module
// * tReppRule virtual class which has implemented classes for each rule type


typedef boost::u32regex tRegex;
class tReppRule; //forward declaration of REPP rule class
class tRepp; //forward declaration of REPP class
typedef std::vector<tReppRule *> tReppGroup;

class tReppTokenizer : public tTokenizer {
  public:
    tReppTokenizer();
    ~tReppTokenizer();
    virtual void tokenize(myString s, inp_list &result);
    virtual std::string description() { return "REPP tokenizer"; }

    virtual position_map position_mapping() {return STANDOFF_POINTS;}

    tRepp *getRepp(std::string name){return _repps[name];}
    void addStartMap(std::vector<int> *map){_startmap.push_back(map);}
    void addEndMap(std::vector<int> *map){_endmap.push_back(map);}

  private:
    std::string _format; //output format

    std::map<std::string, tRepp *> _repps; //modules

    std::vector<std::vector<int> *> _startmap; // for keeping track of 
    std::vector<std::vector<int> *> _endmap;   // characterisation

    settings *_update;
};

class tRepp {
  public:
    tRepp(std::string name, tReppTokenizer *parent);
    ~tRepp();
    tReppGroup *getGroup(int id){return _groups[id];}
    std::vector<tReppRule *> &rules(){return _rules;}
    void read_file(const char *fname) {}; //included files, unimplemented
    tRegex _tokenizer; // tokenisation pattern
    tReppTokenizer *getParent() {return _parent;}

  private:
    std::string _id; //repp name

    std::string _version; 
    std::map<int, tReppGroup *> _groups; //iterative groups
    tReppTokenizer *_parent; //tokenizer
    std::vector<tReppRule *> _rules; //repp rules in file order
};

class tReppRule {
  public:
    tReppRule(std::string type):_type(type){};
    std::string get_type() { return _type; };
    virtual std::string name() { return _type; };
    virtual std::string apply(tRepp *r, std::string item){ return
    std::string(); };

  protected:
    std::string _type;
};

class tReppFSRule: public tReppRule {
  public:
    tReppFSRule(std::string type, const char *target, const char *format);
    std::string apply(tRepp *r, std::string item);
    std::string name() { return _targetstr; };

  private:
    tRegex _target; //lhs of rule
    std::string _targetstr; //mostly for debugging
    std::string _format; //rhs of rule
    std::vector<int> _cgroups; //offsets of capture groups in format
};

class tReppGroupRule: public tReppRule {
  public:
    tReppGroupRule(std::string type, int group)
      :tReppRule(type), _group_id(group){};
    std::string apply(tRepp *r, std::string item);
    std::string name();

  private:
    int _group_id;
};

class tReppIncludeRule: public tReppRule {
  public:
    tReppIncludeRule(std::string type, std::string iname)
      :tReppRule(type), _iname(iname){};
    std::string apply(tRepp *r, std::string item);
    std::string name() {return _iname;};

  private:
    std::string _iname;
};

#endif

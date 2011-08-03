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
#include "repp.h"
#include <stack>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include "unicode.h"
#include "settings.h"
#include "options.h"
#include "utility.h"
#include "cheap.h"
#include "logging.h"

using namespace std;

tReppTokenizer::tReppTokenizer() 
  : _update(NULL) 
{
  //
  // the '-repp' command line option takes an optional argument, which can be
  // a settings file to be read (which, for convenience, will be installed as
  // an overlay) to the global settings object.  much like all settings files,
  // the file can be located in either the 'base' directory (where the grammar
  // file resides), or the general settings directory ('pet/').
  //
  string name = get_opt_string("opt_repp");
  if(!name.empty()) {
    _update = new settings(name, cheap_settings->base(), "reading");

    if(!_update->valid())
      throw tError("Unable to locate REPP configuration '" + name + "'.");

    cheap_settings->install(_update);
  } // if

  // sanity check: make sure there is a top-level entry point
  if(cheap_settings->lookup("repp-tokenizer") == NULL)
    throw tError("No REPP top-level module; "
                 "please check the 'repp-tokenizer' setting.");

  // find modules from setting 'repp-modules' and read in each file
  struct setting *foo;
  if ((foo = cheap_settings->lookup("repp-modules")) == NULL)
    throw tError("No REPP modules defined; "
                 "please check the 'repp-modules' setting.");

  for (int i = 0; i < foo->n; ++i) {
    LOG(logRepp, INFO, "initializing REPP " << foo->values[i]);
    _repps[foo->values[i]] = new tRepp(foo->values[i], this);
  } // for
}

tReppTokenizer::~tReppTokenizer()
{
  for (map<string, tRepp *>::iterator iter = _repps.begin();
    iter != _repps.end(); ++iter)
    delete iter->second;

  if(_update != NULL) {
    if(cheap_settings != NULL) cheap_settings->uninstall(_update);
    delete _update;
    _update = NULL;
  } // if
}

tRepp::tRepp(string name, tReppTokenizer *parent) :_id(name), _parent(parent)
{
  tRegex emptyre = boost::make_u32regex("^$");
  tRegex commentre = boost::make_u32regex("^;.*$");
  tRegex versionre = boost::make_u32regex("^@\\$Date: (.*) \\$$");
  tRegex includere = boost::make_u32regex("^<(\\S+)$");
  tRegex rungroupre = boost::make_u32regex("^>(\\d+)$");
  tRegex readreppre = boost::make_u32regex("^>(\\S+)$");
  tRegex tokre = boost::make_u32regex("^:(.*)$");
  tRegex groupstartre = boost::make_u32regex("^#(\\d+)$");
  tRegex groupendre = boost::make_u32regex("^#$");
  tRegex rulere = boost::make_u32regex("^([!-+^])([^\\t]+)\\t+([^\\t]*)$");

  string file = find_file(name, RPP_EXT, cheap_settings->base());
  if(file.empty())
    file = find_file(name, RPP_EXT,
                     cheap_settings->base() + REPP_SUBDIRECTORY + PATH_SEP);

  ifstream mainf(file.c_str());

  if (mainf.is_open()) {
    int rule_count = 0;
    string line;
    stack<int> in_group;

    int line_no=0;
    getline(mainf,line);
    while (!mainf.eof()) {
      line_no++;
      boost::smatch res;

      if (!boost::u32regex_match(line, res, emptyre) && //not blank
        !boost::u32regex_match(line, res, commentre)){ //or comment
        if (boost::u32regex_match(line, res, versionre))
          _version = res[1];
        else if (boost::u32regex_match(line, res, tokre))
          _tokenizer.assign(boost::make_u32regex(res.str(1).c_str()));
        else if (boost::u32regex_match(line, res, includere)) {
          read_file(res.str(1).c_str()); //TODO
        }
        else if (boost::u32regex_match(line, res, rungroupre)) {
          int rgroup;
          istringstream(res[1]) >> rgroup;
          tReppGroupRule *newrule = new tReppGroupRule(">G", rgroup);
          _rules.push_back(newrule);
          rule_count++;
        }
        else if (boost::u32regex_match(line, res, readreppre)) {
          string const rname(res.str(1).c_str());
          tReppIncludeRule *newrule = new tReppIncludeRule(">I", rname);
          _rules.push_back(newrule);
          rule_count++;
        }
        else if (boost::u32regex_match(line, res, groupstartre)) {
          int rgroup;
          istringstream(res[1]) >> rgroup;
          in_group.push(rgroup);
          _groups.insert(map<int, tReppGroup *>::value_type(rgroup, 
            new tReppGroup));
        }
        else if (boost::u32regex_match(line, res, groupendre)) {
          if (in_group.empty()) {
            LOG(logRepp, WARN, "REPP:" << _id << ":" << line_no 
              << " spurious group close.");
          }
          else
            in_group.pop();
        }
        else if (boost::u32regex_match(line, res, rulere)) {
          tReppFSRule *newrule = new tReppFSRule(res[1], 
            res.str(2).c_str(), res.str(3).c_str());
          if (in_group.empty())
            _rules.push_back(newrule);
          else
            (*(_groups[in_group.top()])).push_back(newrule);
          rule_count++;
        }
        else {
          LOG(logRepp, WARN, "REPP:" << _id << ":" << line_no 
            << " invalid line: " << line);
        }
      }
      getline(mainf, line);
    }
    LOG(logRepp, INFO, "Read " << _id << " [" << rule_count << " rules]");
  }
  else {
    throw tError("Couldn't find REPP module '" + name + 
      "'. Check repp-modules setting.");
  }
}

tRepp::~tRepp()
{

  for (map<int, tReppGroup *>::iterator iter = _groups.begin();
    iter != _groups.end(); ++iter) {
    //delete the group's rules
    for (vector<tReppRule *>::iterator iter2 = (iter->second)->begin();
      iter2 != (iter->second)->end(); ++iter2)
      delete *iter2;
    delete iter->second;
  }
  for (vector<tReppRule *>::iterator iter = _rules.begin();
    iter != _rules.end(); ++iter)
    delete *iter;
}

void tReppTokenizer::tokenize(myString item, inp_list &result) 
{
  boost::smatch res;
  string rest(item);

  //initialise the mappings to make the start and end anchor cells work
  vector<int> *smap = new vector<int>(Conv->convert(rest).length() + 2, 0);
  vector<int> *emap = new vector<int>(Conv->convert(rest).length() + 2, 0);
  (*smap)[0] = 1;
  (*emap)[emap->size()-1] = -1;
  _startmap.push_back(smap);
  _endmap.push_back(emap);

  tRepp *repp = getRepp(cheap_settings->lookup("repp-tokenizer")->values[0]);

  //apply all rules
  for (vector<tReppRule *>::iterator iter = repp->rules().begin();
    iter != repp->rules().end(); ++iter) {
    if ((*iter)->get_type() == ">I") { //conditional include
      if (! cheap_settings->member("repp-calls", (*iter)->name().c_str()))
        continue; // don't run this optional module
    }
    rest = (*iter)->apply(repp, rest);
  }
  //all full string rules have been applied, now tokenise
  int idx = 1; // 1-based because of the anchors used in mapping
  tInputItem *tok;
  if (repp->_tokenizer.empty()) //warn? defaulting to whitespace
    repp->_tokenizer.assign(boost::make_u32regex("[ \t]+"));
  while (rest.length() > 0) {
    if (boost::u32regex_search(rest, res, repp->_tokenizer)) {
      if (res.prefix().matched) {//something found before the first match
        string surface = string(res.prefix().first, res[0].first);
        UnicodeString utok = Conv->convert(surface);
        int tokstart = idx;
        int tokend = idx + utok.length() - 1;
        for (vector<vector<int>*>::reverse_iterator it = _startmap.rbegin();
          it != _startmap.rend(); ++it) {
          tokstart += (*(*it))[tokstart];
        }
        for (vector<vector<int> *>::reverse_iterator it = _endmap.rbegin();
          it != _endmap.rend(); ++it) {
          if (tokend < (int)(*it)->size() ) {
            tokend += (*(*it))[tokend];
          } else {
            tokend += (*(*it))[(*it)->size()-1];
          }
        }
        int start = result.size();
        int end = result.size()+1;
        stringstream tokid;
        tokid << result.size()+42; //to match YY initialization
        
        LOG(logRepp, DEBUG, "creating item from " << surface << " <" 
            << tokstart-1 << ":" << tokend << ">");
        tok = new tInputItem(tokid.str().c_str(), start, end,
          tokstart-1, tokend, surface, surface);
        result.push_back(tok);
        idx += Conv->convert(string(res.prefix().first,
          res.suffix().first)).length();
      } else {
        idx += Conv->convert(string(res[0])).length();
      }
      rest = string(res.suffix().first, res.suffix().second);
    }
    else {//a one token string
      string surface = rest;
      UnicodeString utok = Conv->convert(surface);
      int tokstart = idx;
      int tokend = idx + utok.length() - 1;
      for (vector<vector<int> *>::reverse_iterator it = _startmap.rbegin();
        it != _startmap.rend(); ++it) {
        tokstart += (*(*it))[tokstart];
      }
      for (vector<vector<int> *>::reverse_iterator it = _endmap.rbegin();
        it != _endmap.rend(); ++it) {
        tokend += (*(*it))[tokend];
      }
      stringstream tokid;
      tokid << result.size();
      tok = new tInputItem(tokid.str().c_str(), tokstart-1, tokend,
        surface, surface);
      result.push_back(tok);
      rest = string();
    }
  }

  //clear memory
  for (vector<vector<int> *>::iterator it = _startmap.begin();
    it != _startmap.end(); ++it)
    delete *it;
  for (vector<vector<int> *>::iterator it = _endmap.begin();
    it != _endmap.end(); ++it)
    delete *it;
  _startmap.clear();
  _endmap.clear();
}

// standard string replacement rule
tReppFSRule::tReppFSRule(string type, const char *target, const char *format)
  : tReppRule(type), _targetstr(target)
{
  _format = string(format);
  string tmp(target);

  // boost regex specific pattern manipulation
  if (tmp[0] == '^')
    tmp.replace(0,1, "\\G", 2);
  if (tmp[0] == '(' && tmp[1] == '^')
    tmp.replace(1, 1, "\\G", 2);
  //escape braces in target pattern
  int i = 0;
  while (true) {
    i = tmp.find('{', i);
    if (i == (int)string::npos)
      break;
    tmp.replace(i, 1, "\\{", 2);
    i+=2;
  }
  i = 0;
  while (true) {
    i = tmp.find('}', i);
    if (i == (int)string::npos)
      break;
    tmp.replace(i, 1, "\\}", 2);
    i+=2;
  }

  //record capture group reference positions in format
  _cgroups.push_back(0);
  UnicodeString uformat = Conv->convert(_format);
  int flen = uformat.length();
  for (int x=0; x < flen; ++x) {
    if (uformat.charAt(x) == '\\') {
      if (x+1 < flen && isdigit(uformat.charAt(x+1))) {
        string temp = Conv->convert(uformat.charAt(x+1));
        int groupno;
        istringstream(temp) >> groupno;
        if (groupno == (int)_cgroups.size()) {
          _cgroups.push_back(x);
        }
        else
          break; //out of order group
      } else {
        if (x+1 == flen)
          LOG(logRepp, WARN, "REPP:" << "unescaped backslash in " << _format);
      }
    }  
  }

  _target = boost::make_u32regex(tmp);
}

string tReppGroupRule::name()
{
  ostringstream out;
  out << _group_id;
  return(out.str());
}

string tReppFSRule::apply(tRepp *r, string origitem)
{
  string item = origitem;
  string::const_iterator start, end;
  start = item.begin();
  end = item.end();
  boost::smatch res;
  string newstring("^");
  unsigned int stringindex = 0;
  vector<int> *smap = new vector<int>(item.length());
  vector<int> *emap = new vector<int>(item.length());
  (*smap)[stringindex] = 0; //represent zero length
  (*emap)[stringindex] = 0; //start anchor (^)
  stringindex++;

  int shift = 0;
  while (boost::u32regex_search(start, end, res, _target,
    boost::match_default|boost::format_sed)) {
    // copy string before match
    newstring += string(res.prefix().first, res.prefix().second);
    unsigned int nslen = Conv->convert(newstring).length();
    if (nslen > smap->size()) {
      smap->resize(nslen);
      emap->resize(nslen);
    }
    for (; stringindex < nslen; stringindex++) {
      (*smap)[stringindex] = shift;
      (*emap)[stringindex] = shift;
    }

    // matched portion of string
    newstring += res.format(_format, boost::match_default|boost::format_sed);
    nslen = Conv->convert(newstring).length();
    if (nslen > smap->size()) {
      smap->resize(nslen);
      emap->resize(nslen);
    }

    int mlen = Conv->convert(string(res[0])).length();
    int sublength = nslen - stringindex;
    int nextgroup = 0; //next capture group to look for
    int ingroup = 0; //capture group we are inside
    int groupspan = 0; //length of capture group
    int nextgroupstart = 0; //start index of next capture group
    int endgroup = 0; //end index of next capture group (outside the group)
    int gap; //span between groups (or string boundaries and groups)
    int newstart;
    int newend;
    if (_cgroups.size() > 1) {
      nextgroup = 1;  //looking for capture group 1
      //first group so offset in format is fixed
      nextgroupstart = _cgroups[nextgroup]; 
      gap = nextgroupstart; // gap from start of sentence to first group ref
      newstart = shift;
      //before first group, difference between matched and replaced length
      shift += Conv->convert(string(res[0].first,res[1].first)).length()
        - gap;
      newend = shift + gap - 1;
    } else { //no (in-order) capture groups
      gap = sublength; 
      newstart = shift;
      //difference between matched and replaced length of full match
      shift += mlen - gap;
      newend = shift + gap - 1;
    }
    for (int count = 0; count < sublength; count++, stringindex++) {
      if (nextgroup > 0 && count == nextgroupstart) {
        //we are looking for a group and found one, set up numbers
        ingroup = nextgroup; //in span of capture group <nextgroup>
        groupspan = Conv->convert(string(res[ingroup])).length();
        endgroup = count + groupspan;
        if ((int)_cgroups.size() > nextgroup+1) {
          nextgroup++; //next capture group to look for
          gap =  _cgroups[nextgroup] - _cgroups[ingroup] - 2; //gap between
          nextgroupstart = endgroup + gap;
        }
        else {
          nextgroup = 0; //no more capture groups
          gap = sublength - endgroup;
        }
      }
      if (ingroup) {
        if (count == endgroup) {
          //adding count so we can subtract count for the adjustment,
          //rather than subtracting index within the not_ingroup span
          newstart = shift + count; 

          if (nextgroup > 0) {  
            shift += 
              (Conv->convert(string(res[ingroup].first, 
              res[nextgroup].first)).length()
              - Conv->convert(string(res[ingroup])).length()
              - gap);
          } else {
            shift += (Conv->convert(string(res[ingroup].first,
              res[0].second)).length()
              - Conv->convert(string(res[ingroup])).length()
              - gap);
          }
          //adding count so we can subtract count for the adjustment,
          //rather than subtracting index within the not_ingroup span
          newend = shift + gap - 1 + count;
          ingroup = 0;
        }
        else {
          (*smap)[stringindex] = shift;
          (*emap)[stringindex] = shift;
        }
      } 
      if (!ingroup) { 
        (*smap)[stringindex] = newstart - count;
        (*emap)[stringindex] = newend - count;
      }
    }
    if (sublength == endgroup) { //end of match was end of group
      shift += 
        (Conv->convert(string(res[ingroup].first, 
        res.suffix().first)).length()
        - Conv->convert(string(res[ingroup])).length());
    }
    start = res[0].second;
  }
  if (start == item.begin()) { //never matched
    return origitem;
  } else {
    // copy trailing portion of string
    newstring += string(res.suffix().first, res.suffix().second);
    unsigned int nslen = Conv->convert(newstring).length();
    if (nslen >= smap->size()) {
      smap->resize(nslen+1);
      emap->resize(nslen+1);
    }
    for (; stringindex < nslen; stringindex++) {
      (*smap)[stringindex] = shift;
      (*emap)[stringindex] = shift;
    }
    // a zero length cell on the end to deal with zero length replacement
    (*smap)[nslen] = shift;
    (*emap)[nslen] = shift - 1;

    r->getParent()->addStartMap(smap);
    r->getParent()->addEndMap(emap);
    newstring.erase(0,1);
    return newstring;
  }
}

string tReppGroupRule::apply(tRepp *r, string item)
{
  tReppGroup *gptr = r->getGroup(_group_id);
  string newitem(item);
  while (true) {
    string orig = newitem;
    for (vector<tReppRule *>::iterator iter = gptr->begin();
      iter != gptr->end(); ++iter)
      newitem = (*iter)->apply(r, newitem);
    if (newitem == orig)
      break;
  }
  return newitem;
}

string tReppIncludeRule::apply(tRepp *r, string item)
{
  string newitem(item);
  tRepp *increpp = r->getParent()->getRepp(_iname);
  vector<tReppRule *> &incrules = increpp->rules();
  for (vector<tReppRule *>::iterator iter = incrules.begin();
    iter != incrules.end(); ++iter) {
    newitem = (*iter)->apply(increpp, newitem);
  }

  return newitem;
}

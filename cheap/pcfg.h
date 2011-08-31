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

/** \file pcfg.h
 * agenda-driven PCFG robust parser.
 */

#ifndef _PCFG_H_
#define _PCFG_H_


#include "errors.h"
#include "parse.h"
#include "task.h"
#include "fs.h"
#include "cheaptimer.h"
#include <string>
#include <list>


void analyze_pcfg(chart *&C, fs_alloc_state &FSAS, std::list<tError> &errors,
                  class Resources &resources);

//fs instantiate_robust(tItem* root);

class pcfg_rule_and_passive_task : public basic_task
{
public:
  virtual ~pcfg_rule_and_passive_task() {}

  pcfg_rule_and_passive_task(class chart *C, tAbstractAgenda *A,
                             grammar_rule *rule, class tItem *passive);

  /** Return start and end positions of the possibly resulting edge. */
  int start () { return _passive->start();}
  int end ()   { return _passive->end();}
  bool phrasal () { return false; }
  bool yields_passive () {return _rule->arity() ==1;}

  virtual class tItem *execute();

private:
  class grammar_rule *_rule;
  class tItem *_passive;
};

class pcfg_active_and_passive_task : public basic_task
{
public:
  virtual ~pcfg_active_and_passive_task() {}
  pcfg_active_and_passive_task(class chart *C, tAbstractAgenda *A,
                               class tItem *active, class tItem* passive);

  /** Return start and end positions of the possibly resulting edge. */
  int start () { return std::min(_passive->start(), _active->start());}
  int end ()   { return std::min(_passive->end(), _active->end());}
  bool phrasal () { return false; }
  bool yields_passive () { return true; }

  virtual tItem *execute();

private:
  class tItem *_active;
  class tItem *_passive;
};


#endif

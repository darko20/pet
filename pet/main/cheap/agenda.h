/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2003 Ulrich Callmeier uc@coli.uni-sb.de
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

/* Parser agenda: a queue of prioritized tasks */

#ifndef _AGENDA_H_
#define _AGENDA_H_

#include "task.h"

class tAgenda
{
 public:

    tAgenda()
        : _A()
    {}

    ~tAgenda();
  
    void
    push(class basic_task *t);
    
    class basic_task *
    top();

    class basic_task *
    pop();

    bool
    empty();

 private:

    std::priority_queue<class basic_task *, vector<class basic_task *>,
                        task_priority_less> _A;
};

#endif

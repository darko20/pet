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

/* tasks */

#include "pet-system.h"
#include "task.h"
#include "parse.h"
#include "chart.h"
#include "agenda.h"
#include "options.h"
#include "tsdb++.h"

int basic_task::next_id = 0;

double
packingscore(int start, int end, int n, bool active)
{
    return end - double(start) / n
           - (active ? 0.0 : double(start) / n);
}

item_task::item_task(class chart *C, class tAgenda *A, tItem *it)
    : basic_task(C, A), _item(it)
{
    if(opt_packing)
        setPriority(packingscore(it->start(), it->end(), C->rightmost(), false));
    else 
        setPriority(it->score());
}

tItem *
item_task::execute()
{
    // There should be no way this item (which must be a tLexItem)
    // has been blocked.
    assert(!(opt_packing && _item->blocked()));

    return _item;
}

active_and_passive_task::active_and_passive_task(class chart *C,
                                                 class tAgenda *A,
                                                 tItem *act, tItem *passive)
    : basic_task(C, A), _active(act), _passive(passive)
{
    if(opt_packing)
    {
        tPhrasalItem *active = dynamic_cast<tPhrasalItem *>(act); 

        int start, end;
        active->getCombinedPositions(passive, start, end);

        setPriority(packingscore(start, end,
                              C->rightmost(), !active->passive()));
    }
    else if(Grammar->sm())
    {
        tPhrasalItem *active = dynamic_cast<tPhrasalItem *>(act); 

        list<tItem *> daughters(active->getCombinedDaughters(passive));

        setPriority(Grammar->sm()->scoreLocalTree(active, daughters));
    }
}

tItem *
active_and_passive_task::execute()
{
    if(opt_packing && (_passive->blocked() || _active->blocked()))
        return 0;
    
    tItem *result = _active->combine(_passive);

    if(result) result->score(priority());

    return result;
}

void
basic_task::print(FILE *f)
{
    fprintf(f, "task #%d (%.2f)", _id, _p);
}

void
active_and_passive_task::print(FILE *f)
{
    fprintf(f,
            "task #%d {%d + %d} (%.2f)",
            _id,
            _active->id(), _passive->id(),
            _p);
}

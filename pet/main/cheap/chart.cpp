/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class `chart' */


#include "chart.h"
#include "tsdb++.h"

//#define DEBUG

chart::chart(int len)
  : _Chart(), _pedges(0),
    _Cp_start(len + 1), _Cp_end(len + 1),
    _Ca_start(len + 1), _Ca_end(len + 1),
    _item_owner()
{
}

chart::~chart()
{
}

int chart::_next_stamp = 1;

void chart::add(item *it)
{
  it->stamp(_next_stamp++);

#ifdef DEBUG
  it->print(ferr);
  fprintf(ferr, "\n");
#endif

  _Chart.push_back(it);

  if(it->passive())
    {
      _Cp_start[it->start()].push_back(it);
      _Cp_end[it->end()].push_back(it);
      _pedges++;
    }
  else
    {
      if(it->left_extending())
        _Ca_start[it->start()].push_back(it);
      else
        _Ca_end[it->end()].push_back(it);
    }
}

void chart::print(FILE *f)
{
  for(list<item *>::iterator pos = _Chart.begin(); pos != _Chart.end(); ++pos)
    {
      (*pos)->print(f);
      fprintf(f, "\n");
    }
}

void chart::get_statistics()
{
  // calculate aedges, pedges, raedges, rpedges
  chart_iter iter(this);

  long int totalsize = 0;
  
  while(iter.valid())
    {
      item *it = iter.current();

      if(it -> passive())
	{
	  stats.pedges++;
	  if(it -> result_contrib())
	    stats.rpedges++;

          fs f = it -> get_fs();
          totalsize += f.size();
	}
      else
	{
	  stats.aedges++;
	  if(it -> result_contrib())
	    stats.raedges++;
	}
      iter++;
    }
  stats.fssize = totalsize / stats.pedges;
}

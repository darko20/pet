/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class `chart' */

#include "pet-system.h"
#include "cheap.h"
#include "chart.h"
#include "tsdb++.h"

//#define DEBUG

chart::chart(int len, auto_ptr<item_owner> owner)
  : _Chart(), _Roots(), _pedges(0),
    _Cp_start(len + 1), _Cp_end(len + 1),
    _Ca_start(len + 1), _Ca_end(len + 1),
    _item_owner(owner)
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

// #define LINGO_LKB_COMPAT_HACK

#ifdef LINGO_LKB_COMPAT_HACK
bool inflectedp(item *it)
{
  dag_node *dag = dag_get_path_value(it->get_fs().dag(), "INFLECTED");
  if(dag != FAIL && dag_type(dag) ==
     lookup_type(cheap_settings->req_value("true-type")))
    return true;

  return false;
}
#endif

void chart::get_statistics()
{
  // calculate aedges, pedges, raedges, rpedges
  chart_iter iter(this);

  long int totalsize = 0;
  
  while(iter.valid())
    {
      item *it = iter.current();

      if(it->trait() == INFL_TRAIT
#ifdef LINGO_LKB_COMPAT_HACK
	 || (it->trait() == LEX_TRAIT && !inflectedp(it))
#endif
	 )
	{
	  stats.medges++;
	}
      else if(it -> passive())
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
  stats.fssize = (stats.pedges > 0) ? totalsize / stats.pedges : 0;
}

//
// shortest path algorithm for chart 
// Bernd Kiefer (kiefer@dfki.de)
//

int weight(item * i) { return 1; }

void chart::shortest_path (list <item *> &result) {

  vector<item *>::size_type size = _Cp_start.size() ;
  vector<item *>::size_type u, v ;
  unsigned int *distance = New unsigned int[size] ;
  unsigned int new_dist ;

  vector < list < unsigned int > > pred(size) ;
  list <item *>::iterator curr ;
  item *passive ;

  short int *unseen = New short int[size] ;
  list < unsigned int > current ;

  for (u = 1 ; u <= size ; u++) {
    distance[u] = UINT_MAX ;
    unseen[u] = 1 ; 
  }
  distance[0] = 0 ; unseen[0] = 1 ;

  for (u = 0 ; u < size ; u++) {
    /* this is topologically sorted order */
    for (curr = _Cp_start[u].begin() ; curr != _Cp_start[u].end() ; curr++) {
      passive = *curr ;
      v = passive->end() ; new_dist = distance[u] + weight(passive) ;
      if (distance[v] >= new_dist) {
        if (distance[v] > new_dist) {
          distance[v] = new_dist ;
          pred[v].clear() ;
        }
        pred[v].push_front(u) ;
      } 
    }
  }

  current.push_front(size - 1) ;
  while (! current.empty()) {
    u = current.front() ; current.pop_front() ;
    for (curr = _Cp_end[u].begin() ; curr != _Cp_end[u].end() ; curr++) {
      passive = *curr ;
      v = passive->start() ;
      if ((find (pred[u].begin(), pred[u].end(), v) != pred[u].end())
          && ((int) (distance[u] - distance[v])) == weight(passive)) {
        result.push_front(passive) ;
        if (unseen[v]) { current.push_front(v) ; unseen[v] = 0 ; }
      }
    }
  }
  delete[] distance;
  delete[] unseen;
}

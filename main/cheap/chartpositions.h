/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 * (C) 2001 Bernd Kiefer kiefer@dfki.de
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

// position mappings (from input positions to chart positions)

class position_map
{
public:
  virtual ~position_map() {};
  virtual void clear () = 0;
  virtual void add_start_position (int input_position) = 0;
  virtual void add_end_position (int input_position) = 0;
  virtual int chart_start_position (int input_position) = 0;
  virtual int chart_end_position (int input_position) = 0;
  virtual int max_chart_position () = 0;
};

class identity_position_map : public position_map
{
public:
  identity_position_map ()
  {
    clear ();
  }

  virtual ~identity_position_map ()
  {
  }

  virtual void clear ()
  {
    max_position = 0;
  }

  virtual void add_start_position (int input_position)
  {
    max_position = (input_position > max_position) ? input_position : max_position;
  }

  virtual void add_end_position (int input_position)
  {
    max_position = (input_position > max_position) ? input_position : max_position;
  }

  virtual int chart_start_position (int input_position)
  {
    max_position = (input_position > max_position) ? input_position : max_position;
    return input_position;
  }

  virtual int chart_end_position (int input_position)
  {
    max_position = (input_position > max_position) ? input_position : max_position;
    return input_position;
  }

  virtual int max_chart_position ()
  {
    return max_position;
  }

private:
  int max_position;
};

//
// analogous to lower_bound, but using linear interpolation search
// returns the furthermost iterator i in [first, last) such that,
// for every iterator j in [first, i), *j < value.
//
template < class RandomAccessIterator, class LessThanComparable >
  RandomAccessIterator lower_bound_ip (RandomAccessIterator first,
				       RandomAccessIterator nlast,
				       const LessThanComparable &value)
{
  if (value <= *first)
    return first;

  RandomAccessIterator last = nlast - 1;
  if (value < *last)
    {
      // estimate new position to try, second addend 0 < add < (nlast - first)
      RandomAccessIterator position
	= first + (int) ((((float) (value - *first)) / (*last - *first))
			 * (nlast - first));
      if (value < *position)
	return lower_bound_ip (first, position, value);
      else
	{
	  if (*position < value)
	    return lower_bound_ip (position + 1, nlast, value);
	  else
	    return position;
	}
    }
  else if (value > *last)
    return nlast;
  else
    return last;
}


// This position_map requires all positions to be entered before
// asking for chart postitions. It maps end points not being equal to
// any start point to the nearest start point to its right, or, if it
// is the last end point, to the rightmost chart position.  If
// positions are added in ascending order, adding is linear in the
// number of added positions.

class end_proximity_position_map : public position_map
{
public:
  end_proximity_position_map()
  {
    _max_start_position = -1;
  }

  virtual ~end_proximity_position_map ()
  {
  }

  virtual void clear ()
  {
    _max_start_position = -1;
    _starts.clear ();
  }

  virtual void add_start_position (int input_position)
  {
    if (input_position > _max_start_position)
      {
	_starts.push_back(input_position);
	_max_start_position = input_position;
      }
    else
      {
	// find the position equal to or to the right of input_position
	ppmap_vec::iterator position
	  = lower_bound_ip (_starts.begin (), _starts.end (), input_position);
	// next condition has to be false because input_position <= max_start_position
	assert (position != _starts.end ());
	// new position has to be inserted
	if (*position != input_position)
	  _starts.insert (position, input_position);
      }
  }

  virtual void add_end_position (int input_position)
  {
    /*
       if (input_position > _max_end_position) {
       _ends.push_back(input_position);
       _max_end_position = input_position ;
       }
       else {
       // find the position equal to or to the right of input_position
       ppmap_vec::iterator position
       = lower_bound_ip(_ends.begin(), _ends.end(), input_position);
       // next condition has to be false because input_position <= max_start_position
       assert (position != _ends.end()) ;
       // new positionition has to be inserted
       if (*position != input_position) _ends.insert(position, input_position) ;
       }
     */
  }

  virtual int chart_start_position (int input_position)
  {
    ppmap_vec::iterator position
      = lower_bound_ip (_starts.begin (), _starts.end (), input_position);
    if (*position == input_position)
      return position - _starts.begin ();
    else if (position == _starts.begin ())
      return 0;
    else
      return (position - _starts.begin ()) - 1;
  };

  virtual int chart_end_position (int input_position)
  {
    ppmap_vec::iterator position
      = lower_bound_ip (_starts.begin (), _starts.end (), input_position);
    return position - _starts.begin ();
  };

  virtual int max_chart_position()
  {
    return _starts.size();
  }

private:
  typedef vector<int> ppmap_vec;

  ppmap_vec _starts;
  int _max_start_position;
};

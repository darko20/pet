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

/* Stochastic modelling */

#ifndef _SM_H_
#define _SM_H_

#include "pet-system.h"
#include "grammar.h"
#include "item.h"

#define SM_EXT ".sm"

/** Stochastic Model.
 *  This class represents an abstract stochastic model. It reads the model
 *  from a file, provides a mapping between features and unique integer
 *  identifiers of features, and provides a scoring function for items.
 */
class tSM
{
 public:
    tSM(grammar *, const char *fileName, const char *basePath);
    virtual ~tSM();
       
    /** Compute score for an item with respect to the model. */
    virtual double
    score(item *) = 0;

    /** Return grammar this model corresponds to. */
    inline grammar *
    G()
    { return _G; }

 protected:
    const char *
    fileName()
    { return _fileName; }    

 private:
    char *
    findFile(const char *fileName, const char *basePath);

    grammar *_G;
    char *_fileName;
};

/** Maximum Entropy Model.
 *  The model specifies features and their weights.
 *  A feature has the general form of a sequence of subfeatures. A subfeature
 *  is either a type / instance of the grammar, an integer, or a string.
 */
class tMEM : public tSM
{
 public:
    // _fix_me_
    // We'd prefer not to have to pass fileName and basePath here.
    // There should be a central placed to to this sort of thing.
    tMEM(grammar *G, const char *fileName, const char *basePath);
    virtual ~tMEM();

    virtual double
    score(item *);

 private:
    
    /// Number of features in the model.
    int _nfeatures;
    /// An array specifying the weight of each feature [0.._nfeatures[.
    double *_weights;
    /// Map between features and their identifiers [0.._nfeatures[.
    class tSMMap *_map;

    void
    readModel(const char *fileName);

    void
    parseModel();
};

#endif

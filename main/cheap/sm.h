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

#define SM_EXT ".sm"

class tSMFeature
{
 public:
    tSMFeature(const vector<int> &v)
        : _v(v)
    {}

    int
    hash() const;

    void
    print(FILE *f) const;
    
 private:
    vector<int> _v;
    
    friend int
    compare(const tSMFeature &f1, const tSMFeature &f2);
    friend bool
    operator<(const tSMFeature &, const tSMFeature&);
    friend bool
    operator==(const tSMFeature &, const tSMFeature&);
};

/** Stochastic Model.
 *  This class represents an abstract stochastic model. It reads the model
 *  from a file, provides a mapping between features and unique integer
 *  identifiers of features, and provides a scoring function for items.
 */
class tSM
{
 public:
    tSM(class grammar *, const char *fileName, const char *basePath);
    virtual ~tSM();
       
    /** Compute score for a feature with respect to the model. */
    virtual double
    score(const tSMFeature &) = 0;

    virtual double
    neutralScore() = 0;

    virtual double
    combineScores(double, double) = 0;

    /** Return grammar this model corresponds to. */
    inline class grammar *
    G()
    { return _G; }

 protected:
    const char *
    fileName()
    { return _fileName; }    

 private:
    char *
    findFile(const char *fileName, const char *basePath);

    class grammar *_G;
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
    tMEM(class grammar *G, const char *fileName, const char *basePath);
    virtual ~tMEM();

    virtual double
    score(const tSMFeature &);

    virtual double
    neutralScore()
    { return 0.0 ; }

    virtual double
    combineScores(double a, double b)
    { return a + b; }

 private:
    
    vector<double> _weights;
    class tSMMap *_map;

    void
    readModel(const char *fileName);

    void
    parseModel();

    void
    parseOptions();

    void
    parseFeatures(int);

    void
    parseFeature(int);
};

#endif

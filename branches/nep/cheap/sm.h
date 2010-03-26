/* -*- Mode: C++ -*- */
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

#include "types.h"
#include <list>
#include <vector>
#include <string>
#include <map>

#define SM_EXT ".sm"

/** A feature in a stochastic model. 
 *  This class represents one feature in a stochastic model. Features are
 *  tupels of integers. The tSMMap class efficiently maps features to
 *  integer codes.
 */
class tSMFeature
{
 public:
    tSMFeature(const std::vector<int> &v)
        : _v(v)
    {}

    int
    hash() const;

    void
    print(std::ostream &) const;
    
    int head_gp_0 () const {return _v.size() == 5 ? _v[2] : 0;}
    
    std::string bart_print () const; 
    
 private:
    std::vector<int> _v;
    
    friend int
    compare(const tSMFeature &f1, const tSMFeature &f2);
    friend bool
    operator<(const tSMFeature &, const tSMFeature&);
    friend bool
    operator==(const tSMFeature &, const tSMFeature&);
};

inline std::ostream &operator<<(std::ostream &o, const tSMFeature &f) {
  f.print(o); return o;
}

/** A stochastic model.
 *  This class represents an abstract stochastic model. A model can be
 *  instantiated from a file. The class provides methods to compute the
 *  score for a feature in the model, and to combine two scores.
 */
class tSM
{
 public:
    tSM(class tGrammar *, const char *fileName, const char *basePath);
    virtual ~tSM();
       
    /** Compute score for a feature with respect to the model. */
    virtual double
    score(const tSMFeature &) = 0;

    virtual double
    neutralScore() = 0;

    virtual double
    combineScores(double, double) = 0;

    /** Return grammar this model corresponds to. */
    inline class tGrammar *
    G()
    { return _G; }

    virtual double
    scoreLocalTree(class grammar_rule *, std::list<class tItem *>);
    virtual double
    scoreLocalTree(class grammar_rule *, std::vector<class tItem *>);

    virtual double
    scoreLeaf(class tLexItem *);
  
    /** Return the score for the hypothesis */
    virtual double 
    score_hypothesis(struct tHypothesis* hypo, std::list<tItem*> path,
                     int gplevel);

    /** Return the best predicted lexical (entry) types for the given
     * context.
     */
    virtual std::list<int>
    bestPredict(std::vector<std::string> words, std::vector<std::vector<int> > letypes, int n);

    class tSMMap *map()
    { return _map; }

    /** Return a description string suitable for printing.*/
    virtual std::string
    description() = 0;

 protected:
    const std::string &
    fileName() const 
    { return _fileName; }    

 private:
    class tGrammar *_G;
    std::string _fileName;

    class tSMMap *_map;
};

/** A Maximum Entropy model.
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
    tMEM(class tGrammar *G, const char *fileName, const char *basePath);
    virtual ~tMEM();

    virtual double
    score(const tSMFeature &);

    virtual double
    neutralScore()
    { return 0.0 ; }

    virtual double
    combineScores(double a, double b)
    { return a + b; }

    /** Return a description string suitable for printing.*/
    virtual std::string
    description();
    

 private:
    
    std::vector<double> _weights;

    /** Number of contexts this model was trained on. For reporting
        purposes only. */
    std::string _ctxts;

    /** Format version of the model:
        0: old format 
        1: new format (as of sep-06). 
        2: lexical type prediction model format
    */
    int _format;

    std::vector<int> _robust_rules; 

    void
    readModel(const std::string &fileName);

    void
    parseModel();

    void
    parseOptions();

    void
    parseFeatures(int);

    void
    parseFeature(int);

    /** Handler of new format for feature representation. */ 
    void parseFeature2(int); 

    /** Handler for lexical type prediction model */
    void parseFeature_lexpred(int);
};



/** Probablistic Context Free Grammar */
class tPCFG : public tSM
{
 public:
    // _fix_me_
    // We'd prefer not to have to pass fileName and basePath here.
    // There should be a central placed to to this sort of thing.
    tPCFG(class tGrammar *G, const char *fileName, const char *basePath);
    virtual ~tPCFG();

    virtual double
    score(const tSMFeature &);

    virtual double
    neutralScore() // TODO: this should not be used
    { return 0.0; }

    virtual double
    combineScores(double a, double b);

    /** Return a description string suitable for printing.*/
    virtual std::string
    description();
    
    virtual double
    scoreLocalTree(class grammar_rule *, std::list<class tItem *>);

    virtual double
    scoreLeaf(class tLexItem *);
  
    /** Return the score for the hypothesis */
    virtual double 
    score_hypothesis(struct tHypothesis* hypo, std::list<tItem*> path, int gplevel);
        

 private:
    
    bool _include_leafs;
    bool _use_preterminal_types;
    double _laplace_smoothing;
    double _min_logprob;

    // Initially, _weights contain the number of times this feature has been observed. 
    // AdjustWeights will then make sure Laplace is used for smoothing. 
    // The lisp code currently doesn't do any smoothing. 
    std::vector<double> _weights;
    std::map<type_t,int> _lhs_freq_counts;
    std::map<type_t,int> _lhs_rule_counts;
    
    void
    readModel(const std::string &fileName);

    void
    parseModel();

    void
    parseOptions();

    void
    parseFeatures(int);

    void
    parseFeature(int);
  
    void
    adjustWeights();

    double
    score(std::vector<type_t> rule);

};



/** Generative Model */
class tGM
{
 public:
    tGM(class tGrammar *G, const char *fileName, const char *basePath);
    virtual ~tGM();
    
    virtual double conditional (class grammar_rule *, std::vector<class tItem *>);
    virtual double conditional (std::vector<type_t>);
    virtual double unknown_conditional (type_t ruletype);
    virtual double prior (class grammar_rule *);
    
 private:

    // Indicates the delta in the Lidstone smoothing algorithm: (C(x) + delta) / ( N + delta*V).
    // Laplace is a special case of Lidstone, where delta is 1. 
    double _lidstone_delta;   

    std::map< std::vector<type_t>, int>    _counts; 
    std::map< std::vector<type_t>, double> _weights; 
    std::map<type_t,double> _unknown_conditionals;
    std::map<type_t,int>    _prior_counts;
    std::map<type_t,double> _prior_weights;
    int _total_count;                          // Counts how many training instance were seen. 
    double _unknown_prior;
    std::map<type_t,int>    _lhs_counts;       // Counts how many rules exist with this lhs. 

    void calculateWeights ();

    // Functions for parsing the data file. 
    void readModel(const std::string &fileName);
    void parseModel();
    void parseOptions();
    void parseFeatures(int);
    void parseFeature(int);
  
};

#endif

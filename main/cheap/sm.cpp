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

#include "sm.h"
#include "hash.h"
#include "pet-system.h"
#include "lex-tdl.h"
#include "utility.h"
#include "settings.h"
#include "options.h"
#include "grammar.h"
#include "item.h"

int
tSMFeature::hash() const
{
    return ::bjhash2(_v, 0);
}

void
tSMFeature::print(FILE *f) const
{
    for(vector<int>::const_iterator it = _v.begin();
        it != _v.end(); ++it)
    {
        fprintf(f, "%d ", *it);
    }
}

inline int
compare(const tSMFeature &f1, const tSMFeature &f2)
{
    unsigned int i = 0;
    while(i < f1._v.size() && i < f2._v.size())
    {
        if(f1._v[i] < f2._v[i])
            return -1;
        else if(f1._v[i] > f2._v[i])
            return +1;
        i++;
    }
    if(f1._v.size() < f2._v.size())
        return -1;
    else if(f1._v.size() > f2._v.size())
        return +1;
    else
        return 0;
}

inline bool
operator<(const tSMFeature& a, const tSMFeature &b)
{ return compare(a, b) == -1; }

inline bool
operator==(const tSMFeature& a, const tSMFeature &b)
{ return compare(a, b) == 0; }

namespace HASH_SPACE {
    template<> struct hash<tSMFeature>
    {
        inline size_t operator()(const tSMFeature &key) const
        {
            return key.hash();
        }
    };

    template<> struct hash<string>
    {
        inline size_t operator()(const string &key) const
        {
            return ::bjhash((const ub1 *) key.data(), key.size(), 0);
        }
    };
}

/** Maintains a mapping between features (instances of tSMFeature) and codes.
 *  Also maintains map from types, integers and strings to integers.
 */
class tSMMap
{
 public:
    tSMMap()
        : _n(0), _next_subfeature(INT_MIN)
    {};

    int
    featureToCode(const tSMFeature &feature);

    tSMFeature
    codeToFeature(int code) const;

    inline int
    typeToSubfeature(type_t t)
    { return t; }

    inline int
    intToSubfeature(int i)
    { return i; }

    int
    stringToSubfeature(const string &);

 private:
    /* Mapping between codes and features */
    int _n;
    vector<tSMFeature> _codeToFeature;
    hash_map<tSMFeature, int> _featureToCode;

    /* Subfeature mapping */
    int _next_subfeature;
    hash_map<string, int> _stringToSubfeature;
};

int
tSMMap::featureToCode(const tSMFeature &feature)
{
    if(verbosity > 9)
    {
        fprintf(fstatus, "featureToCode(");
        feature.print(fstatus);
        fprintf(fstatus, ") -> ");
    }

    hash_map<tSMFeature, int>::iterator itMatch = _featureToCode.find(feature);
    if(itMatch != _featureToCode.end())
    {
        if(verbosity > 9)
            fprintf(fstatus, "%d\n", itMatch->second);
        return itMatch->second;
    }
    else
    {
        if(verbosity > 9)
            fprintf(fstatus, "added %d", _n);
        _codeToFeature.push_back(feature);
        return _featureToCode[feature] = _n++;
    }
}

tSMFeature
tSMMap::codeToFeature(int code) const
{
    if(code >= 0 && code < _n)
        return _codeToFeature[code];
    else
        return vector<int>();
}

int
tSMMap::stringToSubfeature(const string &s)
{
    hash_map<string, int>::iterator itMatch = _stringToSubfeature.find(s);
    if(itMatch != _stringToSubfeature.end())
    {
        return itMatch->second;
    }
    else
    {
        return _stringToSubfeature[s] = _next_subfeature++;
    }
}

tSM::tSM(grammar *G, const char *fileName, const char *basePath)
    : _G(G), _fileName(0), _map(0)
{
    _fileName = findFile(fileName, basePath);
    if(!_fileName)
      throw error(string("Could not open SM file \"") 
                  + fileName + string("\""));
    _map = new tSMMap();
}

tSM::~tSM()
{
    if(_fileName)
        free(_fileName);
    delete _map;
}

char *
tSM::findFile(const char *fileName, const char *basePath)
{
    // _fix_me_
    // This piece of uglyness (copied from settings::settings())
    // should be factored out and encapsulated.

    char *res = 0;

    if(basePath)
    {
        char *slash = strrchr((char *) basePath, '/');
        char *prefix = (char *) malloc(strlen(basePath) + 1
                                       + strlen(SET_SUBDIRECTORY) + 1);
        if(slash)
	{
            strncpy(prefix, basePath, slash - basePath + 1);
            prefix[slash - basePath + 1] = '\0';
	}
        else
	{
            strcpy(prefix, "");
	}

        char *fname = (char *) malloc(strlen(prefix) + 1
                                      + strlen(fileName) + 1);
        strcpy(fname, prefix);
        strcat(fname, fileName);
      
        res = find_file(fname, SM_EXT, false);

        free(prefix);
    }
    else
    {
        res = (char *) malloc(strlen(fileName) + 1);
        strcpy(res, fileName);
    }
  
    return res;
}

double
tSM::scoreLocalTree(grammar_rule *R, list<item *> dtrs)
{
    vector<int> v;
    v.push_back(map()->intToSubfeature((unsigned) R->arity() == dtrs.size() ?
                                       1 : 2));
    v.push_back(map()->typeToSubfeature(R->type()));

    double total = neutralScore();

    for(list<item *>::iterator dtr = dtrs.begin();
        dtr != dtrs.end(); ++dtr)
    {
        v.push_back((*dtr)->identity());
        total = combineScores(total, (*dtr)->score());
    }

    tSMFeature f(v);

    return combineScores(total, score(f));
}

double
tSM::scoreLeaf(lex_item *it)
{
    vector<int> v;
    v.push_back(map()->intToSubfeature(1));
    v.push_back(map()->typeToSubfeature(it->type()));

    return score(tSMFeature(v));
}

tMEM::tMEM(grammar *G, const char *fileNameIn, const char *basePath)
    : tSM(G, fileNameIn, basePath)
{
    readModel(fileName());
}

tMEM::~tMEM()
{
}

string
tMEM::description()
{
    ostringstream desc;
    desc << "MEM[" << string(fileName()) << "] "
         << _weights.size() << "/" << _ctxts;

    return desc.str();
}

void
tMEM::readModel(const char *fileName)
{
    push_file(fileName, "reading ME model");
    char *sv = lexer_idchars;
    lexer_idchars = "_+-*?$";
    parseModel();
    lexer_idchars = sv;
}

void
tMEM::parseModel()
{
    /* The model file consists of a header and the actual weights. Example:

         ;; This is a model with 3 features built from 42 contexts.
         :begin :mem 42.
         *maxent-frequency-threshold* := 10.
         :begin :features 3.
         [2 hspec hcomp] 0.1556260000
         [1 hcomp p_temp_le hspec] -1.8462600000
         [2 hcomp p_temp_le] 0.7750630000
         :end :features.
         :end :mem.

    */

    char *tmp;

    match(T_COLON, "`:' before keyword", true);
    match_keyword("begin");
    match(T_COLON, "`:' before keyword", true);
    tmp = match(T_ID, "mem", false);
    if(strcmp(tmp, "mem") != 0)
    {
        syntax_error("expecting `mem' section", LA(0));
    }
    free(tmp);
    tmp = match(T_ID, "number of contexts", false);
    _ctxts = string(tmp);
    free(tmp);
    match(T_DOT, "`.' after section opening", true);
    
    parseOptions();

    match(T_COLON, "`:' before keyword", true);
    match_keyword("begin");
    match(T_COLON, "`:' before keyword", true);
    tmp = match(T_ID, "features", false);
    if(strcmp(tmp, "features") != 0)
    {
        syntax_error("expecting `features' section", LA(0));
    }
    free(tmp);
    tmp = match(T_ID, "number of features", false);
    int nFeatures = strtoint(tmp, "as number of features in MEM");
    free(tmp);
    match(T_DOT, "`.' after section opening", true);

    parseFeatures(nFeatures);

    match(T_COLON, "`:' before keyword", true);
    match_keyword("end");
    match(T_COLON, "`:' before keyword", true);
    tmp = match(T_ID, "features", false);
    if(strcmp(tmp, "features") != 0)
    {
        syntax_error("expecting end of `features' section", LA(0));
    }
    free(tmp);
    match(T_DOT, "`.' after section end", true);

    match(T_COLON, "`:' before keyword", true);
    match_keyword("end");
    match(T_COLON, "`:' before keyword", true);
    tmp = match(T_ID, "mem", false);
    if(strcmp(tmp, "mem") != 0)
    {
        syntax_error("expecting end of `mem' section", LA(0));
    }
    free(tmp);
    match(T_DOT, "`.' after section end", true);
}

void
tMEM::parseOptions()
{
    // _fix_me_
    // actually parse this

    // for now we just skip until we get a `:' `begin'

    while(LA(0)->tag != T_EOF)
    {
        if(LA(0)->tag == T_COLON &&
           is_keyword(LA(1), "begin"))
            break;
        consume(1);
    }
}

void
tMEM::parseFeatures(int nFeatures)
{
    fprintf(fstatus, "[%d features] ", nFeatures);

    _weights.resize(nFeatures);

    int n = 0;
    while(LA(0)->tag != T_EOF)
    {
        if(LA(0)->tag == T_COLON &&
           is_keyword(LA(1), "end"))
            break;

        parseFeature(n++);
    }
}

void
tMEM::parseFeature(int n)
{
    char *tmp;
    if(verbosity > 4)
        fprintf(fstatus, "\n[%d]", n);

    match(T_LBRACKET, "begin of feature vector", true);

    // Vector of subfeatures.
    vector<int> v;

    bool good = true;
    while(LA(0)->tag != T_RBRACKET && LA(0)->tag != T_EOF)
    {
        if(LA(0)->tag == T_ID)
        {
            // This can be an integer or an identifier.
            tmp = match(T_ID, "subfeature in feature vector", false);
            if(verbosity > 4)
                fprintf(fstatus, " %s", tmp);

            char *endptr;
            int t = strtol(tmp, &endptr, 10);
            if(endptr == 0 || *endptr != 0)
            {
                // This is not an integer, so it must be a type/instance.
                char *inst = (char *) malloc(strlen(tmp)+2);
                strcpy(inst, "$");
                strcat(inst, tmp);
                t = lookup_type(inst);
                if(t == -1)
                    t = lookup_type(inst+1);
                free(inst);
                
                if(t == -1)
                {
                    fprintf(ferr, "Unknown type/instance `%s' in feature #%d\n",
                            tmp, n);
                    good = false;
                }
                else
                {
                    v.push_back(map()->typeToSubfeature(t));
                }
            }
            else
            {
                // This is an integer.
                v.push_back(map()->intToSubfeature(t));
            }
            free(tmp);
        }
        else if(LA(0)->tag == T_STRING)
        {
            tmp = match(T_STRING, "subfeature in feature vector", false);
            if(verbosity > 4)
                fprintf(fstatus, " \"%s\"", tmp);
            v.push_back(map()->stringToSubfeature(string(tmp)));
            free(tmp);
        }
    }

    match(T_RBRACKET, "end of feature vector", true);
    
    tmp = match(T_ID, "feature weight", false);
    // _fix_me_
    // check syntax of number
    double w = strtod(tmp, NULL);
    free(tmp);
    if(verbosity > 4)
        fprintf(fstatus, ": %g", w);

    if(good)
    {
        int code = map()->featureToCode(v);
        if(verbosity > 4)
            fprintf(fstatus, " (code %d)\n", code);
        if(code >= (int) _weights.size()) _weights.resize(code);
        _weights[code] = w;
    }
}

double
tMEM::score(const tSMFeature &f)
{
    int code = map()->featureToCode(f);
    if(code < (int) _weights.size())
        return _weights[code];
    else
        return 0.0;
}

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

class tSMFeature
{
 public:
    tSMFeature(const vector<int> &v)
        : _v(v)
    {}

    int
    hash() const;
    
 private:
    vector<int> _v;
    
    friend int
    compare(const tSMFeature &f1, const tSMFeature &f2);
    friend bool
    operator<(const tSMFeature &, const tSMFeature&);
    friend bool
    operator==(const tSMFeature &, const tSMFeature&);
};

int
tSMFeature::hash() const
{
    int k = 0;
    for(vector<int>::const_iterator it = _v.begin();
        it != _v.end(); ++it)
    {
        k += *it;
    }

    return k;
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
}

/** Maintains a mapping between symbols (features) and codes.
 *  
 */
class tSMMap
{
 public:
    tSMMap()
        : _n(0)
    {};

    int
    symbolToCode(const tSMFeature &symbol);

    tSMFeature
    codeToSymbol(int code) const;

 private:
    int _n;
    vector<tSMFeature> _codeToSymbol;
    hash_map<tSMFeature, int> _symbolToCode;
};

int
tSMMap::symbolToCode(const tSMFeature &symbol)
{
    hash_map<tSMFeature, int>::iterator itMatch = _symbolToCode.find(symbol);
    if(itMatch != _symbolToCode.end())
    {
        return itMatch->second;
    }
    else
    {
        _codeToSymbol.push_back(symbol);
        return _symbolToCode[symbol] = _n++;
    }
}

tSMFeature
tSMMap::codeToSymbol(int code) const
{
    if(code >= 0 && code < _n)
        return _codeToSymbol[code];
    else
        return vector<int>();
}


tSM::tSM(grammar *G, const char *fileName, const char *basePath)
    : _G(G), _fileName(0)
{
    _fileName = findFile(fileName, basePath);
}

tSM::~tSM()
{
    if(_fileName)
        free(_fileName);
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

tMEM::tMEM(grammar *G, const char *fileNameIn, const char *basePath)
    : tSM(G, fileNameIn, basePath), _map(0)
{
    _map = new tSMMap();
    readModel(fileName());
}

tMEM::~tMEM()
{
    delete _map;
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
         :end :features.
         [2 hspec hcomp] 0.1556260000
         [1 hcomp p_temp_le hspec] -1.8462600000
         [2 hcomp p_temp_le] 0.7750630000
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
    match(T_ID, "number of contexts", true);
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
    if(verbosity > 4)
        fprintf(fstatus, "\n[%d]", n);

    match(T_LBRACKET, "begin of feature vector", true);

    vector<int> v;
    char *tmp;
    bool good = true;
    while(LA(0)->tag != T_RBRACKET && LA(0)->tag != T_EOF)
    {
        tmp = match(T_ID, "feature vector", false);
        if(verbosity > 4)
            fprintf(fstatus, " %s", tmp);
        
        char *endptr;
        int t = strtol(tmp, &endptr, 10);
        if(endptr == 0 || *endptr != 0)
        {
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
        }
        v.push_back(t);

        free(tmp);
    }

    match(T_RBRACKET, "end of feature vector", true);
    
    tmp = match(T_ID, "feature weight", false);
    // _fix_me_
    // check syntax of number
    double w = strtod(tmp, NULL);
    if(verbosity > 4)
        fprintf(fstatus, ": %g", w);

    if(good)
    {
        int code = _map->symbolToCode(v);
        if(verbosity > 4)
            fprintf(fstatus, " (code %d)\n", code);
    }

    free(tmp);
}

double
tMEM::score(item *it)
{
    return 0.0;
}

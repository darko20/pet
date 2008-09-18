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
#include "hashing.h"
#include "lex-tdl.h"
#include "utility.h"
#include "settings.h"
#include "options.h"
#include "grammar.h"
#include "item.h"
#include <sstream>
#include <float.h>

using namespace std;
using namespace HASH_SPACE;

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
    hash_map<string, int, bj_string_hash> _stringToSubfeature;
};

int
tSMMap::featureToCode(const tSMFeature &feature)
{
    if(verbosity > 14)
    {
        fprintf(fstatus, "featureToCode(");
        feature.print(fstatus);
        fprintf(fstatus, ") -> ");
    }

    hash_map<tSMFeature, int>::iterator itMatch = _featureToCode.find(feature);
    if(itMatch != _featureToCode.end())
    {
        if(verbosity > 14)
            fprintf(fstatus, "%d\n", itMatch->second);
        return itMatch->second;
    }
    else
    {
        if(verbosity > 14)
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
    hash_map<string, int, bj_string_hash>::iterator itMatch =
      _stringToSubfeature.find(s);
    if(itMatch != _stringToSubfeature.end())
    {
        return itMatch->second;
    }
    else
    {
        return _stringToSubfeature[s] = _next_subfeature++;
    }
}

tSM::tSM(tGrammar *G, const char *fileName, const char *basePath)
    : _G(G), _map(0) {
  _fileName = find_file(fileName, SM_EXT, basePath);
  if(_fileName.empty())
    throw tError(string("Could not open SM file \"") + fileName + "\"");
  _map = new tSMMap();
}

tSM::~tSM() {
  delete _map;
}

/*
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
      
        res = find_file(fname, SM_EXT);

        free(prefix);
    }
    else
    {
        res = (char *) malloc(strlen(fileName) + 1);
        strcpy(res, fileName);
    }
  
    return res;
}
*/

double
tSM::scoreLocalTree(grammar_rule *R, vector<tItem *> dtrs)
{
  vector<int> v1, v2;
  v1.push_back(map()->intToSubfeature(1));
  v2.push_back(map()->intToSubfeature(2));
  v1.push_back(map()->intToSubfeature(0));
  v2.push_back(map()->intToSubfeature(0));
  v1.push_back(map()->typeToSubfeature(R->type()));
  v2.push_back(map()->typeToSubfeature(R->type()));
  double total = neutralScore();

  for(vector<tItem *>::iterator dtr = dtrs.begin();
      dtr != dtrs.end(); ++dtr)
    {
      v1.push_back((*dtr)->identity());
      total = combineScores(total, (*dtr)->score());
    }

  v2.push_back(dtrs[R->nextarg()-1]->identity());
  total = combineScores(total, score(tSMFeature(v1)));

  if (R->arity() > 1) 
    total = combineScores(total, score(tSMFeature(v2)));

  return total;
}

double
tSM::scoreLocalTree(grammar_rule *R, list<tItem *> dtrs)
{
  vector<int> v1, v2;
  v1.push_back(map()->intToSubfeature(1));
  v2.push_back(map()->intToSubfeature(2));
  v1.push_back(map()->intToSubfeature(0));
  v2.push_back(map()->intToSubfeature(0));
  v1.push_back(map()->typeToSubfeature(R->type()));
  v2.push_back(map()->typeToSubfeature(R->type()));
  double total = neutralScore();
  int i = 1;
  for(list<tItem *>::iterator dtr = dtrs.begin();
      dtr != dtrs.end(); ++dtr, i++)
    {
      v1.push_back((*dtr)->identity());
      total = combineScores(total, (*dtr)->score());
      if (i == R->nextarg())
        v2.push_back((*dtr)->identity());
    }

  total = combineScores(total, score(tSMFeature(v1)));

  if (R->arity() > 1) 
    total = combineScores(total, score(tSMFeature(v2)));

  return total;
}


double
tSM::scoreLeaf(tLexItem *it)
{
    vector<int> v;
    v.push_back(map()->intToSubfeature(1));
    v.push_back(map()->intToSubfeature(0));
    v.push_back(map()->typeToSubfeature(it->identity()));
    v.push_back(map()->stringToSubfeature(it->orth()));

    return score(tSMFeature(v));
}

double
tSM::score_hypothesis(tHypothesis* hypo, list<tItem*> path, int gplevel)
{
  vector<int> v1, v2;
  double total = neutralScore();
  int level = path.size();
  if (level > gplevel)  // we can only those levels we have ancestors for
    level = gplevel;
  // collect grand-parenting features
  for (int i = level; i >= 0; i -- ) {
    v1.clear();
    v2.clear();

    v1.push_back(map()->intToSubfeature(1));
    v2.push_back(map()->intToSubfeature(2));

    v1.push_back(map()->intToSubfeature(i));
    v2.push_back(map()->intToSubfeature(i));
    // push down appropriate number of ancestors
    unsigned int j = path.size();
    for (list<tItem*>::iterator gp = path.begin();
         gp != path.end(); gp ++, j --)
      if (j <= (unsigned int)i) {
        if (*gp == NULL) {
          v1.push_back(INT_MAX);
          v2.push_back(INT_MAX);
        }
        else {
          v1.push_back((*gp)->identity());
          v2.push_back((*gp)->identity());
        }
      }
    
    if (hypo->edge->rule() == NULL) { // tLexItem
      // push down the lexical type and orth
      tLexItem *lex = (tLexItem*)hypo->edge;
      v1.push_back(map()->typeToSubfeature(lex->identity()));
      v1.push_back(map()->stringToSubfeature(lex->orth()));
    } else { // tPhrasalItem
      tPhrasalItem *phrase = (tPhrasalItem*)hypo->edge;
      v1.push_back(map()->typeToSubfeature(phrase->identity()));
      v2.push_back(map()->typeToSubfeature(phrase->identity()));
      int key = phrase->rule()->nextarg();
      list<tItem*> newpath = path;
      newpath.push_back(hypo->edge);
      if (newpath.size() > gplevel)
        newpath.pop_front();
      for (list<tHypothesis*>::iterator hypo_dtr = hypo->hypo_dtrs.begin();
           hypo_dtr != hypo->hypo_dtrs.end(); hypo_dtr++) {
        v1.push_back((*hypo_dtr)->edge->identity());
        if (i == 0) { // combine the scores of daughters only once
          (*hypo_dtr)->scores.size(); //debug
          if ((*hypo_dtr)->scores.find(newpath) == (*hypo_dtr)->scores.end()) 
            score_hypothesis(*hypo_dtr, newpath, gplevel);
          total = combineScores(total, (*hypo_dtr)->scores[newpath]);
        }
        if (--key == 0) 
          v2.push_back((*hypo_dtr)->edge->identity());

      }
      if (phrase->rule()->arity() > 1) {
        total = combineScores(total, score(tSMFeature(v2)));
      }
    }
    total = combineScores(total, score(tSMFeature(v1)));
  }
  hypo->scores[path] = total;
  return hypo->scores[path];
}

tMEM::tMEM(tGrammar *G, const char *fileNameIn, const char *basePath)
  : tSM(G, fileNameIn, basePath), _format(0)
{
    readModel(fileName());
}

tMEM::~tMEM()
{
}

string
tMEM::description()
{
    std::ostringstream desc;
    desc << "MEM[" << string(fileName()) << "] "
         << _weights.size() << "/" << _ctxts;

    return desc.str();
}

void
tMEM::readModel(const string &fileName)
{
    push_file(fileName, "reading ME model");
    const char *sv = lexer_idchars;
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
    tmp = match(T_ID, "mem|model", false);
    if(strcmp(tmp, "mem") == 0) 
      _format = 0;
    else if (strcmp(tmp, "model") == 0)
      _format = 1;
    else if (strcmp(tmp, "lexmem") == 0)
      _format = 2;
    else 
        syntax_error("expecting `mem|model' section", LA(0));

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
    tmp = match(T_ID, "mem|model", false);
    if(strcmp(tmp, "mem") != 0 && strcmp(tmp, "model") != 0)
    {
        syntax_error("expecting end of `mem|model' section", LA(0));
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
  char * pname;
  char * pvalue;
  while(LA(0)->tag != T_EOF) {
    if(LA(0)->tag == T_COLON &&
       is_keyword(LA(1), "begin"))
      break;
    if (LA(0)->tag == T_ID) {
      pname = match(T_ID, "parameter name", false);
      if(!strcmp(pname, "*feature-grandparenting*")) {
        match(T_ISEQ, "iseq after parameter name", true);
        if (LA(0)->tag == T_COLON) consume(1);
        pvalue = match(T_ID, "parameter value", false);
        match(T_DOT, "dot after parameter value", true);
        set_opt("opt_gplevel", (unsigned int) atoi(pvalue));
        free(pvalue);
      }
      else {
        while(LA(0)->tag != T_DOT) consume(1);
        consume(1);
      }
      free(pname);
    }    
    else 
      consume(1);
  }
}

void
tMEM::parseFeatures(int nFeatures)
{
    LOG(logSM, INFO, "[" << nFeatures << " features] ");

    _weights.resize(nFeatures);

    int n = 0;
    while(LA(0)->tag != T_EOF)
    {
        if(LA(0)->tag == T_COLON &&
           is_keyword(LA(1), "end"))
            break;

        switch (_format) {
        case 0:  // old format
          parseFeature(n++);
          break;
        case 1:  // new format (as of sep-2006)
          parseFeature2(n++);
          break;
        case 2:  // lexical type prediction model
          parseFeature_lexpred(n++);
          break;
        }
    }
}

void
tMEM::parseFeature(int n)
{
    char *tmp;

    LOG(logSM, DEBUG, "[" << n << "]");

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
            LOG(logSM, DEBUG, " " << tmp);

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
                  LOG(logSM, WARN, "Unknown type/instance `" << tmp
                      << "' in feature #" << n);
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
                // Set the level of grand-parenting to 0 to be compatible with 
                // the new format
                v.push_back(map()->intToSubfeature(0));
            }
            free(tmp);
        }
        else if(LA(0)->tag == T_STRING)
        {
            tmp = match(T_STRING, "subfeature in feature vector", false);
            LOG(logSM, DEBUG, " \"" << tmp << "\"");
            v.push_back(map()->stringToSubfeature(string(tmp)));
            free(tmp);
        }
        else if (LA(0)->tag == T_LPAREN || LA(0)->tag == T_RPAREN) {
          consume(1);
        }
        else
        {
            syntax_error("expecting subfeature", LA(0));
            consume(1);
        }
    }

    match(T_RBRACKET, "end of feature vector", true);
    
    tmp = match(T_ID, "feature weight", false);
    // _fix_me_
    // check syntax of number
    double w = strtod(tmp, NULL);
    free(tmp);
    LOG(logSM, DEBUG, ": " << w);

    if(good)
    {
        int code = map()->featureToCode(v);
        LOG(logSM, DEBUG, " (code " << code << ")");
        assert(code >= 0);
        if(code >= (int) _weights.size()) _weights.resize(code + 1);
        _weights[code] = w;
    }
}


void
tMEM::parseFeature2(int n)
{
    char *tmp;
    LOG(logSM, DEBUG, "[" << n << "]");

    match(T_LPAREN, "begin of feature", true);
    match(T_ID, "feature index", true);
    match(T_RPAREN, "after feature index", true);
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
            LOG(logSM, DEBUG, " " << tmp);

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
                  LOG(logSM, INFO, "Unknown type/instance `" << tmp
                      << "' in feature #" << n);
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
            LOG(logSM, DEBUG, " \"" << tmp << "\"");
            v.push_back(map()->stringToSubfeature(string(tmp)));
            free(tmp);
        }
        else if (LA(0)->tag == T_CAP) {
          // This is a special 
          consume(1);
          v.push_back(INT_MAX);
        }
        else if (LA(0)->tag == T_DOLLAR) {
          consume(1);
          v.push_back(INT_MAX);
        }
        else if (LA(0)->tag == T_LPAREN || LA(0)->tag == T_RPAREN) {
          consume(1);
        }
        else
        {
            syntax_error("expecting subfeature", LA(0));
            consume(1);
        }
    }

    match(T_RBRACKET, "end of feature vector", true);
    
    tmp = match(T_ID, "feature weight", false);
    // _fix_me_
    // check syntax of number
    double w = strtod(tmp, NULL);
    free(tmp);
    LOG(logSM, DEBUG, ": " << w);

    if(good)
    {
        int code = map()->featureToCode(v);
        LOG(logSM, DEBUG, " (code " << code << ")");
        assert(code >= 0);
        if(code >= (int) _weights.size()) _weights.resize(code + 1);
        _weights[code] = w;
    }

    //skip the rest part of the feature
    while (LA(0)->tag != T_LPAREN && LA(0)->tag != T_COLON &&  LA(0)->tag != T_EOF)
      consume(1);
}

double
tMEM::score(const tSMFeature &f)
{
    int code = map()->featureToCode(f);
    assert(code >=0);
    if(code < (int) _weights.size())
        return _weights[code];
    else
        return 0.0;
}

void
tMEM::parseFeature_lexpred(int n)
{
    char *tmp;

    if(verbosity > 9)
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
            if(verbosity > 9)
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
            if(verbosity > 9)
                fprintf(fstatus, " \"%s\"", tmp);
            v.push_back(map()->stringToSubfeature(string(tmp)));
            free(tmp);
        }
        else if (LA(0)->tag == T_CAP) {
          // This is a special 
          consume(1);
          v.push_back(INT_MAX);
        }
        else if (LA(0)->tag == T_LPAREN || LA(0)->tag == T_RPAREN) {
          consume(1);
        }
        else
        {
            syntax_error("expecting subfeature", LA(0));
            consume(1);
        }
    }

    match(T_RBRACKET, "end of feature vector", true);
    
    tmp = match(T_ID, "feature weight", false);
    // _fix_me_
    // check syntax of number
    double w = strtod(tmp, NULL);
    free(tmp);
    if(verbosity > 9)
        fprintf(fstatus, ": %g", w);

    if(good)
    {
        int code = map()->featureToCode(v);
        if(verbosity > 9)
            fprintf(fstatus, " (code %d)\n", code);
        assert(code >= 0);
        if(code >= (int) _weights.size()) _weights.resize(code + 1);
        _weights[code] = w;
    }
}


string normstr(string str) {
  string newstr;
  for (unsigned int i = 0; i < str.length(); i ++) {
    if (islower(str[i]) || isdigit(str[i]) ||
        str[i] == '-' || str[i] == '_' ||
        str[i] == ' ' || str[i] == '\'')
      newstr += str[i];
    else if (isupper(str[i]))
      newstr += tolower(str[i]);
  }
  return newstr;
}

std::list<int>
tSM::bestPredict(std::vector<string> words, std::vector<std::vector<int> > letypes, int n) {
  // for each output, create the features and accumulate the values
  //double max = DBL_MIN; // _fix_me DBL_MIN?
  list<double> scores(n);
  for (list<double>::iterator it = scores.begin();
       it != scores.end(); it ++) {
    *it = DBL_MIN;
  }
  list<type_t> types(n);
  //  type_t type;
  list_int* outputs = G()->predicts();

  for (; outputs != 0; outputs = rest(outputs)) {
    type_t output = first(outputs);
    vector<int> v;
    double total = neutralScore();
    string lword = normstr(words[4]);

    //0: has digit? 0:1
    v.push_back(map()->typeToSubfeature(output));
    v.push_back(map()->intToSubfeature(0));
    int dc = 0;
    for (unsigned int p = 0; p < words[4].length(); p ++)
      if (isdigit(words[4][p])) {
        dc = 1;
        break;
      }
    v.push_back(map()->intToSubfeature(dc));
    total = combineScores(total, score(tSMFeature(v)));
    v.clear();

    //1: has uppercase? 0:1
    v.push_back(map()->typeToSubfeature(output));
    v.push_back(map()->intToSubfeature(1));
    int uc = 0;
    for (unsigned int p = 0; p < words[4].length(); p ++)
      if (isupper(words[4][p])) {
        uc = 1;
        break;
      }
    v.push_back(map()->intToSubfeature(uc));
    total = combineScores(total, score(tSMFeature(v)));
    v.clear();

    //2: with space or hypen? 0:1
    v.push_back(map()->typeToSubfeature(output));
    v.push_back(map()->intToSubfeature(2));
    if (words[4].find(' ') != string::npos || 
        words[4].find('-') != string::npos)
      v.push_back(map()->intToSubfeature(1));
    else 
      v.push_back(map()->intToSubfeature(0));
    total = combineScores(total, score(tSMFeature(v)));
    v.clear();

    //3: prefix len str
    for (unsigned int i = 1; i < 3; i ++) {
      v.push_back(map()->typeToSubfeature(output));
      v.push_back(map()->intToSubfeature(3));
      v.push_back(map()->intToSubfeature(i));
      if (lword.length() >= i) {
        v.push_back(map()->stringToSubfeature(lword.substr(0,i)));
      } else {
        v.push_back(map()->stringToSubfeature(string("_")));
      }
      total = combineScores(total, score(tSMFeature(v)));
      v.clear();
    }

    //4: suffix len str
    for (unsigned int i = 1; i < 3; i ++) {
      v.push_back(map()->typeToSubfeature(output));
      v.push_back(map()->intToSubfeature(4));
      v.push_back(map()->intToSubfeature(i));
      if (lword.length() >= i) {
        v.push_back(map()->stringToSubfeature(lword.substr(lword.length()-i,i)));
      } else {
        v.push_back(map()->stringToSubfeature(string("_")));
      }
      total = combineScores(total, score(tSMFeature(v)));
      v.clear();
    }

    //5: context word features
    for (int i = 0; i < 4; i ++) {
      string word;
      if (words[i].empty())
        word = "_";
      else
        word = normstr(words[i]);
      // _fix_me: transform(words[i].begin(),words[i].end(),word.begin(), tolower);
      v.push_back(map()->typeToSubfeature(output));
      v.push_back(map()->intToSubfeature(5));
      v.push_back(map()->intToSubfeature(i));
      v.push_back(map()->stringToSubfeature(word));
      total = combineScores(total, score(tSMFeature(v)));
      v.clear();
    }
    //6: context type features
    for (int i = 0; i < 4; i ++) {
      if (letypes[i].empty()) {
        v.push_back(map()->typeToSubfeature(output));
        v.push_back(map()->intToSubfeature(6));
        v.push_back(map()->intToSubfeature(i));
        v.push_back(INT_MAX);
        total = combineScores(total, score(tSMFeature(v)));
        v.clear();
      } else {
        for (vector<type_t>::iterator it = letypes[i].begin();
             it != letypes[i].end(); it ++) {
          v.push_back(map()->typeToSubfeature(output));
          v.push_back(map()->intToSubfeature(6));
          v.push_back(map()->intToSubfeature(i));       
          v.push_back(map()->typeToSubfeature(*it));
          total = combineScores(total, score(tSMFeature(v)));
          v.clear();
        }
      }
    }
    list<type_t>::iterator tit = types.begin();
    for (list<double>::iterator sit = scores.begin();
         sit != scores.end() && tit != types.end(); sit ++, tit ++ ) {
      if (total > *sit) {
        scores.insert(sit, 1, total);
        types.insert(tit, 1, output);
        scores.resize(n);
        types.resize(n);
        break;
      }
    }
    //    if (total > max) {
    //      max = total;
    //      type = output;
    //    }
  }
  //  return type;
  return types;
}

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


/** Maintains a mapping between features and feature ids.
 *  
 */
class tSMMap
{


};

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
    : tSM(G, fileNameIn, basePath)
{
    readModel(fileName());
}

tMEM::~tMEM()
{
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
    fprintf(fstatus, "\n[%d]", n);

    match(T_LBRACKET, "begin of feature vector", true);
 
    char *tmp;
    while(LA(0)->tag != T_RBRACKET && LA(0)->tag != T_EOF)
    {
        tmp = match(T_ID, "feature vector", false);
        fprintf(fstatus, " %s", tmp);
        free(tmp);
    }

    match(T_RBRACKET, "end of feature vector", true);
    
    tmp = match(T_ID, "feature weight", false);
    fprintf(fstatus, ": %s", tmp);
    free(tmp);
}

double
tMEM::score(item *it)
{
    return 0.0;
}

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
    do
    {
        if(LA(0)->tag == T_EOF)
        {
            fprintf(fstatus, "*EOF*\n");
        }
        else
        {
            fprintf(fstatus, "[%d]<%s>\n", LA(0)->tag, LA(0)->text);
        }

        if(LA(0)->tag != T_EOF)
            consume(1);
    } while(LA(0)->tag != T_EOF);
    consume(1);
}

double
tMEM::score(item *it)
{
    return 0.0;
}

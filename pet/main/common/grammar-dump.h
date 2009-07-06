/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
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

/** \file grammar-dump.h
 * Dumped form of a grammar
 * A grammar file consists of a header, a table of contents, and a 
 * number of sections in arbitrary order
 *
 * version history: \par
 *
 * - 1 -> 2 : introduced featset information for each type (2-nov-99)
 * - 2 -> 3 : introduced new field headdtr to rule_dump (5-nov-99)
 * - 3 -> 4 : new: feattable info after featset info (15-nov-99)
 * - 4 -> 5 : unexpanded (qc) structures now have a negative type value
 *             (20-jan-00)
 * - 5 -> 6 : also dump appropriate types for attributes (20-jan-00)
 * - 6 -> 7 : dump number of bytes wasted per type by encoding (9-feb-00)
 * - 7 -> 8 : dump dags depth first, root is now last node in dump (28-may-00)
 * - 8 -> 9 : major cleanup - byteorder independence, bitcode compression
 *             (12-jun-00)
 * - 9 -> 10 : major cleanup, part II (13-jun-00)
 * - 10 -> 11 : specify encoding type
 * - 11 -> 12 : encode types as int rather than short in some places
 * - 12 -> 13 : major cleanup. no more instances and symbols, store status
 *              value for all types instead; do not store waste information;
 *              store morphological rules
 * - 13 -> 14 : introduce more general TOC concept; this allows to skip
 *              sections we don't know about. in this way, we can add sections
 *              in the  future without having to change the version number
 *              (19-jun-01) 
 * - 14 -> 15 : remove lexicon and rule section; list of rules and lexicon is
 *               now constructed in the parser using status values 
 * - 15 -> 16: add new section containing grammar properties
 */

#ifndef _GRAMMAR_DUMP_H_
#define _GRAMMAR_DUMP_H_

#include <dumper.h>
#include <map>

/** Default file extension for binary grammar files */
#define GRAMMAR_EXT ".grm"

/** Magic Number of binary grammar files */
#define DUMP_MAGIC 0x03422711
/** Current binary grammar file version */
#define DUMP_VERSION 16

/** @name Header Dump
 * Dump/Undump the header of the grammar file.
 * It consists of the magic number, the dumper version and the \a description,
 * which should be a string describing the grammar and grammar version.
 */
/*@{*/
void dump_header(dumper *f, const char *description);
char *undump_header(dumper *f, int &version);
/*@}*/

/** section type identifiers */
enum sectiontype { SEC_NOSECTION, SEC_SYMTAB, SEC_PRINTNAMES, SEC_HIERARCHY,
                   SEC_FEATTABS, SEC_FULLFORMS, SEC_INFLR, SEC_CONSTRAINTS,
                   SEC_IRREGS, SEC_PROPERTIES, SEC_SUPERTYPES, SEC_CHART };

/** The table of contents structure for reading. */
class dump_toc
{
 public:
  /** Extract the table of contents from the dumper */
  dump_toc(dumper *dump);
  /** go to the section with section type \a s.
   * \return \c true if successful, \c false if the section does not exist or
   * is otherwise invalid.
   */
  bool goto_section(sectiontype s);
  
 private:
  dumper *_dump;
  
  std::map<sectiontype, long int> _toc;
};

/** The table of contents structure for writing/creating a TOC. */
class dump_toc_maker
{
 public:
  /** establish a table of contents for dumper \a dump */
  dump_toc_maker(dumper *dump);
  ~dump_toc_maker();

  /** \brief Add the section with type \a s to the table of contents. All
   *  sections have to be added before dumping of the data starts, because the
   *  TOC is at the beginning of the file.
   */
  void add_section(sectiontype s);
  /** \brief Finish the creation of the table of contents. No more calls to
   *  add_section() can be made after calling this function.
   */
  void close();
    
  /** Start the section \a s at the current file pointer location of the
   *  dumper, and write this position to the table of contents.
   */
  void start_section(sectiontype s);
    
 private:
  dumper *_dump;
  bool _open;
    
  std::map<sectiontype, long int> _toc;
  std::map<sectiontype, long int> _where;
};

#endif

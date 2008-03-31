/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
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

/**
 * \file chart-mapping.h
 * Nonmonotonic rewrite rules that operate on a chart of typed feature
 * structures.
 */ 

#ifndef _CHART_MAPPING_H_
#define _CHART_MAPPING_H_

#include "pet-config.h"

#include "fs.h"
#include "fs-chart.h"
#include "fs-chart-util.h"
#include "list-int.h"

#include <list>
#include <boost/regex.hpp>
#ifdef HAVE_BOOST_REGEX_ICU_HPP
#include <boost/regex/icu.hpp>
#endif
#include <boost/tuple/tuple.hpp>
#include "boost/tuple/tuple_comparison.hpp"


// forward declarations:
class tChartMappingEngine;
class tChartMappingRule;
class tChartMappingRuleArg;
class tChartMappingMatch;


/**
 * A container mapping paths in feature structures to regular expression
 * objects.
 */
#ifdef HAVE_BOOST_REGEX_ICU_HPP
typedef std::map<list_int*, boost::u32regex> tPathRegexMap;
#else
typedef std::map<list_int*, boost::regex> tPathRegexMap;
#endif


/**
 * Class that executes chart mapping rules.
 */
class tChartMappingEngine
{
private:
  
  /**
   * The chart mapping rules to be used in this instance of
   * tChartMappingEngine.
   */
  const std::list<tChartMappingRule*> & _rules;
  
public:
  /**
   * Construct a new object, using the specified chart mapping rules.
   */
  tChartMappingEngine(const std::list<tChartMappingRule*> & rules);
  
  /**
   * Apply the chart mapping rules supplied during construction to the
   * specified chart.
   *
   * \param[in,out] chart    the chart to operate on
   */
  void apply_rules(class tChart &chart);
  
};



enum tChartMappingRuleTrait { CM_INPUT_TRAIT, CM_LEX_TRAIT };

/**
 * Representation of a chart mapping rule of the grammar.
 */
class tChartMappingRule
{
  
  friend class tChartMappingMatch;
  
private:
  
  /** Identifier of the rule's type in PET's type system. */
  type_t _type;
  
  /** Signals which type of items we're operating on. */
  tChartMappingRuleTrait _trait;
  
  /**
   * Feature structure representation of this rule. This fs is not
   * necessarily equal to the type's fs since we replace string literals
   * containing regular expressions with the general string type in order
   * to allow for successful unification. The alternative would be to
   * make every string literal a subtype of the string type and every
   * regular expression that matches this string literal.
   */
  fs _fs;
  
  /**
   * The list of all arguments of this rule. This rule is manages the
   * memory for these items.
   */
  std::vector<tChartMappingRuleArg*> _args;
  
  /**
   * Constructs a new tChartMappingRule object for the specified \c type.
   * \param[in] type
   * \see create()
   */
  tChartMappingRule(type_t type, tChartMappingRuleTrait trait);
  
public:
  
  /** 
   * Set to \c true if all output items shall be created in the same cell.
   */
  bool _same_cell_output; // TODO remove!!! this is a pretty ugly hack
  
  /** 
   * Factory method that returns a new tChartMappingRule object for the
   * specified type. If the feature structure of the given type is not
   * a valid chart mapping rule, \c NULL is returned.
   * \param[in] type
   * \param[in] trait
   * \return a tChartMappingRule for type \a type or \c NULL
   */
  static tChartMappingRule* create(type_t type, tChartMappingRuleTrait trait);

  /**
   * Gets the type of this rule instance.
   */
  type_t type() const;
  
  /**
   * Gets the trait of this rule, namely whether it is an input or lexical
   * chart mapping rule.
   */
  tChartMappingRuleTrait trait() const;

  /**
   * Gets the print name of this rule.
   * \see print_name(type_t)
   */
  const char* printname() const;
  
  /**
   * Returns the feature structure associated with this rule. 
   */
  fs instantiate() const;
  
  /**
   * Returns a list of arguments of this rule (i.e., all CONTEXT and
   * INPUT items) in the order of how they should be matched.
   */
  const std::vector<tChartMappingRuleArg*> args() const;
  
  /**
   * Returns an argument by its name or 0 if it is not found.
   */
  tChartMappingRuleArg* arg(std::string name);
  
};



/**
 * Representation of a positional constraint on chart mapping rule arguments.
 */
class tChartMappingPosCons
{
public:
  
  /** Relation of this constraint. */
  enum tChartMappingPosConsRel { same_cell, succeeding, all_succeeding } rel;
  
  /**
   * Second argument of the relation (the first being the argument holding
   * the constraint).
   */ 
  class tChartMappingRuleArg *arg;
};



/**
 * Representation of a chart mapping rule argument.
 */
class tChartMappingRuleArg
{
  
private:
  
  /** \see name() */
  std::string _name;
  
  /** \see is_input_arg() is_context_arg() */
  bool _is_input;
  
  /** \see nr() */
  int _nr;
  
  /** \see regexs() */
  tPathRegexMap _regexs;
  
  /**
   * Constructor.
   * \see create()
   */
  tChartMappingRuleArg(const std::string &name,
      bool is_input, int nr, tPathRegexMap &regexs);
  
  /** No default copy constructor. */
  tChartMappingRuleArg(const tChartMappingRuleArg& arg);
  
public:
  
  std::list<tChartMappingPosCons> _poscons;
  
  /**
   * Destructor. 
   */
  virtual ~tChartMappingRuleArg();
  
  /**
   * Factory method that returns a new tChartMappingRuleArg object.
   * Enforces the creation of these object on the heap. The caller
   * (tChartMappingRule) is responsible for deleting the object.
   */
  static tChartMappingRuleArg* create(const std::string &name,
      bool is_input, int nr, tPathRegexMap &regexs);
  
  /**
   * The name of this rule argument. Argument's names are used to
   * refer to the relative position to other arguments and to refer
   * to regex captures.
   */
  const std::string& name() const;
  
  /**
   * Checks whether this argument is from the rule's INPUT list.
   */
  bool is_input_arg() const;
  
  /**
   * Checks whether this argument is from the rule's CONTEXT list.
   */
  bool is_context_arg() const;
  
  /**
   * Returns the number of this item within its list (either the INPUT or
   * CONTEXT list).
   */
  int nr() const;
  
  /**
   * Returns a container mapping paths in feature structures to regular
   * expression objects.
   */
  const tPathRegexMap& regexs() const;
  
};



/**
 * A partially or fully matched instantiation of a tChartMappingRule .
 * It is similar to an active chart item in syntactic parsing in that it gives
 * access to all the arguments matched so far and lists the arguments to be
 * matched next. Unlike an active chart item in syntactic parsing it does not
 * represent a structure that can be embedded in other structures, i.e. it is
 * not an item. Once created, objects of this class cannot change anymore but
 * they can give rise to new tChartMappingMatch objects by matching the next
 * argument of the rule.
 */
class tChartMappingMatch
{
  
private:
  /** The rule which is used for building this rule match. */
  const tChartMappingRule *_rule;
  
  /** The previous rule match (0 if this is the empty match). */
  const tChartMappingMatch * const _previous;
  
  /** The current instantiation of the rule match. */
  fs _fs; // TODO const?
  
  /** The tItem that was matched by \a _previous to form this object.  */
  tItem * const _item;
  
  /** The argument that was used to match _item . */
  const tChartMappingRuleArg * const _arg;
  
  /** The string captures of _item . */
  std::map<std::string, std::string> _captures;
  
  /** Index (in the rule's _args list) of the next argument to be matched. */
  unsigned int _next_arg_idx;
  
  /**
   * Constructor.
   * \see create()
   * \see retrieve()
   */
  tChartMappingMatch(const tChartMappingRule *rule,
      const tChartMappingMatch *previous,
      const fs &f, tItem *item, const tChartMappingRuleArg * arg,
      std::map<std::string, std::string> captures,
      unsigned int next_arg_idx);
  
  /**
   * Create a (partially or fully matched) tChartMappingMatch instance for
   * the specified parameters.
   * The returned object is owned by the caller of this method.
   * \see match()
   */
  tChartMappingMatch*
  create(const fs &f, tItem *item, const tChartMappingRuleArg *arg,
      std::map<std::string, std::string> captures, unsigned int next_arg_idx);
  
public:
  
  /**
   * Create a (completely unmatched) tChartMappingMatch instance for the
   * specified \a rule.
   * The returned object is owned by the caller of this method.
   */
  static tChartMappingMatch* create(const tChartMappingRule *rule);
                       
  /** Is this rule match complete? */
  bool is_complete() const;
  
  /** Returns the rule for this rule match. */
  const tChartMappingRule* get_rule() const;
  
  /**
   * Returns the feature structure representation of this match.
   */
  fs get_fs();
  
  /**
   * Returns the feature structure for the specified argument within the
   * feature structure representation of this match.
   */
  fs get_arg_fs(const tChartMappingRuleArg *arg);
  
  /**
   * Returns the next argument that should be matched or \c 0 if the there
   * is no such argument.
   */
  const tChartMappingRuleArg* get_next_arg() const;
  
  /**
   * Tries to match the specified item \a item with the argument \a arg of
   * this match. Returns \c 0 if such a match is not possible.
   * This function uses a regex-enabled unification. Matched (sub-)strings
   * are added to the \a captures map, using "${" + \a get_next_arg().name() +
   * ":" + the path whose value has been matched + ":" + the number of the
   * matched substring + "}" as the key. For example, if the argument's name
   * is "I2" and contains a regular expression "/(bar)rks/" in path "FORM",
   * then the matched substring "bar" is stored under key "${I2:FORM:1}".
   */
  tChartMappingMatch* match(tItem *item, const tChartMappingRuleArg *arg);
  
  /**
   * Returns a matched item by its argument name.
   */
  tItem* matched_item(std::string argname);
  
  /**
   * Returns all INPUT and CONTEXT items that have been matched so far.
   */
  std::list<tItem*> matched_items();
  
  /**
   * Returns all INPUT items that have been matched so far.
   */
  std::list<tItem*> matched_input_items();
  
  /**
   * Returns all INPUT items that have been matched so far.
   */
  std::list<tItem*> matched_context_items();
  
  /**
   * Returns the feature structures of all OUTPUT items.
   */
  std::list<fs> output_fss();
  
};



// ========================================================================
// INLINE DEFINITIONS
// ========================================================================

// =====================================================
// class tChartMappingEngine
// =====================================================

inline
tChartMappingEngine::tChartMappingEngine(
    const std::list<tChartMappingRule*> &rules)
: _rules(rules)
{
  // nothing to do
}



// =====================================================
// class tChartMappingRule
// =====================================================

inline tChartMappingRule*
tChartMappingRule::create(type_t type, tChartMappingRuleTrait trait)
{
  try {
    return new tChartMappingRule(type, trait);
  } catch (tError error) {
    fprintf(stderr, "%s\n", error.getMessage().c_str()); // ferr not in scope
  }
  return NULL;
}

inline type_t
tChartMappingRule::type() const
{
  return _type;
}

inline tChartMappingRuleTrait
tChartMappingRule::trait() const
{
  return _trait;
}

inline const char*
tChartMappingRule::printname() const
{
  return print_name(_type);
}

inline fs
tChartMappingRule::instantiate() const
{
  return copy(_fs);
}

inline const std::vector<tChartMappingRuleArg*>
tChartMappingRule::args() const
{
  return _args;
}



// =====================================================
// class tChartMappingRuleArg
// =====================================================

inline
tChartMappingRuleArg::tChartMappingRuleArg(const std::string &name,
    bool is_input, int nr, tPathRegexMap &regexs)
: _name(name), _is_input(is_input), _nr(nr), _regexs(regexs)
{
  // done
}

inline
tChartMappingRuleArg::~tChartMappingRuleArg()
{
  for (tPathRegexMap::iterator it = _regexs.begin(); it != _regexs.end(); it++)
    free_list(it->first);
}

inline tChartMappingRuleArg*
tChartMappingRuleArg::create(const std::string &name,
    bool is_input, int nr, tPathRegexMap &regexs)
{
  return new tChartMappingRuleArg(name, is_input, nr, regexs);
}

inline const std::string&
tChartMappingRuleArg::name() const
{
  return _name;
}

inline bool
tChartMappingRuleArg::is_input_arg() const
{
  return _is_input; 
}

inline bool
tChartMappingRuleArg::is_context_arg() const
{
  return !_is_input;
}

inline int
tChartMappingRuleArg::nr() const
{
  return _nr;
}

inline const tPathRegexMap&
tChartMappingRuleArg::regexs() const
{
  return _regexs;
}



// =====================================================
// class tChartMappingMatch
// =====================================================

inline
tChartMappingMatch::tChartMappingMatch(const tChartMappingRule *rule,
    const tChartMappingMatch *previous, 
    const fs &f, tItem *item, const tChartMappingRuleArg *arg,
    std::map<std::string, std::string> captures, unsigned int next_arg_idx)
: _rule(rule), _previous(previous), _fs(f), _item(item), _arg(arg),
  _captures(captures), _next_arg_idx(next_arg_idx)
{
  // done
}

inline tChartMappingMatch*
tChartMappingMatch::create(const tChartMappingRule *rule)
{
  return new tChartMappingMatch(rule, 0, rule->instantiate(), 0, 0,
      std::map<std::string, std::string>(), 0);
}

inline tChartMappingMatch*
tChartMappingMatch::create(const fs &f,
    tItem *item,
    const tChartMappingRuleArg *arg,
    std::map<std::string, std::string> captures,
    unsigned int next_arg_idx)
{
  return new tChartMappingMatch(_rule, this, f, item, arg, captures,
      next_arg_idx);
}

inline bool
tChartMappingMatch::is_complete() const
{
  return (_next_arg_idx == _rule->_args.size());
}

inline const tChartMappingRule*
tChartMappingMatch::get_rule() const
{
  return _rule; 
}

inline fs
tChartMappingMatch::get_fs()
{
  return _fs;
}

inline fs
tChartMappingMatch::get_arg_fs(const tChartMappingRuleArg *arg)
{
  const list_int *path = arg->is_input_arg() ?
      tChartUtil::input_path() : tChartUtil::context_path();
  return _fs.nth_value(path, arg->nr());
}


inline const tChartMappingRuleArg*
tChartMappingMatch::get_next_arg() const
{
  return is_complete() ? 0 : _rule->_args[_next_arg_idx];
}

inline tItem*
tChartMappingMatch::matched_item(std::string argname)
{
  for (const tChartMappingMatch *curr = this; curr; curr = curr->_previous)
    if (curr->_arg && curr->_arg->name() == argname)
      return curr->_item;
  return 0;
}

inline std::list<tItem*>
tChartMappingMatch::matched_items()
{
  std::list<tItem*> items;
  for (const tChartMappingMatch *curr = this; curr; curr = curr->_previous)
    if (curr->_arg)
      items.push_front(curr->_item);
  return items;
}

inline std::list<tItem*>
tChartMappingMatch::matched_input_items()
{
  std::list<tItem*> items;
  for (const tChartMappingMatch *curr = this; curr; curr = curr->_previous)
    if (curr->_arg && curr->_arg->is_input_arg())
      items.push_front(curr->_item);
  return items;
}

inline std::list<tItem*>
tChartMappingMatch::matched_context_items()
{
  std::list<tItem*> items;
  for (const tChartMappingMatch *curr = this; curr; curr = curr->_previous)
    if (curr->_arg && curr->_arg->is_context_arg())
      items.push_front(curr->_item);
  return items;
}

#endif /*_CHART_MAPPING_H_*/

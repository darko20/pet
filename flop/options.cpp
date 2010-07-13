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

/* command line options */

#include "flop.h"
#include "options.h"
#include "version.h"
#include "utility.h"
#include "logging.h"
#include "pet-config.h"
#include "configs.h"
#include <iostream>
#include <exception>
#include <boost/program_options.hpp>

namespace po = boost::program_options;


std::string parse_options(int argc, char* argv[])
{
    int cmi = 0;
    int local_cap = 1000;
    int count_tasks = 0;
    std::string filename;
    po::options_description generic("generic options");
    generic.add_options()
        ("help", "produce help message")
        ("pre", "do only syntactic preprocessing")
        ("expand-all-instances", "expand all (even lexicon) instances")
        ("full-expansion", "don't do partial expansion")
        ("unfill", "unfill after expansion")
        ("minimal", "minimal fixed arity encoding")
        ("no-semantics", "remove all semantics")
        ("propagate-status", "propagate status the PAGE way")
        ("cmi", po::value<int>(&cmi)->implicit_value(0), "create morph info, level = 0..2, default 0")
        ("local-cap", po::value<int>(&local_cap), "enable local phrasal search space restriction")
        ("count-tasks", po::value<int>(&count_tasks) ,"indicates which types of tasks should be counted: all (0, default), successful (1), or successful+passive (2).");
        ;
    po::options_description hidden("hidden options");
    hidden.add_options()
        ("input-file", po::value<std::string>(&filename), "input file")
    ;        

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(hidden);

    po::options_description visible("valid options are");
    visible.add(generic);

    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).
            options(cmdline_options).positional(p).run(), vm);
        po::notify(vm);
        if (vm.count("help") || (cmi<0) || (cmi>2)) {
            std::cout << visible << "\n";
            return "";
        }
        managed_opt("opt_expand_all_instances",
            "expand all type definitions, except for pseudo types", false);
        managed_opt("opt_minimal", "", false);
        managed_opt("opt_no_sem", "", false);

        set_opt("opt_pre", vm.count("pre")>0);
        set_opt("opt_unfill", vm.count("unfill")>0);
        set_opt("opt_minimal", vm.count("minimal")>0);
        set_opt("opt_full_expansion", vm.count("full-expansion")>0);
        set_opt("opt_expand_all_instances", vm.count("expand-all-instances")>0);
        set_opt("opt_no_sem", vm.count("no-semantics")>0);
        set_opt("opt_propagate_status", vm.count("propagate-status")>0);
        set_opt("opt_cmi", cmi);
        set_opt("opt_local_cap", local_cap);
        set_opt("opt_count_tasks", count_tasks);
    }
    catch (po::required_option) {
        throw std::logic_error("option error");
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        throw std::logic_error("option error");
    }
    if (filename.empty()) {
        LOG(root, FATAL, "parse_options(): expecting name of TDL grammar to process");
    }
    return filename;
}

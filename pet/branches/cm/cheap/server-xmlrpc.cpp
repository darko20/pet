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

#include "pet-config.h"
#include "server-xmlrpc.h"

#include "chart.h"
#include "item-printer.h"
#include "mrs.h"
#include "options.h"
#include "parse.h"
#include "tsdb++.h"
#include "vpm.h"

#ifdef HAVE_ECL
#include "petecl.h"
#endif
#ifdef HAVE_MRS
#include "petmrs.h"
#include "cppbridge.h"
#endif

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/girerr.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

using namespace std;

extern tVPM *vpm;

struct alive_method : public xmlrpc_c::method
{
  alive_method()
  {
    _signature = "b:";
    _help = "Returns true if this server is alive.";
  }
  
  void execute(xmlrpc_c::paramList const& params,
               xmlrpc_c::value * const retval)
  {
    // get method parameters:
    params.verifyEnd(0);
    
    // prepare return value:
    *retval = xmlrpc_c::value_boolean(true);
  }
};


struct info_method : public xmlrpc_c::method
{
  info_method()
  {
    _signature = "s:";
    _help = "Return a description of this server.";
  }
  
  void execute(xmlrpc_c::paramList const& params,
               xmlrpc_c::value * const retval)
  {
    // get method parameters:
    params.verifyEnd(0);
    
    // prepare return value:
    *retval = xmlrpc_c::value_string("cheap XML/RPC server\n"
        "see http://wiki.delph-in.net/moin/PetTop\n"
        "This XML/RPC interface is not stable, "
        "it may change in future versions.");
  }
};


struct analyze_method : public xmlrpc_c::method
{
  analyze_method()
  {
    _signature = "S:s";
    _help = "Analyze the specified input and return the result."
        "The input should be in the same format as required by this"
        "cheap server (usually specified with the `-tok' parameter).";
  }
  
  void execute(xmlrpc_c::paramList const& params, xmlrpc_c::value* const retval)
  {
    // get method parameters:
    string input(params.getString(0));
    params.verifyEnd(1);
    
    // Define a string that will hold the error message for errors that
    // occurred during parsing or be empty if there were no errors.
    // This kind of error handling is a bit clumsy. But it makes it easier
    // to cleanup the resources acquired during analyze(). A better solution
    // would be to use the RAII pattern for all required resources.
    string error;
    
    // analyze string:
    chart *Chart = 0;
    fs_alloc_state FSAS;
    int id = 1;
    try {
      list<tError> errors;
      analyze(input, Chart, FSAS, errors, id);
      // this looks rather strange, but it seems to be the current paradigm
      if (!errors.empty()) 
        throw errors.front();
    } catch (tError e) {
      error = e.getMessage();
    }
    
    // collect readings:
    if (error.empty()) {
      vector<xmlrpc_c::value> readings_helper;
      list<tItem*> readings(Chart->readings().begin(), Chart->readings().end());
      for (list<tItem*>::iterator it=readings.begin(); it!=readings.end(); it++)
      {
        tItem *item = *it;
        
        // get derivation:
        std::map<std::string, xmlrpc_c::value> reading_helper;
        ostringstream osstream;
        tCompactDerivationPrinter printer(osstream);
        printer.print(item);
        reading_helper["derivation"] = xmlrpc_c::value_string(osstream.str());
        
        // get MRS:
        string opt_mrs = get_opt_string("opt_mrs");
        if (!opt_mrs.empty()) {
          string mrs_str;
          if (opt_mrs == "new") {
            osstream.clear();
            fs f = item->get_fs();
            mrs::tPSOA* mrs = new mrs::tPSOA(f.dag());
            if (mrs->valid()) {
              mrs::tPSOA* mapped_mrs = vpm->map_mrs(mrs, true);
              if (mapped_mrs->valid())
                mapped_mrs->print(osstream);
              delete mapped_mrs;
            }
            delete mrs;
            mrs_str = osstream.str();
          } else {
#ifdef HAVE_MRS
            mrs_str = ecl_cpp_extract_mrs(item->get_fs().dag(), opt_mrs.c_str());
#else
            mrs_str = "";
#endif
          }
          reading_helper["mrs"] = xmlrpc_c::value_string(mrs_str);
        }
        
        // store reading:
        readings_helper.push_back(xmlrpc_c::value_struct(reading_helper));
      }
      
      // prepare return value:
      std::map<std::string, xmlrpc_c::value> results_helper;
      results_helper["surface"] =
        xmlrpc_c::value_string(Chart->get_surface_string());
      results_helper["readings"] = xmlrpc_c::value_array(readings_helper);
      *retval = xmlrpc_c::value_struct(results_helper);
    }
    
    // delete resources:
    if (Chart != 0)
      delete Chart;
    
    if (!error.empty()) {
      // rethrow as girerr::error to make the message available to the client
      throw girerr::error(error);
    }
  }
};


struct parsable_method : public xmlrpc_c::method
{
  parsable_method()
  {
    _signature = "b:s";
    _help = "Analyze the specified input and return whether it is parsable.";
  }
  
  void execute(xmlrpc_c::paramList const& params, xmlrpc_c::value* const retval)
  {
    // get method parameters:
    string input(params.getString(0));
    params.verifyEnd(1);
    
    // Define a string that will hold the error message for errors that
    // occurred during parsing or be empty if there were no errors.
    // This kind of error handling is a bit clumsy. But it makes it easier
    // to cleanup the resources acquired during analyze(). A better solution
    // would be to use the RAII pattern for all required resources.
    string error;
    
    // analyze string:
    chart *Chart = 0;
    fs_alloc_state FSAS;
    int id = 1;
    try {
      list<tError> errors;
      analyze(input, Chart, FSAS, errors, id);
      // this looks rather strange, but it seems to be the current paradigm
      if (!errors.empty()) 
        throw errors.front();
    } catch (tError e) {
      error = e.getMessage();
    }
    
    bool result = error.empty() && !Chart->readings().empty();
    *retval = xmlrpc_c::value_boolean(result);
    
    // delete resources:
    if (Chart != 0)
      delete Chart;
  }
};


tXMLRPCServer::tXMLRPCServer(int port)
: _port(port)
{
  _socket = socket(AF_INET, SOCK_STREAM, 0);
  if (_socket == -1) {
    throw tError((std::string)"Unable to create server socket: "
        + strerror(errno));
  }
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);
  if (::bind(_socket, (struct sockaddr *) &address, sizeof(address)) == -1) {
    throw tError((std::string)"Unable to bind server socket: "
        + strerror(errno));
  }
  cerr << "[XML-RPC server] bound to port " << port << '\n';
}

void
tXMLRPCServer::run()
{
  std::string uri_path = "/cheap-rpc2";
  if (!_socket)
    throw tError("Server socket is not initialized.");
  
  xmlrpc_c::registry reg;
  xmlrpc_c::methodPtr const alive_method_ptr(new alive_method);
  xmlrpc_c::methodPtr const info_method_ptr(new info_method);
  xmlrpc_c::methodPtr const analyze_method_ptr(new analyze_method);
  xmlrpc_c::methodPtr const parsable_method_ptr(new parsable_method);
  reg.addMethod("cheap.alive", alive_method_ptr);
  reg.addMethod("cheap.info", info_method_ptr);
  reg.addMethod("cheap.analyze", analyze_method_ptr);
  reg.addMethod("cheap.parsable", parsable_method_ptr);
  
  xmlrpc_c::serverAbyss server(xmlrpc_c::serverAbyss::constrOpt()
    .registryP(&reg)
    .socketFd(_socket)
    .uriPath(uri_path)
    .logFileName("/dev/stderr"));
  
  cerr << "[XML-RPC server] waiting for clients at http://localhost:"
       << _port << uri_path << '\n';
  server.run();
  assert(false); // server.run() should never return
}

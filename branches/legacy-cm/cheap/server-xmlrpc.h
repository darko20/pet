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

/** \file server-xmlrpc.h
 * Cheap server using the XML-RPC protocol.
 */

#ifndef _SERVER_XMLRPC_H
#define _SERVER_XMLRPC_H

#include "pet-config.h"

/**
 * An XML-RPC server for cheap.
 */
class tXMLRPCServer
{
public:
  
  /** Default constructor, bound to port \a port. */
  tXMLRPCServer(int port);
  
  /**
   * Start the server.
   */
  void run();
  
private:
  
  /** socket file descriptor */
  int _socket;
  
  /** port to which this server is bound */
  int _port;
  
};

#endif /*_SERVER_XMLRPC_H*/

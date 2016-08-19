/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "legacy-nfdc.hpp"
#include "core/version.hpp"

#include <boost/lexical_cast.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

static void
usage(const char* programName)
{
  std::cout << "Usage:\n" << programName  << " [-h] [-V] COMMAND [<Command Options>]\n"
    "       -h print usage and exit\n"
    "       -V print version and exit\n"
    "\n"
    "   COMMAND can be one of the following:\n"
    "       register [-I] [-C] [-c cost] [-e expiration time] [-o origin] name <faceId | faceUri>\n"
    "           register name to the given faceId or faceUri\n"
    "           -I: unset CHILD_INHERIT flag\n"
    "           -C: set CAPTURE flag\n"
    "           -c: specify cost (default 0)\n"
    "           -e: specify expiration time in ms\n"
    "               (by default the entry remains in FIB for the lifetime of the associated face)\n"
    "           -o: specify origin\n"
    "               0 for Local producer applications, 128 for NLSR, 255(default) for static routes\n"
    "       unregister [-o origin] name <faceId | faceUri>\n"
    "           unregister name from the given faceId\n"
    "       create [-P] <faceUri> \n"
    "           Create a face in one of the following formats:\n"
    "           UDP unicast:    udp[4|6]://<remote-IP-or-host>[:<remote-port>]\n"
    "           TCP:            tcp[4|6]://<remote-IP-or-host>[:<remote-port>] \n"
    "           -P: create permanent (instead of persistent) face\n"
    "       destroy <faceId | faceUri> \n"
    "           Destroy a face\n"
    "       set-strategy <name> <strategy> \n"
    "           Set the strategy for a namespace \n"
    "       unset-strategy <name> \n"
    "           Unset the strategy for a namespace \n"
    "       add-nexthop [-c <cost>] <name> <faceId | faceUri>\n"
    "           Add a nexthop to a FIB entry\n"
    "           -c: specify cost (default 0)\n"
    "       remove-nexthop <name> <faceId | faceUri> \n"
    "           Remove a nexthop from a FIB entry\n"
    << std::endl;
}

static int
main(int argc, char** argv)
{
  ndn::Face face;
  LegacyNfdc p(face);

  p.m_programName = argv[0];

  if (argc < 2) {
    usage(p.m_programName);
    return 0;
  }

  if (!strcmp(argv[1], "-h")) {
    usage(p.m_programName);
    return 0;
  }

  if (!strcmp(argv[1], "-V")) {
    std::cout << NFD_VERSION_BUILD_STRING << std::endl;
    return 0;
  }

  ::optind = 2; //start reading options from 2nd argument i.e. Command
  int opt;
  while ((opt = ::getopt(argc, argv, "ICc:e:o:P")) != -1) {
    switch (opt) {
      case 'I':
        p.m_flags = p.m_flags & ~(ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
        break;

      case 'C':
        p.m_flags = p.m_flags | ndn::nfd::ROUTE_FLAG_CAPTURE;
        break;

      case 'c':
        try {
          p.m_cost = boost::lexical_cast<uint64_t>(::optarg);
        }
        catch (const boost::bad_lexical_cast&) {
          std::cerr << "Error: cost must be in unsigned integer format" << std::endl;
          return 1;
        }
        break;

      case 'e':
        uint64_t expires;
        try {
          expires = boost::lexical_cast<uint64_t>(::optarg);
        }
        catch (const boost::bad_lexical_cast&) {
          std::cerr << "Error: expiration time must be in unsigned integer format" << std::endl;
          return 1;
        }
        p.m_expires = ndn::time::milliseconds(expires);
        break;

      case 'o':
        try {
          p.m_origin = boost::lexical_cast<uint64_t>(::optarg);
        }
        catch (const boost::bad_lexical_cast&) {
          std::cerr << "Error: origin must be in unsigned integer format" << std::endl;
          return 1;
        }
        break;

      case 'P':
        p.m_facePersistency = ndn::nfd::FACE_PERSISTENCY_PERMANENT;
        break;

      default:
        usage(p.m_programName);
        return 1;
    }
  }

  if (argc == ::optind) {
    usage(p.m_programName);
    return 1;
  }

  try {
    p.m_commandLineArguments = argv + ::optind;
    p.m_nOptions = argc - ::optind;

    //argv[1] points to the command, so pass it to the dispatch
    bool isOk = p.dispatch(argv[1]);
    if (!isOk) {
      usage(p.m_programName);
      return 1;
    }
    face.processEvents();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }
  return 0;
}

} // namespace nfdc
} // namespace tools
} // namespace nfd

int
main(int argc, char** argv)
{
  return nfd::tools::nfdc::main(argc, argv);
}

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "program.hpp"
#include "core/extended-error-message.hpp"
#include "core/version.hpp"

#include <iostream>
#include <unistd.h>

namespace ndn {
namespace tools {
namespace autoconfig_server {

static void
usage(const char* programName)
{
  std::cout << "Usage: " << programName << " [-h] [-V] [-p prefix]... <hub-face>\n"
            << "\n"
            << "Options:\n"
            << "  -h        - print usage and exit\n"
            << "  -V        - print version number and exit\n"
            << "  -p prefix - a local prefix of the HUB\n"
            << "  hub-face  - a FaceUri to reach the HUB\n";
}

static int
main(int argc, char** argv)
{
  Options options;

  int opt = -1;
  while ((opt = ::getopt(argc, argv, "hVp:")) != -1) {
    switch (opt) {
    case 'h':
      usage(argv[0]);
      return 0;
    case 'V':
      std::cout << NFD_VERSION_BUILD_STRING << std::endl;
      return 0;
    case 'p':
      options.routablePrefixes.emplace_back(::optarg);
      break;
    default:
      usage(argv[0]);
      return 2;
    }
  }

  if (argc != ::optind + 1) {
    usage(argv[0]);
    return 2;
  }

  if (!options.hubFaceUri.parse(argv[::optind])) {
    std::cerr << "ERROR: cannot parse HUB FaceUri" << std::endl;
    return 2;
  }

  try {
    Face face;
    KeyChain keyChain;
    Program program(options, face, keyChain);
    program.run();
  }
  catch (const std::exception& e) {
    std::cerr << ::nfd::getExtendedErrorMessage(e) << std::endl;
    return 1;
  }
  return 0;
}

} // namespace autoconfig_server
} // namespace tools
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::tools::autoconfig_server::main(argc, argv);
}

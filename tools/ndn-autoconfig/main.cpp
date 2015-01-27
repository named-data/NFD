/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include "version.hpp"

#include "multicast-discovery.hpp"
#include "guess-from-search-domains.hpp"
#include "guess-from-identity-name.hpp"

#include <boost/noncopyable.hpp>

namespace ndn {
namespace tools {

class NdnAutoconfig : boost::noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  NdnAutoconfig()
    : m_stage1(m_face, m_keyChain,
               [&] (const std::string& errorMessage) {
                 std::cerr << "Stage 1 failed: " << errorMessage << std::endl;
                 m_stage2.start();
               })
    , m_stage2(m_face, m_keyChain,
               [&] (const std::string& errorMessage) {
                 std::cerr << "Stage 2 failed: " << errorMessage << std::endl;
                 m_stage3.start();
               })
    , m_stage3(m_face, m_keyChain,
               [&] (const std::string& errorMessage) {
                 std::cerr << "Stage 3 failed: " << errorMessage << std::endl;
                 throw Error("No more stages, automatic discovery failed");
               })
  {
    m_stage1.start();
  }

  void
  run()
  {
    m_face.processEvents();
  }

  static void
  usage(const char* programName)
  {
    std::cout << "Usage:\n"
              << "  " << programName  << " [options]\n"
              << "\n"
              << "Options:\n"
              << "  [-h]  - print usage and exit\n"
              << "  [-V]  - print version number and exit\n"
              << std::endl;
  }

private:
  Face m_face;
  KeyChain m_keyChain;

  autoconfig::MulticastDiscovery m_stage1;
  autoconfig::GuessFromSearchDomains m_stage2;
  autoconfig::GuessFromIdentityName m_stage3;
};

} // namespace tools
} // namespace ndn

int
main(int argc, char** argv)
{
  int opt;
  const char* programName = argv[0];

  while ((opt = getopt(argc, argv, "hV")) != -1) {
    switch (opt) {
    case 'h':
      ndn::tools::NdnAutoconfig::usage(programName);
      return 0;
    case 'V':
      std::cout << NFD_VERSION_BUILD_STRING << std::endl;
      return 0;
    }
  }

  try {
    ndn::tools::NdnAutoconfig autoConfigInstance;
    autoConfigInstance.run();
  }
  catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << std::endl;
    return 1;
  }
  return 0;
}

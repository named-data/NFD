/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#include "version.hpp"
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>


void
usage(const char* programName)
{
  std::cout << "Usage:\n" << programName  << " [-h] [-V] Uri \n"
            << "   -h  - print usage and exit\n"
            << "   -V  - print version number and exit\n"
            << "\n"
            << "   Uri - a FaceMgmt URI\n"
            << std::endl;
}

using namespace ndn;

class NdnAutoconfigServer
{
public:
  explicit
  NdnAutoconfigServer(const std::string& uri)
    : m_faceMgmtUri(uri)
  {
  }

  void
  onInterest(const Name& name, const Interest& interest)
  {
    ndn::Data data(ndn::Name(interest.getName()).appendVersion());
    data.setFreshnessPeriod(ndn::time::hours(1)); // 1 hour

    // create and encode uri block
    Block uriBlock = dataBlock(tlv::nfd::Uri,
                               reinterpret_cast<const uint8_t*>(m_faceMgmtUri.c_str()),
                               m_faceMgmtUri.size());
    data.setContent(uriBlock);
    m_keyChain.sign(data);
    m_face.put(data);
  }

  void
  onRegisterFailed(const ndn::Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix in local hub's daemon (" <<
              reason << ")" << std::endl;
    m_face.shutdown();
  }

  void
  listen()
  {
    m_face.setInterestFilter("/localhop/ndn-autoconf/hub",
                             ndn::bind(&NdnAutoconfigServer::onInterest, this, _1, _2),
                             ndn::bind(&NdnAutoconfigServer::onRegisterFailed, this, _1, _2));
    m_face.processEvents();
  }

private:
  ndn::Face m_face;
  KeyChain m_keyChain;
  std::string m_faceMgmtUri;

};

int
main(int argc, char** argv)
{
  int opt;
  const char* programName = argv[0];

  while ((opt = getopt(argc, argv, "hV")) != -1) {
    switch (opt) {
    case 'h':
      usage(programName);
      return 0;
    case 'V':
      std::cout << NFD_VERSION_BUILD_STRING << std::endl;
      return 0;
    default:
      usage(programName);
      return 1;
    }
  }

  if (argc != optind + 1) {
    usage(programName);
    return 1;
  }
  // get the configured face management uri
  NdnAutoconfigServer producer(argv[optind]);

  try {
    producer.listen();
  }
  catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << std::endl;
    return 1;
  }
  return 0;
}

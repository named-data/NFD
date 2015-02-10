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

namespace ndn {

const static Name LOCALHOP_HUB               = "/localhop/ndn-autoconf/hub";
const static Name LOCALHOP_ROUTABLE_PREFIXES = "/localhop/nfd/rib/routable-prefixes";

static void
usage(const char* programName)
{
  std::cout << "Usage:\n" << programName  << " [-h] [-V] [-p prefix] [-p prefix] ... Uri \n"
            << "   -h        - print usage and exit\n"
            << "   -V        - print version number and exit\n"
            << "   -p prefix - the local prefix of the hub\n"
            << "\n"
            << "   Uri - a FaceMgmt URI\n"
            << std::endl;
}

class PrefixCollection : noncopyable
{
public:
  bool
  empty() const
  {
    return m_prefixes.empty();
  }

  void
  add(const Name& prefix)
  {
    m_prefixes.push_back(prefix);
  }

  template<bool T>
  size_t
  wireEncode(EncodingImpl<T>& encoder) const
  {
    size_t totalLength = 0;

    for (std::vector<Name>::const_reverse_iterator i = m_prefixes.rbegin();
         i != m_prefixes.rend(); ++i) {
      totalLength += i->wireEncode(encoder);
    }

    totalLength += encoder.prependVarNumber(totalLength);
    totalLength += encoder.prependVarNumber(tlv::Content);
    return totalLength;
  }

  Block
  wireEncode() const
  {
    Block block;

    EncodingEstimator estimator;
    size_t estimatedSize = wireEncode(estimator);

    EncodingBuffer buffer(estimatedSize);
    wireEncode(buffer);

    return buffer.block();
  }

private:
  std::vector<Name> m_prefixes;
};

class NdnAutoconfigServer : noncopyable
{
public:
  NdnAutoconfigServer(const std::string& hubFaceUri, const PrefixCollection& routablePrefixes)
  {
    KeyChain m_keyChain;

    // pre-create hub Data
    m_hubData = make_shared<Data>(Name(LOCALHOP_HUB).appendVersion());
    m_hubData->setFreshnessPeriod(time::hours(1)); // 1 hour
    m_hubData->setContent(dataBlock(tlv::nfd::Uri,
                                    reinterpret_cast<const uint8_t*>(hubFaceUri.c_str()),
                                    hubFaceUri.size()));
    m_keyChain.sign(*m_hubData);

    // pre-create routable prefix Data
    if (!routablePrefixes.empty()) {
      Name routablePrefixesDataName(LOCALHOP_ROUTABLE_PREFIXES);
      routablePrefixesDataName.appendVersion();
      routablePrefixesDataName.appendSegment(0);
      m_routablePrefixesData = make_shared<Data>(routablePrefixesDataName);
      m_routablePrefixesData->setContent(routablePrefixes.wireEncode());
      m_routablePrefixesData->setFinalBlockId(routablePrefixesDataName.get(-1));
      m_routablePrefixesData->setFreshnessPeriod(time::seconds(5)); // 5s
      m_keyChain.sign(*m_routablePrefixesData);
    }
  }

  void
  onHubInterest(const Name& name, const Interest& interest)
  {
    m_face.put(*m_hubData);
  }

  void
  onRoutablePrefixesInterest(const Name& name, const Interest& interest)
  {
    m_face.put(*m_routablePrefixesData);
  }

  void
  onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix in local hub's daemon (" <<
              reason << ")" << std::endl;
    m_face.shutdown();
  }

  void
  run()
  {
    m_face.setInterestFilter(LOCALHOP_HUB,
                             bind(&NdnAutoconfigServer::onHubInterest, this, _1, _2),
                             RegisterPrefixSuccessCallback(),
                             bind(&NdnAutoconfigServer::onRegisterFailed, this, _1, _2));

    if (static_cast<bool>(m_routablePrefixesData)) {
      m_face.setInterestFilter(LOCALHOP_ROUTABLE_PREFIXES,
                               bind(&NdnAutoconfigServer::onRoutablePrefixesInterest, this, _1, _2),
                               RegisterPrefixSuccessCallback(),
                               bind(&NdnAutoconfigServer::onRegisterFailed, this, _1, _2));
    }

    m_face.processEvents();
  }

private:
  Face m_face;

  shared_ptr<Data> m_hubData;
  shared_ptr<Data> m_routablePrefixesData;
};

int
main(int argc, char** argv)
{
  const char* programName = argv[0];

  PrefixCollection routablePrefixes;

  int opt;
  while ((opt = getopt(argc, argv, "hVp:")) != -1) {
    switch (opt) {
    case 'h':
      usage(programName);
      return 0;
    case 'V':
      std::cout << NFD_VERSION_BUILD_STRING << std::endl;
      return 0;
    case 'p':
      routablePrefixes.add(Name(optarg));
      break;
    default:
      usage(programName);
      return 1;
    }
  }

  if (argc != optind + 1) {
    usage(programName);
    return 1;
  }

  std::string hubFaceUri = argv[optind];
  NdnAutoconfigServer instance(hubFaceUri, routablePrefixes);

  try {
    instance.run();
  }
  catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << std::endl;
    return 1;
  }
  return 0;
}

} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::main(argc, argv);
}

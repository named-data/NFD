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

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/face-uri.hpp>
#include <ndn-cxx/management/nfd-controller.hpp>
#include <ndn-cxx/management/nfd-face-monitor.hpp>
#include <ndn-cxx/management/nfd-face-status.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "version.hpp"
#include "core/network.hpp"

using namespace ndn::nfd;
using ndn::Face;
using ndn::KeyChain;
using ndn::nfd::FaceEventNotification;
using ndn::util::FaceUri;
using ::nfd::Network;

namespace ndn {
namespace nfd_autoreg {

namespace po = boost::program_options;

class AutoregServer : boost::noncopyable
{
public:
  AutoregServer()
    : m_controller(m_face, m_keyChain)
    , m_faceMonitor(m_face)
    , m_cost(255)
  {
  }

  void
  onRegisterCommandSuccess(uint64_t faceId, const Name& prefix)
  {
    std::cerr << "SUCCEED: register " << prefix << " on face " << faceId << std::endl;
  }

  void
  onRegisterCommandFailure(uint64_t faceId, const Name& prefix,
                           uint32_t code, const std::string& reason)
  {
    std::cerr << "FAILED: register " << prefix << " on face " << faceId
              << " (code: " << code << ", reason: " << reason << ")" << std::endl;
  }

  /**
   * \return true if uri has schema allowed to do auto-registrations
   */
  bool
  hasAllowedSchema(const FaceUri& uri)
  {
    const std::string& scheme = uri.getScheme();
    return (scheme == "udp4" || scheme == "tcp4" ||
            scheme == "udp6" || scheme == "tcp6");
  }

  /**
   * \return true if address is blacklisted
   */
  bool
  isBlacklisted(const boost::asio::ip::address& address)
  {
    for (std::vector<Network>::const_iterator network = m_blackList.begin();
         network != m_blackList.end();
         ++network)
      {
        if (network->doesContain(address))
          return true;
      }

    return false;
  }

  /**
   * \return true if address is whitelisted
   */
  bool
  isWhitelisted(const boost::asio::ip::address& address)
  {
    for (std::vector<Network>::const_iterator network = m_whiteList.begin();
         network != m_whiteList.end();
         ++network)
      {
        if (network->doesContain(address))
          return true;
      }

    return false;
  }

  void
  registerPrefixesForFace(uint64_t faceId,
                          const std::vector<ndn::Name>& prefixes)
  {
    for (std::vector<ndn::Name>::const_iterator prefix = prefixes.begin();
         prefix != prefixes.end();
         ++prefix)
      {
        m_controller.start<RibRegisterCommand>(
          ControlParameters()
            .setName(*prefix)
            .setFaceId(faceId)
            .setOrigin(ROUTE_ORIGIN_AUTOREG)
            .setCost(m_cost)
            .setExpirationPeriod(time::milliseconds::max()),
          bind(&AutoregServer::onRegisterCommandSuccess, this, faceId, *prefix),
          bind(&AutoregServer::onRegisterCommandFailure, this, faceId, *prefix, _1, _2));
      }
  }

  void
  registerPrefixesIfNeeded(uint64_t faceId, const FaceUri& uri, FacePersistency facePersistency)
  {
    if (hasAllowedSchema(uri)) {
      boost::system::error_code ec;
      boost::asio::ip::address address = boost::asio::ip::address::from_string(uri.getHost(), ec);

      if (!address.is_multicast()) {
        // register all-face prefixes
        registerPrefixesForFace(faceId, m_allFacesPrefixes);

        // register autoreg prefixes if new face is on-demand and not blacklisted and whitelisted
        if (facePersistency == FACE_PERSISTENCY_ON_DEMAND &&
            !isBlacklisted(address) && isWhitelisted(address)) {
          registerPrefixesForFace(faceId, m_autoregPrefixes);
        }
      }
    }
  }

  void
  onNotification(const FaceEventNotification& notification)
  {
    if (notification.getKind() == FACE_EVENT_CREATED &&
        notification.getFaceScope() != FACE_SCOPE_LOCAL)
      {
        std::cerr << "PROCESSING: " << notification << std::endl;

        registerPrefixesIfNeeded(notification.getFaceId(), FaceUri(notification.getRemoteUri()),
                                 notification.getFacePersistency());
      }
    else
      {
        std::cerr << "IGNORED: " << notification << std::endl;
      }
  }


  void
  signalHandler()
  {
    m_face.shutdown();
  }


  void
  usage(std::ostream& os,
        const po::options_description& optionDesciption,
        const char* programName)
  {
    os << "Usage:\n"
       << "  " << programName << " --prefix=</autoreg/prefix> [--prefix=/another/prefix] ...\n"
       << "\n";
    os << optionDesciption;
  }

  void
  startProcessing()
  {
    std::cerr << "AUTOREG prefixes: " << std::endl;
    for (std::vector<ndn::Name>::const_iterator prefix = m_autoregPrefixes.begin();
         prefix != m_autoregPrefixes.end();
         ++prefix)
      {
        std::cout << "  " << *prefix << std::endl;
      }
    std::cerr << "ALL-FACES-AUTOREG prefixes: " << std::endl;
    for (std::vector<ndn::Name>::const_iterator prefix = m_allFacesPrefixes.begin();
         prefix != m_allFacesPrefixes.end();
         ++prefix)
      {
        std::cout << "  " << *prefix << std::endl;
      }

    if (!m_blackList.empty())
      {
        std::cerr << "Blacklisted networks: " << std::endl;
        for (std::vector<Network>::const_iterator network = m_blackList.begin();
             network != m_blackList.end();
             ++network)
          {
            std::cout << "  " << *network << std::endl;
          }
      }

    std::cerr << "Whitelisted networks: " << std::endl;
    for (std::vector<Network>::const_iterator network = m_whiteList.begin();
         network != m_whiteList.end();
         ++network)
      {
        std::cout << "  " << *network << std::endl;
      }

    m_faceMonitor.onNotification.connect(bind(&AutoregServer::onNotification, this, _1));
    m_faceMonitor.start();

    boost::asio::signal_set signalSet(m_face.getIoService(), SIGINT, SIGTERM);
    signalSet.async_wait(bind(&AutoregServer::signalHandler, this));

    m_face.processEvents();
  }


  void
  fetchFaceStatusSegments(const Data& data, const shared_ptr<ndn::OBufferStream>& buffer)
  {
    buffer->write(reinterpret_cast<const char*>(data.getContent().value()),
                  data.getContent().value_size());

    uint64_t currentSegment = data.getName().get(-1).toSegment();

    const name::Component& finalBlockId = data.getMetaInfo().getFinalBlockId();
    if (finalBlockId.empty() || finalBlockId.toSegment() > currentSegment) {
      m_face.expressInterest(data.getName().getPrefix(-1).appendSegment(currentSegment + 1),
                             bind(&AutoregServer::fetchFaceStatusSegments, this, _2, buffer),
                             ndn::OnTimeout());
    }
    else {
      return processFaceStatusDataset(buffer);
    }
  }

  void
  startFetchingFaceStatusDataset()
  {
    shared_ptr<ndn::OBufferStream> buffer = make_shared<ndn::OBufferStream>();

    Interest interest("/localhost/nfd/faces/list");
    interest.setChildSelector(1);
    interest.setMustBeFresh(true);

    m_face.expressInterest(interest,
                           bind(&AutoregServer::fetchFaceStatusSegments, this, _2, buffer),
                           ndn::OnTimeout());
  }

  void
  processFaceStatusDataset(const shared_ptr<ndn::OBufferStream>& buffer)
  {
    ndn::ConstBufferPtr buf = buffer->buf();
    std::vector<uint64_t> multicastFaces;

    size_t offset = 0;
    while (offset < buf->size()) {
      bool isOk = false;
      Block block;
      std::tie(isOk, block) = Block::fromBuffer(buf, offset);
      if (!isOk) {
        std::cerr << "ERROR: cannot decode FaceStatus TLV" << std::endl;
        break;
      }

      offset += block.size();

      nfd::FaceStatus faceStatus(block);
      registerPrefixesIfNeeded(faceStatus.getFaceId(), FaceUri(faceStatus.getRemoteUri()),
                               faceStatus.getFacePersistency());
    }
  }

  int
  main(int argc, char* argv[])
  {
    po::options_description optionDesciption;
    optionDesciption.add_options()
      ("help,h", "produce help message")
      ("prefix,i", po::value<std::vector<ndn::Name> >(&m_autoregPrefixes)->composing(),
       "prefix that should be automatically registered when new a remote non-local face is "
       "established")
      ("all-faces-prefix,a", po::value<std::vector<ndn::Name> >(&m_allFacesPrefixes)->composing(),
       "prefix that should be automatically registered for all TCP and UDP non-local faces "
       "(blacklists and whitelists do not apply to this prefix)")
      ("cost,c", po::value<uint64_t>(&m_cost)->default_value(255),
       "FIB cost which should be assigned to autoreg nexthops")
      ("whitelist,w", po::value<std::vector<Network> >(&m_whiteList)->composing(),
       "Whitelisted network, e.g., 192.168.2.0/24 or ::1/128")
      ("blacklist,b", po::value<std::vector<Network> >(&m_blackList)->composing(),
       "Blacklisted network, e.g., 192.168.2.32/30 or ::1/128")
      ("version,V", "show version and exit")
      ;

    po::variables_map options;
    try
      {
        po::store(po::command_line_parser(argc, argv).options(optionDesciption).run(), options);
        po::notify(options);
      }
    catch (std::exception& e)
      {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        usage(std::cerr, optionDesciption, argv[0]);
        return 1;
      }

    if (options.count("help"))
      {
        usage(std::cout, optionDesciption, argv[0]);
        return 0;
      }

    if (options.count("version"))
      {
        std::cout << NFD_VERSION_BUILD_STRING << std::endl;
        return 0;
      }

    if (m_autoregPrefixes.empty() && m_allFacesPrefixes.empty())
      {
        std::cerr << "ERROR: at least one --prefix or --all-faces-prefix must be specified"
                  << std::endl << std::endl;
        usage(std::cerr, optionDesciption, argv[0]);
        return 2;
      }

    if (m_whiteList.empty())
      {
        // Allow everything
        m_whiteList.push_back(Network::getMaxRangeV4());
        m_whiteList.push_back(Network::getMaxRangeV6());
      }

    try
      {
        startFetchingFaceStatusDataset();
        startProcessing();
      }
    catch (std::exception& e)
      {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 2;
      }

    return 0;
  }

private:
  Face m_face;
  KeyChain m_keyChain;
  Controller m_controller;
  FaceMonitor m_faceMonitor;
  std::vector<ndn::Name> m_autoregPrefixes;
  std::vector<ndn::Name> m_allFacesPrefixes;
  uint64_t m_cost;
  std::vector<Network> m_whiteList;
  std::vector<Network> m_blackList;
};

} // namespace nfd_autoreg
} // namespace ndn

int
main(int argc, char* argv[])
{
  ndn::nfd_autoreg::AutoregServer server;
  return server.main(argc, argv);
}

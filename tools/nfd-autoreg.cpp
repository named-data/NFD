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

#include "core/extended-error-message.hpp"
#include "core/network.hpp"
#include "core/version.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/mgmt/nfd/face-monitor.hpp>
#include <ndn-cxx/mgmt/nfd/face-status.hpp>
#include <ndn-cxx/net/address-converter.hpp>
#include <ndn-cxx/net/face-uri.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include <iostream>

namespace ndn {
namespace nfd_autoreg {

using ::nfd::Network;

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
                           const nfd::ControlResponse& response)
  {
    std::cerr << "FAILED: register " << prefix << " on face " << faceId
              << " (code: " << response.getCode() << ", reason: " << response.getText() << ")"
              << std::endl;
  }

  /**
   * \return true if uri has schema allowed to do auto-registrations
   */
  static bool
  hasAllowedSchema(const FaceUri& uri)
  {
    const std::string& scheme = uri.getScheme();
    return scheme == "udp4" || scheme == "tcp4" ||
           scheme == "udp6" || scheme == "tcp6";
  }

  /**
   * \return true if address is blacklisted
   */
  bool
  isBlacklisted(const boost::asio::ip::address& address) const
  {
    return std::any_of(m_blackList.begin(), m_blackList.end(),
                       bind(&Network::doesContain, _1, address));
  }

  /**
   * \return true if address is whitelisted
   */
  bool
  isWhitelisted(const boost::asio::ip::address& address) const
  {
    return std::any_of(m_whiteList.begin(), m_whiteList.end(),
                       bind(&Network::doesContain, _1, address));
  }

  void
  registerPrefixesForFace(uint64_t faceId, const std::vector<Name>& prefixes)
  {
    for (const Name& prefix : prefixes) {
      m_controller.start<nfd::RibRegisterCommand>(
        nfd::ControlParameters()
          .setName(prefix)
          .setFaceId(faceId)
          .setOrigin(nfd::ROUTE_ORIGIN_AUTOREG)
          .setCost(m_cost)
          .setExpirationPeriod(time::milliseconds::max()),
        bind(&AutoregServer::onRegisterCommandSuccess, this, faceId, prefix),
        bind(&AutoregServer::onRegisterCommandFailure, this, faceId, prefix, _1));
    }
  }

  void
  registerPrefixesIfNeeded(uint64_t faceId, const FaceUri& uri, nfd::FacePersistency facePersistency)
  {
    if (hasAllowedSchema(uri)) {
      boost::system::error_code ec;
      boost::asio::ip::address address = ndn::ip::addressFromString(uri.getHost(), ec);

      if (!address.is_multicast()) {
        // register all-face prefixes
        registerPrefixesForFace(faceId, m_allFacesPrefixes);

        // register autoreg prefixes if new face is on-demand and not blacklisted and whitelisted
        if (facePersistency == nfd::FACE_PERSISTENCY_ON_DEMAND &&
            !isBlacklisted(address) && isWhitelisted(address)) {
          registerPrefixesForFace(faceId, m_autoregPrefixes);
        }
      }
    }
  }

  void
  onNotification(const nfd::FaceEventNotification& notification)
  {
    if (notification.getKind() == nfd::FACE_EVENT_CREATED &&
        notification.getFaceScope() != nfd::FACE_SCOPE_LOCAL) {
      std::cerr << "PROCESSING: " << notification << std::endl;

      registerPrefixesIfNeeded(notification.getFaceId(), FaceUri(notification.getRemoteUri()),
                               notification.getFacePersistency());
    }
    else {
      std::cerr << "IGNORED: " << notification << std::endl;
    }
  }

  void
  signalHandler()
  {
    m_face.shutdown();
  }

  static void
  usage(std::ostream& os,
        const boost::program_options::options_description& desc,
        const char* programName)
  {
    os << "Usage: " << programName << " [--prefix=</autoreg/prefix>]... [options]\n"
       << "\n"
       << desc;
  }

  void
  startProcessing()
  {
    std::cerr << "AUTOREG prefixes: " << std::endl;
    for (const Name& prefix : m_autoregPrefixes) {
      std::cout << "  " << prefix << std::endl;
    }
    std::cerr << "ALL-FACES-AUTOREG prefixes: " << std::endl;
    for (const Name& prefix : m_allFacesPrefixes) {
      std::cout << "  " << prefix << std::endl;
    }

    if (!m_blackList.empty()) {
      std::cerr << "Blacklisted networks: " << std::endl;
      for (const Network& network : m_blackList) {
        std::cout << "  " << network << std::endl;
      }
    }

    std::cerr << "Whitelisted networks: " << std::endl;
    for (const Network& network : m_whiteList) {
      std::cout << "  " << network << std::endl;
    }

    m_faceMonitor.onNotification.connect(bind(&AutoregServer::onNotification, this, _1));
    m_faceMonitor.start();

    boost::asio::signal_set signalSet(m_face.getIoService(), SIGINT, SIGTERM);
    signalSet.async_wait(bind(&AutoregServer::signalHandler, this));

    m_face.processEvents();
  }

  void
  startFetchingFaceStatusDataset()
  {
    m_controller.fetch<nfd::FaceDataset>(
      [this] (const std::vector<nfd::FaceStatus>& faces) {
        for (const auto& faceStatus : faces) {
          registerPrefixesIfNeeded(faceStatus.getFaceId(), FaceUri(faceStatus.getRemoteUri()),
                                   faceStatus.getFacePersistency());
        }
      },
      [] (uint32_t code, const std::string& reason) {});
  }

  int
  main(int argc, char* argv[])
  {
    namespace po = boost::program_options;

    po::options_description optionsDesc("Options");
    optionsDesc.add_options()
      ("help,h", "print this message and exit")
      ("version,V", "show version information and exit")
      ("prefix,i", po::value<std::vector<Name>>(&m_autoregPrefixes)->composing(),
       "prefix that should be automatically registered when a new non-local face is created")
      ("all-faces-prefix,a", po::value<std::vector<Name>>(&m_allFacesPrefixes)->composing(),
       "prefix that should be automatically registered for all TCP and UDP non-local faces "
       "(blacklists and whitelists do not apply to this prefix)")
      ("cost,c", po::value<uint64_t>(&m_cost)->default_value(255),
       "FIB cost that should be assigned to autoreg nexthops")
      ("whitelist,w", po::value<std::vector<Network>>(&m_whiteList)->composing(),
       "Whitelisted network, e.g., 192.168.2.0/24 or ::1/128")
      ("blacklist,b", po::value<std::vector<Network>>(&m_blackList)->composing(),
       "Blacklisted network, e.g., 192.168.2.32/30 or ::1/128")
      ;

    po::variables_map options;
    try {
      po::store(po::parse_command_line(argc, argv, optionsDesc), options);
      po::notify(options);
    }
    catch (const std::exception& e) {
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
      usage(std::cerr, optionsDesc, argv[0]);
      return 2;
    }

    if (options.count("help") > 0) {
      usage(std::cout, optionsDesc, argv[0]);
      return 0;
    }

    if (options.count("version") > 0) {
      std::cout << NFD_VERSION_BUILD_STRING << std::endl;
      return 0;
    }

    if (m_autoregPrefixes.empty() && m_allFacesPrefixes.empty()) {
      std::cerr << "ERROR: at least one --prefix or --all-faces-prefix must be specified"
                << std::endl << std::endl;
      usage(std::cerr, optionsDesc, argv[0]);
      return 2;
    }

    if (m_whiteList.empty()) {
      // Allow everything
      m_whiteList.push_back(Network::getMaxRangeV4());
      m_whiteList.push_back(Network::getMaxRangeV6());
    }

    try {
      startFetchingFaceStatusDataset();
      startProcessing();
    }
    catch (const std::exception& e) {
      std::cerr << "ERROR: " << ::nfd::getExtendedErrorMessage(e) << std::endl;
      return 1;
    }

    return 0;
  }

private:
  Face m_face;
  KeyChain m_keyChain;
  nfd::Controller m_controller;
  nfd::FaceMonitor m_faceMonitor;
  std::vector<Name> m_autoregPrefixes;
  std::vector<Name> m_allFacesPrefixes;
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

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#include <ndn-cxx/management/nfd-controller.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "version.hpp"
#include "core/face-uri.hpp"
#include "core/face-monitor.hpp"
#include "network.hpp"

namespace po = boost::program_options;

namespace nfd {

using namespace ndn::nfd;
using ndn::Face;

class AutoregServer : boost::noncopyable
{
public:
  AutoregServer()
    : m_controller(m_face)
    , m_faceMonitor(m_face)
    , m_cost(255)
  {
  }

  void
  onRegisterCommandSuccess(const FaceEventNotification& notification, const Name& prefix)
  {
    std::cerr << "SUCCEED: register " << prefix << " on face "
              << notification.getFaceId() << std::endl;
  }

  void
  onRegisterCommandFailure(const FaceEventNotification& notification, const Name& prefix,
                           uint32_t code, const std::string& reason)
  {
    std::cerr << "FAILED: register " << prefix << " on face " << notification.getFaceId()
              << " (code: " << code << ", reason: " << reason << ")" << std::endl;
  }

  /**
   * \return true if auto-register should not be performed for uri
   */
  bool
  isFiltered(const FaceUri& uri)
  {
    const std::string& scheme = uri.getScheme();
    if (!(scheme == "udp4" || scheme == "tcp4" ||
          scheme == "udp6" || scheme == "tcp6"))
      return true;

    boost::asio::ip::address address = boost::asio::ip::address::from_string(uri.getHost());

    for (std::vector<Network>::const_iterator network = m_blackList.begin();
         network != m_blackList.end();
         ++network)
      {
        if (network->doesContain(address))
          return true;
      }

    for (std::vector<Network>::const_iterator network = m_whiteList.begin();
         network != m_whiteList.end();
         ++network)
      {
        if (network->doesContain(address))
          return false;
      }

    return true;
  }

  void
  processCreateFace(const FaceEventNotification& notification)
  {
    FaceUri uri(notification.getRemoteUri());

    if (isFiltered(uri))
      {
        std::cerr << "FILTERED: " << notification << std::endl;
        return;
      }

    std::cerr << "PROCESSING: " << notification << std::endl;
    for (std::vector<ndn::Name>::const_iterator prefix = m_autoregPrefixes.begin();
         prefix != m_autoregPrefixes.end();
         ++prefix)
      {
        m_controller.start<RibRegisterCommand>(
          ControlParameters()
            .setName(*prefix)
            .setFaceId(notification.getFaceId())
            .setOrigin(ROUTE_ORIGIN_AUTOREG)
            .setCost(m_cost)
            .setExpirationPeriod(time::milliseconds::max()),
          bind(&AutoregServer::onRegisterCommandSuccess, this, notification, *prefix),
          bind(&AutoregServer::onRegisterCommandFailure, this, notification, *prefix, _1, _2));
      }
  }

  void
  onNotification(const FaceEventNotification& notification)
  {
    if (notification.getKind() == FACE_EVENT_CREATED &&
        !notification.isLocal() &&
        notification.isOnDemand())
      {
        processCreateFace(notification);
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

    m_faceMonitor.onNotification += bind(&AutoregServer::onNotification, this, _1);
    m_faceMonitor.start();

    boost::asio::signal_set signalSet(m_face.getIoService(), SIGINT, SIGTERM);
    signalSet.async_wait(bind(&AutoregServer::signalHandler, this));

    m_face.processEvents();
  }

  int
  main(int argc, char* argv[])
  {
    po::options_description optionDesciption;
    optionDesciption.add_options()
      ("help,h", "produce help message")
      ("prefix,i", po::value<std::vector<ndn::Name> >(&m_autoregPrefixes)->composing(),
       "prefix that should be automatically registered when new remote face is established")
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

    if (m_autoregPrefixes.empty())
      {
        std::cerr << "ERROR: at least one --prefix must be specified" << std::endl << std::endl;
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
  Controller m_controller;
  FaceMonitor m_faceMonitor;
  std::vector<ndn::Name> m_autoregPrefixes;
  uint64_t m_cost;
  std::vector<Network> m_whiteList;
  std::vector<Network> m_blackList;
};

} // namespace nfd

int
main(int argc, char* argv[])
{
  nfd::AutoregServer server;
  return server.main(argc, argv);
}

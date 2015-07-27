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

#include <ndn-cxx/util/network-monitor.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/scheduler-scoped-event-id.hpp>

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

  explicit
  NdnAutoconfig(bool isDaemonMode)
    : m_face(m_io)
    , m_scheduler(m_io)
    , m_startStagesEvent(m_scheduler)
    , m_isDaemonMode(isDaemonMode)
    , m_terminationSignalSet(m_io)
    , m_stage1(m_face, m_keyChain,
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
                 if (!m_isDaemonMode)
                   BOOST_THROW_EXCEPTION(Error("No more stages, automatic discovery failed"));
                 else
                   std::cerr << "No more stages, automatic discovery failed" << std::endl;
               })
  {
    if (m_isDaemonMode) {
      m_networkMonitor.reset(new util::NetworkMonitor(m_io));
      m_networkMonitor->onNetworkStateChanged.connect([this] {
          // delay stages, so if multiple events are triggered in short sequence,
          // only one auto-detection procedure is triggered
          m_startStagesEvent = m_scheduler.scheduleEvent(time::seconds(5),
                                                         bind(&NdnAutoconfig::startStages, this));
        });
    }

    // Delay a little bit
    m_startStagesEvent = m_scheduler.scheduleEvent(time::milliseconds(100),
                                                   bind(&NdnAutoconfig::startStages, this));
  }

  void
  run()
  {
    if (m_isDaemonMode) {
      m_terminationSignalSet.add(SIGINT);
      m_terminationSignalSet.add(SIGTERM);
      m_terminationSignalSet.async_wait(bind(&NdnAutoconfig::terminate, this, _1, _2));
    }

    m_io.run();
  }

  void
  terminate(const boost::system::error_code& error, int signalNo)
  {
    if (error)
      return;

    m_io.stop();
  }

  static void
  usage(const char* programName)
  {
    std::cout << "Usage:\n"
              << "  " << programName  << " [options]\n"
              << "\n"
              << "Options:\n"
              << "  [-h]  - print usage and exit\n"
              << "  [-d]  - run in daemon mode.  In daemon mode, " << programName << " will try \n"
              << "          to detect network change events and re-run auto-discovery procedure.\n"
              << "          In addition, the auto-discovery procedure is unconditionally re-run\n"
              << "          every hour.\n"
              << "          NOTE: if connection to NFD fails, the daemon will be terminated.\n"
              << "  [-V]  - print version number and exit\n"
              << std::endl;
  }

private:
  void
  startStages()
  {
    m_stage1.start();
    if (m_isDaemonMode) {
      m_startStagesEvent = m_scheduler.scheduleEvent(time::hours(1),
                                                     bind(&NdnAutoconfig::startStages, this));
    }
  }

private:
  boost::asio::io_service m_io;
  Face m_face;
  KeyChain m_keyChain;
  unique_ptr<util::NetworkMonitor> m_networkMonitor;
  util::Scheduler m_scheduler;
  util::scheduler::ScopedEventId m_startStagesEvent;
  bool m_isDaemonMode;
  boost::asio::signal_set m_terminationSignalSet;

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
  bool isDaemonMode = false;

  while ((opt = getopt(argc, argv, "dhV")) != -1) {
    switch (opt) {
    case 'd':
      isDaemonMode = true;
      break;
    case 'h':
      ndn::tools::NdnAutoconfig::usage(programName);
      return 0;
    case 'V':
      std::cout << NFD_VERSION_BUILD_STRING << std::endl;
      return 0;
    }
  }

  try {
    ndn::tools::NdnAutoconfig autoConfigInstance(isDaemonMode);
    autoConfigInstance.run();
  }
  catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << std::endl;
    return 1;
  }
  return 0;
}

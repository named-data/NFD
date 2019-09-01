/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "nfd.hpp"
#include "rib/service.hpp"

#include "common/global.hpp"
#include "common/logger.hpp"
#include "common/privilege-helper.hpp"
#include "core/version.hpp"

#include <string.h> // for strsignal()

#include <boost/config.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/version.hpp>

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <thread>

#include <ndn-cxx/util/logging.hpp>
#include <ndn-cxx/util/ostream-joiner.hpp>
#include <ndn-cxx/version.hpp>

#ifdef HAVE_LIBPCAP
#include <pcap/pcap.h>
#endif
#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif
#ifdef HAVE_WEBSOCKET
#include <websocketpp/version.hpp>
#endif

namespace po = boost::program_options;

NFD_LOG_INIT(Main);

namespace nfd {

/** \brief Executes NFD with RIB manager
 *
 *  NFD (main forwarding procedure) and RIB manager execute in two different threads.
 *  Each thread has its own instances of global io_service and global scheduler.
 *
 *  When either of the daemons fails, execution of non-failed daemon will be terminated as
 *  well.  In other words, when NFD fails, RIB manager will be terminated; when RIB manager
 *  fails, NFD will be terminated.
 */
class NfdRunner : noncopyable
{
public:
  explicit
  NfdRunner(const std::string& configFile)
    : m_nfd(configFile, m_nfdKeyChain)
    , m_configFile(configFile)
    , m_terminationSignalSet(getGlobalIoService())
    , m_reloadSignalSet(getGlobalIoService())
  {
    m_terminationSignalSet.add(SIGINT);
    m_terminationSignalSet.add(SIGTERM);
    m_terminationSignalSet.async_wait(bind(&NfdRunner::terminate, this, _1, _2));

    m_reloadSignalSet.add(SIGHUP);
    m_reloadSignalSet.async_wait(bind(&NfdRunner::reload, this, _1, _2));
  }

  void
  initialize()
  {
    m_nfd.initialize();
  }

  int
  run()
  {
    // Return value: a non-zero value is assigned when either NFD or RIB manager (running in
    // a separate thread) fails.
    std::atomic_int retval(0);

    boost::asio::io_service* const mainIo = &getGlobalIoService();
    setMainIoService(mainIo);
    boost::asio::io_service* ribIo = nullptr;

    // Mutex and conditional variable to implement synchronization between main and RIB manager
    // threads:
    // - to block main thread until RIB manager thread starts and initializes ribIo (to allow
    //   stopping it later)
    std::mutex m;
    std::condition_variable cv;

    std::thread ribThread([configFile = m_configFile, &retval, &ribIo, mainIo, &cv, &m] {
      {
        std::lock_guard<std::mutex> lock(m);
        ribIo = &getGlobalIoService();
        BOOST_ASSERT(ribIo != mainIo);
        setRibIoService(ribIo);
      }
      cv.notify_all(); // notify that ribIo has been assigned

      try {
        ndn::KeyChain ribKeyChain;
        // must be created inside a separate thread
        rib::Service ribService(configFile, ribKeyChain);
        getGlobalIoService().run(); // ribIo is not thread-safe to use here
      }
      catch (const std::exception& e) {
        NFD_LOG_FATAL(boost::diagnostic_information(e));
        retval = 1;
        mainIo->stop();
      }

      {
        std::lock_guard<std::mutex> lock(m);
        ribIo = nullptr;
      }
    });

    {
      // Wait to guarantee that ribIo is properly initialized, so it can be used to terminate
      // RIB manager thread.
      std::unique_lock<std::mutex> lock(m);
      cv.wait(lock, [&ribIo] { return ribIo != nullptr; });
    }

    try {
      systemdNotify("READY=1");
      mainIo->run();
    }
    catch (const std::exception& e) {
      NFD_LOG_FATAL(boost::diagnostic_information(e));
      retval = 1;
    }
    catch (const PrivilegeHelper::Error& e) {
      NFD_LOG_FATAL(e.what());
      retval = 4;
    }

    {
      // ribIo is guaranteed to be alive at this point
      std::lock_guard<std::mutex> lock(m);
      if (ribIo != nullptr) {
        ribIo->stop();
        ribIo = nullptr;
      }
    }
    ribThread.join();

    return retval;
  }

  static void
  systemdNotify(const char* state)
  {
#ifdef HAVE_SYSTEMD
    sd_notify(0, state);
#endif
  }

private:
  void
  terminate(const boost::system::error_code& error, int signalNo)
  {
    if (error)
      return;

    NFD_LOG_INFO("Caught signal " << signalNo << " (" << ::strsignal(signalNo) << "), exiting...");

    systemdNotify("STOPPING=1");
    getGlobalIoService().stop();
  }

  void
  reload(const boost::system::error_code& error, int signalNo)
  {
    if (error)
      return;

    NFD_LOG_INFO("Caught signal " << signalNo << " (" << ::strsignal(signalNo) << "), reloading...");

    systemdNotify("RELOADING=1");
    m_nfd.reloadConfigFile();
    systemdNotify("READY=1");

    m_reloadSignalSet.async_wait(bind(&NfdRunner::reload, this, _1, _2));
  }

private:
  ndn::KeyChain           m_nfdKeyChain;
  Nfd                     m_nfd;
  std::string             m_configFile;

  boost::asio::signal_set m_terminationSignalSet;
  boost::asio::signal_set m_reloadSignalSet;
};

static void
printUsage(std::ostream& os, const char* programName, const po::options_description& opts)
{
  os << "Usage: " << programName << " [options]\n"
     << "\n"
     << "Run the NDN Forwarding Daemon (NFD)\n"
     << "\n"
     << opts;
}

static void
printLogModules(std::ostream& os)
{
  const auto& modules = ndn::util::Logging::getLoggerNames();
  std::copy(modules.begin(), modules.end(), ndn::make_ostream_joiner(os, "\n"));
  os << std::endl;
}

} // namespace nfd

int
main(int argc, char** argv)
{
  using namespace nfd;

  std::string configFile = DEFAULT_CONFIG_FILE;

  po::options_description description("Options");
  description.add_options()
    ("help,h",    "print this message and exit")
    ("version,V", "show version information and exit")
    ("config,c",  po::value<std::string>(&configFile),
                  "path to configuration file (default: " DEFAULT_CONFIG_FILE ")")
    ("modules,m", "list available logging modules")
    ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, description), vm);
    po::notify(vm);
  }
  catch (const std::exception& e) {
    // Cannot use NFD_LOG_* macros here, because the logging subsystem is not initialized yet
    // at this point. Moreover, we don't want to clutter error messages related to command-line
    // parsing with timestamps and other useless text added by the macros.
    std::cerr << "ERROR: " << e.what() << "\n\n";
    printUsage(std::cerr, argv[0], description);
    return 2;
  }

  if (vm.count("help") > 0) {
    printUsage(std::cout, argv[0], description);
    return 0;
  }

  if (vm.count("version") > 0) {
    std::cout << NFD_VERSION_BUILD_STRING << std::endl;
    return 0;
  }

  if (vm.count("modules") > 0) {
    printLogModules(std::cout);
    return 0;
  }

  const std::string boostBuildInfo =
      "with Boost version " + to_string(BOOST_VERSION / 100000) +
      "." + to_string(BOOST_VERSION / 100 % 1000) +
      "." + to_string(BOOST_VERSION % 100);
  const std::string pcapBuildInfo =
#ifdef HAVE_LIBPCAP
      "with " + std::string(pcap_lib_version());
#else
      "without libpcap";
#endif
  const std::string wsBuildInfo =
#ifdef HAVE_WEBSOCKET
      "with WebSocket++ version " + to_string(websocketpp::major_version) +
      "." + to_string(websocketpp::minor_version) +
      "." + to_string(websocketpp::patch_version);
#else
      "without WebSocket++";
#endif

  std::clog << "NFD version " << NFD_VERSION_BUILD_STRING << " starting\n"
            << "Built with " BOOST_COMPILER ", with " BOOST_STDLIB
               ", " << boostBuildInfo <<
               ", " << pcapBuildInfo <<
               ", " << wsBuildInfo <<
               ", with ndn-cxx version " NDN_CXX_VERSION_BUILD_STRING
            << std::endl;

  NfdRunner runner(configFile);
  try {
    runner.initialize();
  }
  catch (const boost::filesystem::filesystem_error& e) {
    NFD_LOG_FATAL(boost::diagnostic_information(e));
    return e.code() == boost::system::errc::permission_denied ? 4 : 1;
  }
  catch (const std::exception& e) {
    NFD_LOG_FATAL(boost::diagnostic_information(e));
    return 1;
  }
  catch (const PrivilegeHelper::Error& e) {
    // PrivilegeHelper::Errors do not inherit from std::exception
    // and represent seteuid/gid failures
    NFD_LOG_FATAL(e.what());
    return 4;
  }

  return runner.run();
}

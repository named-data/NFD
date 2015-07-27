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

#include "nfd.hpp"
#include "rib/nrd.hpp"

#include "version.hpp"
#include "core/global-io.hpp"
#include "core/logger.hpp"
#include "core/privilege-helper.hpp"

#include <string.h>

#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

// boost::thread is used instead of std::thread to guarantee proper cleanup of thread local storage,
// see http://www.boost.org/doc/libs/1_48_0/doc/html/thread/thread_local_storage.html
#include <boost/thread.hpp>

#include <atomic>
#include <condition_variable>

namespace nfd {

NFD_LOG_INIT("NFD");

/** \brief Executes NFD with RIB manager
 *
 *  NFD (main forwarding procedure) and RIB manager execute in two different threads.
 *  Each thread has its own instances global io_service and global scheduler.
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

  static void
  printUsage(std::ostream& os, const std::string& programName)
  {
    os << "Usage: \n"
       << "  " << programName << " [options]\n"
       << "\n"
       << "Run NFD forwarding daemon\n"
       << "\n"
       << "Options:\n"
       << "  [--help]    - print this help message\n"
       << "  [--version] - print version and exit\n"
       << "  [--modules] - list available logging modules\n"
       << "  [--config /path/to/nfd.conf] - path to configuration file "
       << "(default: " << DEFAULT_CONFIG_FILE << ")\n"
      ;
  }

  static void
  printModules(std::ostream& os)
  {
    os << "Available logging modules: \n";

    for (const auto& module : LoggerFactory::getInstance().getModules()) {
      os << module << "\n";
    }
  }

  void
  initialize()
  {
    m_nfd.initialize();
  }

  int
  run()
  {
    /** \brief return value
     *  A non-zero value is assigned when either NFD or RIB manager (running in a separate
     *  thread) fails.
     */
    std::atomic_int retval(0);

    boost::asio::io_service* const mainIo = &getGlobalIoService();
    boost::asio::io_service* nrdIo = nullptr;

    // Mutex and conditional variable to implement synchronization between main and RIB manager
    // threads:
    // - to block main thread until RIB manager thread starts and initializes nrdIo (to allow
    //   stopping it later)
    std::mutex m;
    std::condition_variable cv;

    std::string configFile = this->m_configFile; // c++11 lambda cannot capture member variables
    boost::thread nrdThread([configFile, &retval, &nrdIo, mainIo, &cv, &m] {
        {
          std::lock_guard<std::mutex> lock(m);
          nrdIo = &getGlobalIoService();
          BOOST_ASSERT(nrdIo != mainIo);
        }
        cv.notify_all(); // notify that nrdIo has been assigned

        try {
          ndn::KeyChain nrdKeyChain;
          // must be created inside a separate thread
          rib::Nrd nrd(configFile, nrdKeyChain);
          nrd.initialize();
          getGlobalIoService().run(); // nrdIo is not thread-safe to use here
        }
        catch (const std::exception& e) {
          NFD_LOG_FATAL(e.what());
          retval = 6;
          mainIo->stop();
        }

        {
          std::lock_guard<std::mutex> lock(m);
          nrdIo = nullptr;
        }
      });

    {
      // Wait to guarantee that nrdIo is properly initialized, so it can be used to terminate
      // RIB manager thread.
      std::unique_lock<std::mutex> lock(m);
      cv.wait(lock, [&nrdIo] { return nrdIo != nullptr; });
    }

    try {
      mainIo->run();
    }
    catch (const std::exception& e) {
      NFD_LOG_FATAL(e.what());
      retval = 4;
    }
    catch (const PrivilegeHelper::Error& e) {
      NFD_LOG_FATAL(e.what());
      retval = 5;
    }

    {
      // nrdIo is guaranteed to be alive at this point
      std::lock_guard<std::mutex> lock(m);
      if (nrdIo != nullptr) {
        nrdIo->stop();
        nrdIo = nullptr;
      }
    }
    nrdThread.join();

    return retval;
  }

  void
  terminate(const boost::system::error_code& error, int signalNo)
  {
    if (error)
      return;

    NFD_LOG_INFO("Caught signal '" << ::strsignal(signalNo) << "', exiting...");
    getGlobalIoService().stop();
  }

  void
  reload(const boost::system::error_code& error, int signalNo)
  {
    if (error)
      return;

    NFD_LOG_INFO("Caught signal '" << ::strsignal(signalNo) << "', reloading...");
    m_nfd.reloadConfigFile();

    m_reloadSignalSet.async_wait(bind(&NfdRunner::reload, this, _1, _2));
  }

private:
  ndn::KeyChain           m_nfdKeyChain;
  Nfd                     m_nfd;
  std::string             m_configFile;

  boost::asio::signal_set m_terminationSignalSet;
  boost::asio::signal_set m_reloadSignalSet;
};

} // namespace nfd

int
main(int argc, char** argv)
{
  using namespace nfd;

  namespace po = boost::program_options;

  po::options_description description;

  std::string configFile = DEFAULT_CONFIG_FILE;
  description.add_options()
    ("help,h",    "print this help message")
    ("version,V", "print version and exit")
    ("modules,m", "list available logging modules")
    ("config,c",  po::value<std::string>(&configFile), "path to configuration file")
    ;

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(description).run(), vm);
    po::notify(vm);
  }
  catch (const std::exception& e) {
    // avoid NFD_LOG_FATAL to ensure that errors related to command-line parsing always appear on the
    // terminal and are not littered with timestamps and other things added by the logging subsystem
    std::cerr << "ERROR: " << e.what() << std::endl;
    NfdRunner::printUsage(std::cerr, argv[0]);
    return 1;
  }

  if (vm.count("help") > 0) {
    NfdRunner::printUsage(std::cout, argv[0]);
    return 0;
  }

  if (vm.count("version") > 0) {
    std::cout << NFD_VERSION_BUILD_STRING << std::endl;
    return 0;
  }

  if (vm.count("modules") > 0) {
    NfdRunner::printModules(std::cout);
    return 0;
  }

  NfdRunner runner(configFile);

  try {
    runner.initialize();
  }
  catch (const boost::filesystem::filesystem_error& e) {
    if (e.code() == boost::system::errc::permission_denied) {
      NFD_LOG_FATAL("Permissions denied for " << e.path1() << ". " <<
                    argv[0] << " should be run as superuser");
      return 3;
    }
    else {
      NFD_LOG_FATAL(e.what());
      return 2;
    }
  }
  catch (const std::exception& e) {
    NFD_LOG_FATAL(e.what());
    return 2;
  }
  catch (const PrivilegeHelper::Error& e) {
    // PrivilegeHelper::Errors do not inherit from std::exception
    // and represent seteuid/gid failures

    NFD_LOG_FATAL(e.what());
    return 3;
  }

  return runner.run();
}

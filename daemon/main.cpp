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

#include <getopt.h>
#include <boost/filesystem.hpp>

#include "core/logger.hpp"
#include "core/global-io.hpp"
#include "fw/forwarder.hpp"
#include "mgmt/internal-face.hpp"
#include "mgmt/fib-manager.hpp"
#include "mgmt/face-manager.hpp"
#include "mgmt/strategy-choice-manager.hpp"
#include "mgmt/status-server.hpp"
#include "core/config-file.hpp"

namespace nfd {

NFD_LOG_INIT("NFD");

struct ProgramOptions
{
  bool showUsage;
  bool showModules;
  std::string config;
};

class Nfd : noncopyable
{
public:

  void
  initialize(const std::string& configFile)
  {
    initializeLogging(configFile);

    m_forwarder = make_shared<Forwarder>();

    initializeManagement(configFile);
  }


  void
  initializeLogging(const std::string& configFile)
  {
    ConfigFile config;
    LoggerFactory::getInstance().setConfigFile(config);

    for (size_t i = 0; i < N_SUPPORTED_CONFIG_SECTIONS; ++i)
      {
        if (SUPPORTED_CONFIG_SECTIONS[i] != "log")
          {
            config.addSectionHandler(SUPPORTED_CONFIG_SECTIONS[i],
                                     bind(std::plus<int>(), 0, 0)); // no-op.
          }
      }

    config.parse(configFile, true);
    config.parse(configFile, false);
  }

  void
  initializeManagement(const std::string& configFile)
  {
    m_internalFace = make_shared<InternalFace>();

    m_fibManager = make_shared<FibManager>(boost::ref(m_forwarder->getFib()),
                                           bind(&Forwarder::getFace, m_forwarder.get(), _1),
                                           m_internalFace);

    m_faceManager = make_shared<FaceManager>(boost::ref(m_forwarder->getFaceTable()),
                                             m_internalFace);

    m_strategyChoiceManager =
      make_shared<StrategyChoiceManager>(boost::ref(m_forwarder->getStrategyChoice()),
                                         m_internalFace);

    m_statusServer = make_shared<StatusServer>(m_internalFace,
                                               boost::ref(*m_forwarder));

    ConfigFile config;
    m_internalFace->getValidator().setConfigFile(config);

    m_forwarder->addFace(m_internalFace);

    m_faceManager->setConfigFile(config);

    config.addSectionHandler("log", bind(std::plus<int>(), 0, 0)); // no-op

    // parse config file
    config.parse(configFile, true);
    config.parse(configFile, false);

    // add FIB entry for NFD Management Protocol
    shared_ptr<fib::Entry> entry = m_forwarder->getFib().insert("/localhost/nfd").first;
    entry->addNextHop(m_internalFace, 0);
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
       << "  [--help]   - print this help message\n"
       << "  [--modules] - list available logging modules\n"
       << "  [--config /path/to/nfd.conf] - path to configuration file "
       << "(default: " << DEFAULT_CONFIG_FILE << ")\n"
      ;
  }

  static void
  printModules(std::ostream& os)
  {
    using namespace std;

    os << "Available logging modules: \n";

    list<string> modules(LoggerFactory::getInstance().getModules());
    for (list<string>::const_iterator i = modules.begin(); i != modules.end(); ++i)
      {
        os << *i << "\n";
      }
  }

  static bool
  parseCommandLine(int argc, char** argv, ProgramOptions& options)
  {
    options.showUsage = false;
    options.showModules = false;
    options.config = DEFAULT_CONFIG_FILE;

    while (true) {
      int optionIndex = 0;
      static ::option longOptions[] = {
        { "help"   , no_argument      , 0, 0 },
        { "modules", no_argument      , 0, 0 },
        { "config" , required_argument, 0, 0 },
        { 0        , 0                , 0, 0 }
      };
      int c = getopt_long_only(argc, argv, "", longOptions, &optionIndex);
      if (c == -1)
        break;

      switch (c) {
        case 0:
          switch (optionIndex) {
            case 0: // help
              options.showUsage = true;
              break;
            case 1: // modules
              options.showModules = true;
              break;
            case 2: // config
              options.config = ::optarg;
              break;
            default:
              return false;
          }
          break;
      }
    }
    return true;
  }

  void
  terminate(const boost::system::error_code& error,
            int signalNo,
            boost::asio::signal_set& signalSet)
  {
    if (error)
      return;

    if (signalNo == SIGINT ||
        signalNo == SIGTERM)
      {
        getGlobalIoService().stop();
        NFD_LOG_INFO("Caught signal '" << strsignal(signalNo) << "', exiting...");
      }
    else
      {
        /// \todo May be try to reload config file (at least security section)
        signalSet.async_wait(bind(&Nfd::terminate, this, _1, _2,
                                  boost::ref(signalSet)));
      }
  }

private:
  shared_ptr<Forwarder> m_forwarder;

  shared_ptr<InternalFace>          m_internalFace;
  shared_ptr<FibManager>            m_fibManager;
  shared_ptr<FaceManager>           m_faceManager;
  shared_ptr<StrategyChoiceManager> m_strategyChoiceManager;
  shared_ptr<StatusServer>          m_statusServer;

  static const std::string SUPPORTED_CONFIG_SECTIONS[];
  static const size_t      N_SUPPORTED_CONFIG_SECTIONS;
};

const std::string Nfd::SUPPORTED_CONFIG_SECTIONS[] =
  {
    "log",
    "face_system",
    "authorizations",
    "rib_security",
  };

const size_t Nfd::N_SUPPORTED_CONFIG_SECTIONS =
  sizeof(SUPPORTED_CONFIG_SECTIONS) / sizeof(std::string);

} // namespace nfd

int
main(int argc, char** argv)
{
  using namespace nfd;

  ProgramOptions options;
  bool isCommandLineValid = Nfd::parseCommandLine(argc, argv, options);
  if (!isCommandLineValid) {
    Nfd::printUsage(std::cerr, argv[0]);
    return 1;
  }
  if (options.showUsage) {
    Nfd::printUsage(std::cout, argv[0]);
    return 0;
  }

  if (options.showModules) {
    Nfd::printModules(std::cout);
    return 0;
  }

  Nfd nfdInstance;

  try {
    nfdInstance.initialize(options.config);
  }
  catch (boost::filesystem::filesystem_error& e) {
    if (e.code() == boost::system::errc::permission_denied) {
      NFD_LOG_FATAL("Permissions denied for " << e.path1() << ". " <<
                    argv[0] << " should be run as superuser");
    }
    else {
      NFD_LOG_FATAL(e.what());
    }
    return 1;
  }
  catch (const std::exception& e) {
    NFD_LOG_FATAL(e.what());
    return 2;
  }

  boost::asio::signal_set signalSet(getGlobalIoService());
  signalSet.add(SIGINT);
  signalSet.add(SIGTERM);
  signalSet.add(SIGHUP);
  signalSet.add(SIGUSR1);
  signalSet.add(SIGUSR2);
  signalSet.async_wait(bind(&Nfd::terminate, &nfdInstance, _1, _2,
                            boost::ref(signalSet)));

  try {
    getGlobalIoService().run();
  }
  catch (std::exception& e) {
    NFD_LOG_FATAL(e.what());
    return 3;
  }

  return 0;
}

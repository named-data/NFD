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

#include <getopt.h>
#include <boost/filesystem.hpp>

#include "version.hpp"
#include "core/logger.hpp"
#include "core/global-io.hpp"
#include "core/privilege-helper.hpp"
#include "fw/forwarder.hpp"
#include "face/null-face.hpp"
#include "mgmt/internal-face.hpp"
#include "mgmt/fib-manager.hpp"
#include "mgmt/face-manager.hpp"
#include "mgmt/strategy-choice-manager.hpp"
#include "mgmt/status-server.hpp"
#include "core/config-file.hpp"
#include "mgmt/general-config-section.hpp"
#include "mgmt/tables-config-section.hpp"

namespace nfd {

NFD_LOG_INIT("NFD");

struct ProgramOptions
{
  bool showUsage;
  bool showVersion;
  bool showModules;
  std::string config;
};

class Nfd : noncopyable
{
public:
  explicit
  Nfd(const std::string& configFile)
    : m_configFile(configFile)
    , m_originalStreamBuf(0)
  {
  }

  ~Nfd()
  {
    if (static_cast<bool>(m_originalStreamBuf)) {
      std::clog.rdbuf(m_originalStreamBuf);
    }
  }

  void
  initialize()
  {
    initializeLogging();

    m_forwarder = make_shared<Forwarder>();

    initializeManagement();

    m_forwarder->getFaceTable().addReserved(make_shared<NullFace>(), FACEID_NULL);
    m_forwarder->getFaceTable().addReserved(
      make_shared<NullFace>(FaceUri("contentstore://")), FACEID_CONTENT_STORE);

    PrivilegeHelper::drop();
  }


  void
  initializeLogging()
  {
    ConfigFile config(&ConfigFile::ignoreUnknownSection);
    LoggerFactory::getInstance().setConfigFile(config);

    config.parse(m_configFile, true);
    config.parse(m_configFile, false);
  }

  class IgnoreRibAndLogSections
  {
  public:
    void
    operator()(const std::string& filename,
               const std::string& sectionName,
               const ConfigSection& section,
               bool isDryRun)

    {
      // Ignore "log" and sections beginning with "rib_" (intended for rib manager),
      // but raise an error if we're missing a handler for an NFD section.

      if (sectionName.find("rib") == 0 || sectionName == "log")
        {
          // do nothing
        }
      else
        {
          // missing NFD section
          ConfigFile::throwErrorOnUnknownSection(filename, sectionName, section, isDryRun);
        }
    }
  };

  void
  initializeManagement()
  {
    m_internalFace = make_shared<InternalFace>();

    m_fibManager = make_shared<FibManager>(ref(m_forwarder->getFib()),
                                           bind(&Forwarder::getFace, m_forwarder.get(), _1),
                                           m_internalFace,
                                           ndn::ref(m_keyChain));

    m_faceManager = make_shared<FaceManager>(ref(m_forwarder->getFaceTable()),
                                             m_internalFace,
                                             ndn::ref(m_keyChain));

    m_strategyChoiceManager =
      make_shared<StrategyChoiceManager>(ref(m_forwarder->getStrategyChoice()),
                                         m_internalFace,
                                         ndn::ref(m_keyChain));

    m_statusServer = make_shared<StatusServer>(m_internalFace,
                                               ref(*m_forwarder),
                                               ndn::ref(m_keyChain));

    ConfigFile config((IgnoreRibAndLogSections()));
    general::setConfigFile(config);

    TablesConfigSection tablesConfig(m_forwarder->getCs(),
                                     m_forwarder->getPit(),
                                     m_forwarder->getFib(),
                                     m_forwarder->getStrategyChoice(),
                                     m_forwarder->getMeasurements());
    tablesConfig.setConfigFile(config);

    m_internalFace->getValidator().setConfigFile(config);

    m_forwarder->getFaceTable().addReserved(m_internalFace, FACEID_INTERNAL_FACE);

    m_faceManager->setConfigFile(config);

    // parse config file
    config.parse(m_configFile, true);
    config.parse(m_configFile, false);

    tablesConfig.ensureTablesAreConfigured();

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
    options.showVersion = false;
    options.showModules = false;
    options.config = DEFAULT_CONFIG_FILE;

    while (true) {
      int optionIndex = 0;
      static ::option longOptions[] = {
        { "help"   , no_argument      , 0, 0 },
        { "modules", no_argument      , 0, 0 },
        { "config" , required_argument, 0, 0 },
        { "version", no_argument      , 0, 0 },
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
        case 3: // version
          options.showVersion = true;
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
        signalSet.async_wait(bind(&Nfd::terminate, this, _1, _2, ref(signalSet)));
      }
  }

  void
  reload(const boost::system::error_code& error,
         int signalNo,
         boost::asio::signal_set& signalSet)
  {
    if (error)
      return;

    NFD_LOG_INFO("Caught signal '" << strsignal(signalNo));

    ////////////////////////
    // Reload config file //
    ////////////////////////

    // Logging
    initializeLogging();
    /// \todo Reopen log file

    // Other stuff
    ConfigFile config((IgnoreRibAndLogSections()));

    general::setConfigFile(config);

    TablesConfigSection tablesConfig(m_forwarder->getCs(),
                                     m_forwarder->getPit(),
                                     m_forwarder->getFib(),
                                     m_forwarder->getStrategyChoice(),
                                     m_forwarder->getMeasurements());

    tablesConfig.setConfigFile(config);

    m_internalFace->getValidator().setConfigFile(config);
    m_faceManager->setConfigFile(config);

    config.parse(m_configFile, false);

    ////////////////////////

    signalSet.async_wait(bind(&Nfd::reload, this, _1, _2, ref(signalSet)));
  }

private:
  std::string m_configFile;

  shared_ptr<Forwarder> m_forwarder;

  shared_ptr<InternalFace>          m_internalFace;
  shared_ptr<FibManager>            m_fibManager;
  shared_ptr<FaceManager>           m_faceManager;
  shared_ptr<StrategyChoiceManager> m_strategyChoiceManager;
  shared_ptr<StatusServer>          m_statusServer;

  shared_ptr<std::ofstream>         m_logFile;
  std::basic_streambuf<char>*       m_originalStreamBuf;
  ndn::KeyChain                     m_keyChain;
};

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

  if (options.showVersion) {
    std::cout << NFD_VERSION_BUILD_STRING << std::endl;
    return 0;
  }

  if (options.showModules) {
    Nfd::printModules(std::cout);
    return 0;
  }

  Nfd nfdInstance(options.config);

  try {
    nfdInstance.initialize();
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
  catch (const PrivilegeHelper::Error& e) {
    // PrivilegeHelper::Errors do not inherit from std::exception
    // and represent seteuid/gid failures

    NFD_LOG_FATAL(e.what());
    return 3;
  }

  boost::asio::signal_set terminationSignalSet(getGlobalIoService());
  terminationSignalSet.add(SIGINT);
  terminationSignalSet.add(SIGTERM);
  terminationSignalSet.async_wait(bind(&Nfd::terminate, &nfdInstance, _1, _2,
                                       ref(terminationSignalSet)));

  boost::asio::signal_set reloadSignalSet(getGlobalIoService());
  reloadSignalSet.add(SIGHUP);
  reloadSignalSet.async_wait(bind(&Nfd::reload, &nfdInstance, _1, _2,
                                  ref(reloadSignalSet)));

  try {
    getGlobalIoService().run();
  }
  catch (const std::exception& e) {
    NFD_LOG_FATAL(e.what());
    return 4;
  }
  catch (const PrivilegeHelper::Error& e) {
    NFD_LOG_FATAL(e.what());
    return 5;
  }

  return 0;
}

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

#include "version.hpp"
#include "common.hpp"
#include "rib-manager.hpp"
#include "core/config-file.hpp"
#include "core/global-io.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace rib {

NFD_LOG_INIT("NRD");

struct ProgramOptions
{
  bool showUsage;
  bool showVersion;
  bool showModules;
  std::string config;
};

class Nrd : noncopyable
{
public:
  class IgnoreNfdAndLogSections
  {
  public:
    void
    operator()(const std::string& filename,
               const std::string& sectionName,
               const ConfigSection& section,
               bool isDryRun)
    {
      // Ignore "log" and sections belonging to NFD,
      // but raise an error if we're missing a handler for an "rib_" section.

      if (sectionName.find("rib_") != 0 || sectionName == "log")
        {
          // do nothing
        }
      else
        {
          // missing NRD section
          ConfigFile::throwErrorOnUnknownSection(filename, sectionName, section, isDryRun);
        }
    }
  };

  Nrd()
    : m_face(getGlobalIoService())
  {
  }

  void
  initialize(const std::string& configFile)
  {
    initializeLogging(configFile);

    m_ribManager = make_shared<RibManager>(ndn::ref(m_face));

    ConfigFile config((IgnoreNfdAndLogSections()));
    m_ribManager->setConfigFile(config);

    // parse config file
    config.parse(configFile, true);
    config.parse(configFile, false);

    m_ribManager->registerWithNfd();
    m_ribManager->enableLocalControlHeader();
  }

  void
  initializeLogging(const std::string& configFile)
  {
    ConfigFile config(&ConfigFile::ignoreUnknownSection);
    LoggerFactory::getInstance().setConfigFile(config);

    config.parse(configFile, true);
    config.parse(configFile, false);
  }

  static void
  printUsage(std::ostream& os, const std::string& programName)
  {
    os << "Usage: \n"
       << "  " << programName << " [options]\n"
       << "\n"
       << "Run NRD daemon\n"
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
        /// \todo May be try to reload config file
        signalSet.async_wait(bind(&Nrd::terminate, this, _1, _2,
                                  ref(signalSet)));
      }
  }

private:
  shared_ptr<RibManager> m_ribManager;
  ndn::Face m_face;
};

} // namespace rib
} // namespace nfd

int
main(int argc, char** argv)
{
  using namespace nfd::rib;

  ProgramOptions options;
  bool isCommandLineValid = Nrd::parseCommandLine(argc, argv, options);
  if (!isCommandLineValid) {
    Nrd::printUsage(std::cerr, argv[0]);
    return 1;
  }
  if (options.showUsage) {
    Nrd::printUsage(std::cout, argv[0]);
    return 0;
  }

  if (options.showModules) {
    Nrd::printModules(std::cout);
    return 0;
  }

  if (options.showVersion) {
    std::cout << NFD_VERSION_BUILD_STRING << std::endl;
    return 0;
  }

  Nrd nrdInstance;

  try {
    nrdInstance.initialize(options.config);
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

  boost::asio::signal_set signalSet(nfd::getGlobalIoService());
  signalSet.add(SIGINT);
  signalSet.add(SIGTERM);
  signalSet.add(SIGHUP);
  signalSet.add(SIGUSR1);
  signalSet.add(SIGUSR2);
  signalSet.async_wait(bind(&Nrd::terminate, &nrdInstance, _1, _2,
                            ndn::ref(signalSet)));

  try {
    nfd::getGlobalIoService().run();
  }
  catch (std::exception& e) {
    NFD_LOG_FATAL(e.what());
    return 3;
  }

  return 0;
}

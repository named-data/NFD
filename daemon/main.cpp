/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

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
#include "mgmt/config-file.hpp"

namespace nfd {

NFD_LOG_INIT("Main");

struct ProgramOptions
{
  bool showUsage;
  std::string config;
};

class Nfd : noncopyable
{
public:
  explicit
  Nfd(const std::string& configFile)
    : m_internalFace(new InternalFace())
    , m_fibManager(m_forwarder.getFib(),
                   bind(&Forwarder::getFace, &m_forwarder, _1),
                   m_internalFace)
    , m_faceManager(m_forwarder.getFaceTable(), m_internalFace)
    , m_strategyChoiceManager(m_forwarder.getStrategyChoice(), m_internalFace)
    , m_statusServer(m_internalFace, m_forwarder)
  {
    ConfigFile config;
    m_internalFace->getValidator().setConfigFile(config);

    m_forwarder.addFace(m_internalFace);

    m_faceManager.setConfigFile(config);

    // parse config file
    config.parse(configFile, true);
    config.parse(configFile, false);

    // add FIB entry for NFD Management Protocol
    shared_ptr<fib::Entry> entry = m_forwarder.getFib().insert("/localhost/nfd").first;
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
       << "  [--config /path/to/nfd.conf] - path to configuration file "
       << "(default: " << DEFAULT_CONFIG_FILE << ")\n"
      ;
  }

  static bool
  parseCommandLine(int argc, char** argv, ProgramOptions& options)
  {
    options.showUsage = false;
    options.config = DEFAULT_CONFIG_FILE;

    while (true) {
      int optionIndex = 0;
      static ::option longOptions[] = {
        { "help"   , no_argument      , 0, 0 },
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
        case 1: // config
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
        std::cout << "Caught signal '" << strsignal(signalNo) << "', exiting..." << std::endl;
      }
    else
      {
        /// \todo May be try to reload config file (at least security section)
        signalSet.async_wait(bind(&Nfd::terminate, this, _1, _2,
                                  boost::ref(signalSet)));
      }
  }

private:
  Forwarder m_forwarder;
  shared_ptr<InternalFace> m_internalFace;

  FibManager            m_fibManager;
  FaceManager           m_faceManager;
  StrategyChoiceManager m_strategyChoiceManager;
  StatusServer          m_statusServer;
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

  try {
    Nfd nfdInstance(options.config);

    boost::asio::signal_set signalSet(getGlobalIoService());
    signalSet.add(SIGINT);
    signalSet.add(SIGTERM);
    signalSet.add(SIGHUP);
    signalSet.add(SIGUSR1);
    signalSet.add(SIGUSR2);
    signalSet.async_wait(bind(&Nfd::terminate, &nfdInstance, _1, _2,
                              boost::ref(signalSet)));

    getGlobalIoService().run();
  }
  catch (boost::filesystem::filesystem_error& e) {
    if (e.code() == boost::system::errc::permission_denied) {
      NFD_LOG_FATAL("Permissions denied for " << e.path1() << ". "
                    << argv[0] << " should be run as superuser");
    }
    else {
      NFD_LOG_FATAL(e.what());
    }
    return 2;
  }
  catch (std::exception& e) {
    NFD_LOG_FATAL(e.what());
    return 1;
  }

  return 0;
}

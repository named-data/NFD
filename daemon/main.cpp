/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include <getopt.h>
#include "core/logger.hpp"
#include "fw/forwarder.hpp"
#include "mgmt/internal-face.hpp"
#include "mgmt/fib-manager.hpp"
#include "mgmt/face-manager.hpp"
#include "mgmt/local-control-header-manager.hpp"
#include "mgmt/strategy-choice-manager.hpp"
#include "mgmt/status-server.hpp"
#include "mgmt/config-file.hpp"

#include <boost/filesystem.hpp>

namespace nfd {

NFD_LOG_INIT("Main");

struct ProgramOptions
{
  bool m_showUsage;
  std::string m_config;
};

static ProgramOptions g_options;
static Forwarder* g_forwarder;
static FibManager* g_fibManager;
static FaceManager* g_faceManager;
static LocalControlHeaderManager* g_localControlHeaderManager;
static StrategyChoiceManager* g_strategyChoiceManager;
static StatusServer* g_statusServer;
static shared_ptr<InternalFace> g_internalFace;


void
usage(char* programName)
{
  printf(
    "%s --help\n\tshow this help and exit\n"
    "%s "
       "[--config /path/to/nfd.conf]\n"
       "\trun forwarding daemon\n"
       "\t--config <configuration file>]: path to configuration file\n"
    "\n",
    programName, programName
  );
}

bool
parseCommandLine(int argc, char** argv)
{
  g_options.m_showUsage = false;
  g_options.m_config = DEFAULT_CONFIG_FILE;

  while (true) {
    int option_index = 0;
    static ::option long_options[] = {
      { "help"   , no_argument      , 0, 0 },
      { "config" , required_argument, 0, 0 },
      { 0        , 0                , 0, 0 }
    };
    int c = getopt_long_only(argc, argv, "", long_options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0://help
            g_options.m_showUsage = true;
            break;
          case 1://config
            g_options.m_config = ::optarg;
            break;
        }
        break;
    }
  }

  return true;
}

void
initializeMgmt()
{
  ConfigFile config;

  g_internalFace = make_shared<InternalFace>();
  g_internalFace->getValidator().setConfigFile(config);
  g_forwarder->addFace(g_internalFace);

  g_fibManager = new FibManager(g_forwarder->getFib(),
                                bind(&Forwarder::getFace, g_forwarder, _1),
                                g_internalFace);

  g_faceManager = new FaceManager(g_forwarder->getFaceTable(), g_internalFace);
  g_faceManager->setConfigFile(config);

  g_localControlHeaderManager =
    new LocalControlHeaderManager(bind(&Forwarder::getFace, g_forwarder, _1),
                                  g_internalFace);

  g_strategyChoiceManager = new StrategyChoiceManager(g_forwarder->getStrategyChoice(),
                                                      g_internalFace);

  g_statusServer = new StatusServer(g_internalFace, *g_forwarder);

  config.parse(g_options.m_config, true);
  config.parse(g_options.m_config, false);

  shared_ptr<fib::Entry> entry = g_forwarder->getFib().insert("/localhost/nfd").first;
  entry->addNextHop(g_internalFace, 0);
}

int
main(int argc, char** argv)
{
  bool isCommandLineValid = parseCommandLine(argc, argv);
  if (!isCommandLineValid) {
    usage(argv[0]);
    return 1;
  }
  if (g_options.m_showUsage) {
    usage(argv[0]);
    return 0;
  }

  try {
    g_forwarder = new Forwarder();
    initializeMgmt();

    /// \todo Add signal processing to gracefully terminate the app

    getGlobalIoService().run();
  // } catch(ConfigFile::Error& error) {
  //   NFD_LOG_ERROR("Error: " << error.what());
  //   NFD_LOG_ERROR("You should either specify --config option or copy sample configuration into "
  //                 << DEFAULT_CONFIG_FILE);
  }
  catch (boost::filesystem::filesystem_error& e) {
    if (e.code() == boost::system::errc::permission_denied) {
      NFD_LOG_ERROR("Error: Permissions denied for " << e.path1());
      NFD_LOG_ERROR(argv[0] << " should be run as superuser");
    }
    else {
      NFD_LOG_ERROR("Error: " << e.what());
    }
  }
  catch (std::exception& e) {
    NFD_LOG_ERROR("Error: " << e.what());
    return 1;
  }

  return 0;
}

} // namespace nfd

int
main(int argc, char** argv)
{
  return nfd::main(argc, argv);
}


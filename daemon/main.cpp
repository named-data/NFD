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
#include "face/tcp-factory.hpp"

#ifdef HAVE_UNIX_SOCKETS
#include "face/unix-stream-factory.hpp"
#endif

namespace nfd {

NFD_LOG_INIT("Main");

struct ProgramOptions
{
  struct TcpOutgoing
  {
    TcpOutgoing(std::pair<std::string, std::string> endpoint)
      : m_endpoint(endpoint)
    {
    }

    std::pair<std::string, std::string> m_endpoint;
    std::vector<Name> m_prefixes;
  };

  bool m_showUsage;
  std::pair<std::string, std::string> m_tcpListen;
  std::vector<TcpOutgoing> m_tcpOutgoings;
  std::string m_unixListen;
};

static ProgramOptions g_options;
static Forwarder* g_forwarder;
static FibManager* g_fibManager;
static FaceManager* g_faceManager;
static LocalControlHeaderManager* g_localControlHeaderManager;
static StrategyChoiceManager* g_strategyChoiceManager;
static TcpFactory* g_tcpFactory;
static shared_ptr<TcpChannel> g_tcpChannel;
static shared_ptr<InternalFace> g_internalFace;

#ifdef HAVE_UNIX_SOCKETS
static UnixStreamFactory* g_unixFactory;
static shared_ptr<UnixStreamChannel> g_unixChannel;
#endif


void
usage(char* programName)
{
  printf(
    "%s --help\n\tshow this help and exit\n"
    "%s [--tcp-listen \"0.0.0.0:6363\"] "
#ifdef HAVE_UNIX_SOCKETS
       "[--unix-listen \"/var/run/nfd.sock\"] "
#endif
       "[--tcp-connect \"192.0.2.1:6363\" "
            "[--prefix </example>]]\n"
      "\trun forwarding daemon\n"
      "\t--tcp-listen <ip:port>: listen on IP and port\n"
#ifdef HAVE_UNIX_SOCKETS
      "\t--unix-listen <unix-socket-path>: listen on Unix socket\n"
#endif
      "\t--tcp-connect <ip:port>: connect to IP and port (can occur multiple times)\n"
      "\t--prefix <NDN name>: add this face as nexthop to FIB entry "
        "(must appear after --tcp-connect, can occur multiple times)\n"
    "\n",
    programName, programName
  );
}

inline std::pair<std::string, std::string>
parseIpPortPair(const std::string& s)
{
  size_t pos = s.rfind(":");
  if (pos == std::string::npos) {
    throw std::invalid_argument("ip:port");
  }

  return std::make_pair(s.substr(0, pos), s.substr(pos+1));
}

bool
parseCommandLine(int argc, char** argv)
{
  g_options.m_showUsage = false;
  g_options.m_tcpListen = std::make_pair("0.0.0.0", "6363");
  g_options.m_unixListen = "/var/run/nfd.sock";
  g_options.m_tcpOutgoings.clear();

  while (1) {
    int option_index = 0;
    static ::option long_options[] = {
      { "help"          , no_argument      , 0, 0 },
      { "tcp-listen"    , required_argument, 0, 0 },
      { "tcp-connect"   , required_argument, 0, 0 },
      { "prefix"        , required_argument, 0, 0 },
      { "unix-listen"   , required_argument, 0, 0 },
      { 0               , 0                , 0, 0 }
    };
    int c = getopt_long_only(argc, argv, "", long_options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0://help
            g_options.m_showUsage = true;
            break;
          case 1://tcp-listen
            g_options.m_tcpListen = parseIpPortPair(::optarg);
            break;
          case 2://tcp-connect
            g_options.m_tcpOutgoings.push_back(parseIpPortPair(::optarg));
            break;
          case 3://prefix
            if (g_options.m_tcpOutgoings.empty()) {
              return false;
            }
            g_options.m_tcpOutgoings.back().m_prefixes.push_back(Name(::optarg));
            break;
          case 4://unix-listen
            g_options.m_unixListen = ::optarg;
            break;
        }
        break;
    }
  }
  return true;
}

void
onFaceFail(shared_ptr<Face> face, const std::string& reason)
{
  g_forwarder->removeFace(face);
}

void
onFaceEstablish(shared_ptr<Face> newFace, std::vector<Name>* prefixes)
{
  g_forwarder->addFace(newFace);
  newFace->onFail += bind(&onFaceFail, newFace, _1);

  // add nexthop on prefixes
  if (prefixes != 0) {
    Fib& fib = g_forwarder->getFib();
    for (std::vector<Name>::iterator it = prefixes->begin();
        it != prefixes->end(); ++it) {
      std::pair<shared_ptr<fib::Entry>, bool> fibInsertResult =
        fib.insert(*it);
      shared_ptr<fib::Entry> fibEntry = fibInsertResult.first;
      fibEntry->addNextHop(newFace, 0);
    }
  }
}

void
onFaceError(const std::string& reason)
{
  throw std::runtime_error(reason);
}

void
initializeTcp()
{
  g_tcpFactory = new TcpFactory();
  g_tcpChannel = g_tcpFactory->createChannel(g_options.m_tcpListen.first,
                                             g_options.m_tcpListen.second);
  g_tcpChannel->listen(
    bind(&onFaceEstablish, _1, static_cast<std::vector<Name>*>(0)),
    &onFaceError);

  for (std::vector<ProgramOptions::TcpOutgoing>::iterator it =
      g_options.m_tcpOutgoings.begin();
      it != g_options.m_tcpOutgoings.end(); ++it) {
    g_tcpChannel->connect(it->m_endpoint.first, it->m_endpoint.second,
      bind(&onFaceEstablish, _1, &(it->m_prefixes)), &onFaceError);
  }
}

#ifdef HAVE_UNIX_SOCKETS
void
initializeUnix()
{
  g_unixFactory = new UnixStreamFactory();
  g_unixChannel = g_unixFactory->createChannel(g_options.m_unixListen);

  g_unixChannel->listen(
    bind(&onFaceEstablish, _1, static_cast<std::vector<Name>*>(0)),
    &onFaceError);
}
#endif

void
initializeMgmt()
{
  g_internalFace = make_shared<InternalFace>();
  g_forwarder->addFace(g_internalFace);

  g_fibManager = new FibManager(g_forwarder->getFib(),
                                bind(&Forwarder::getFace, g_forwarder, _1),
                                g_internalFace);

  g_faceManager = new FaceManager(g_forwarder->getFaceTable(), g_internalFace);

  g_localControlHeaderManager =
    new LocalControlHeaderManager(bind(&Forwarder::getFace, g_forwarder, _1),
                                  g_internalFace);

  g_strategyChoiceManager = new StrategyChoiceManager(g_forwarder->getStrategyChoice(),
                                                      g_internalFace);

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

  g_forwarder = new Forwarder();
  initializeTcp();
#ifdef HAVE_UNIX_SOCKETS
  initializeUnix();
#endif
  initializeMgmt();

  /// \todo Add signal processing to gracefully terminate the app

  try {
    getGlobalIoService().run();
  } catch(std::exception& ex) {
    NFD_LOG_ERROR(ex.what());
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

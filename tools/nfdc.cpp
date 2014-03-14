/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */
#include "nfdc.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex_find_format.hpp>
#include <boost/regex.hpp>

void
usage(const char* programName)
{
  std::cout << "Usage:\n" << programName  << " [-h] COMMAND\n"
  "       -h print usage and exit\n"
  "\n"
  "   COMMAND can be one of following:\n"
  "       add-nexthop <name> <faceId> [<cost>]\n"
  "           Add a nexthop to a FIB entry\n"
  "       remove-nexthop <name> <faceId> \n"
  "           Remove a nexthop from a FIB entry\n"
  "       create <uri> \n"
  "           Create a face in one of the following formats:\n"
  "           UDP unicast:    udp[4|6]://<remote-IP-or-host>[:<remote-port>]\n"
  "           TCP:            tcp[4|6]://<remote-IP-or-host>[:<remote-port>] \n"
  "       destroy <faceId> \n"
  "           Destroy a face\n"
  "       set-strategy <name> <strategy> \n"
  "           Set the strategy for a namespace \n"
  "       unset-strategy <name> \n"
  "           Unset the strategy for a namespace \n"
  << std::endl;
}

namespace nfdc {
  
Controller::Controller(ndn::Face& face)
  : ndn::nfd::Controller(face)
{
}
  
Controller::~Controller()
{
}
bool
Controller::dispatch(const std::string& command, const char* commandOptions[], int nOptions)
{
  if (command == "add-nexthop") {
    if (nOptions == 2)
      fibAddNextHop(commandOptions, false);
    else if (nOptions == 3)
      fibAddNextHop(commandOptions, true);
    else
      return false;
  }
  else if (command == "remove-nexthop") {
    if (nOptions != 2)
      return false;
    fibRemoveNextHop(commandOptions);
  }
  else if (command == "create") {
    if (nOptions != 1)
      return false;
    faceCreate(commandOptions);
  }
  else if (command == "destroy") {
    if (nOptions != 1)
      return false;
    faceDestroy(commandOptions);
  }
  else if (command == "set-strategy") {
    if (nOptions != 2)
      return false;
    strategyChoiceSet(commandOptions);
  }
  else if (command == "unset-strategy") {
    if (nOptions != 1)
      return false;
    strategyChoiceUnset(commandOptions);
  }
  else
    usage(m_programName);

  return true;
}

void
Controller::fibAddNextHop(const char* commandOptions[], bool hasCost)
{
  ndn::nfd::FibManagementOptions fibOptions;
  
  const std::string& name = commandOptions[0];
  const int faceId = boost::lexical_cast<int>(commandOptions[1]);

  fibOptions.setName(name);
  fibOptions.setFaceId(faceId);

  if (hasCost)
  {
    const int cost = boost::lexical_cast<int>(commandOptions[2]);
    fibOptions.setCost(cost);
  }
  startFibCommand("add-nexthop",
                  fibOptions,
                  bind(&Controller::onFibSuccess, this, _1, "Nexthop insertion succeeded"),
                  bind(&Controller::onError, this, _1, "Nexthop insertion failed"));
}

void
Controller::fibRemoveNextHop(const char* commandOptions[])
{
  const std::string& name = commandOptions[0];
  const int faceId = boost::lexical_cast<int>(commandOptions[1]);
  ndn::nfd::FibManagementOptions fibOptions;

  fibOptions.setName(name);
  fibOptions.setFaceId(faceId);
  startFibCommand("remove-nexthop",
                  fibOptions,
                  bind(&Controller::onFibSuccess, this, _1, "Nexthop Removal succeeded"),
                  bind(&Controller::onError, this, _1, "Nexthop Removal failed"));
}

namespace {
bool
isValidUri(const std::string& input)
{
  // an extended regex to support the validation of uri structure
  // boost::regex e("^[a-z0-9]+-?+[a-z0-9]+\\:\\/\\/.*");
  boost::regex e("^[a-z0-9]+\\:.*");
  return boost::regex_match(input, e);
}
} // anonymous namespace

void
Controller::faceCreate(const char* commandOptions[])
{
  ndn::nfd::FaceManagementOptions faceOptions;
  const std::string& uri = commandOptions[0];
  faceOptions.setUri(uri);
  
  if (isValidUri(uri))
  {
    startFaceCommand("create",
                     faceOptions,
                     bind(&Controller::onFaceSuccess, this, _1, "Face creation succeeded"),
                     bind(&Controller::onError, this, _1, "Face creation failed"));
  }
  else
    throw Error("invalid uri format");
}
 
void
Controller::faceDestroy(const char* commandOptions[])
{
  ndn::nfd::FaceManagementOptions faceOptions;
  const int faceId = boost::lexical_cast<int>(commandOptions[0]);
  faceOptions.setFaceId(faceId);
                        
  startFaceCommand("destroy",
                   faceOptions,
                   bind(&Controller::onFaceSuccess, this, _1, "Face destroy succeeded"),
                   bind(&Controller::onError, this, _1, "Face destroy failed"));
}
  
void
Controller::strategyChoiceSet(const char* commandOptions[])
{
  const std::string& name = commandOptions[0];
  const std::string& strategy = commandOptions[1];
  ndn::nfd::StrategyChoiceOptions strategyChoiceOptions;
  
  strategyChoiceOptions.setName(name);
  strategyChoiceOptions.setStrategy(strategy);
  
  startStrategyChoiceCommand("set",
                             strategyChoiceOptions,
                             bind(&Controller::onSetStrategySuccess,
                                  this,
                                  _1,
                                  "Successfully set strategy choice"),
                             bind(&Controller::onError, this, _1, "Failed to set strategy choice"));
  
}
  
void
Controller::strategyChoiceUnset(const char* commandOptions[])
{
  const std::string& name = commandOptions[0];
  ndn::nfd::StrategyChoiceOptions strategyChoiceOptions;
  
  strategyChoiceOptions.setName(name);
  startStrategyChoiceCommand("unset",
                             strategyChoiceOptions,
                             bind(&Controller::onSetStrategySuccess,
                                  this,
                                  _1,
                                  "Successfully unset strategy choice"),
                             bind(&Controller::onError, this, _1, "Failed to unset strategy choice"));
}
  
void
Controller::onFibSuccess(const ndn::nfd::FibManagementOptions& resp, const std::string& message)
{
  std::cout << message << ": " << resp << std::endl;
}
  
void
Controller::onFaceSuccess(const ndn::nfd::FaceManagementOptions& resp, const std::string& message)
{
  std::cout << message << ": " << resp << std::endl;
}
void
Controller::onSetStrategySuccess(const ndn::nfd::StrategyChoiceOptions& resp,
                                 const std::string& message)
{
  std::cout << message << ": " << resp << std::endl;
}
  
void
Controller::onError(const std::string& error, const std::string& message)
{
  throw Error(message + ": " + error);
}
} // namespace nfdc

int
main(int argc, char** argv)
{
  ndn::Face face;
  nfdc::Controller p(face);

  p.m_programName = argv[0];
  int opt;
  while ((opt = getopt(argc, argv, "h")) != -1) {
    switch (opt) {
      case 'h':
        usage(p.m_programName);
        return 0;
        
      default:
        usage(p.m_programName);
        return 1;
    }
  }

  if (argc == optind) {
    usage(p.m_programName);
    return 1;
  }

  try {
    bool hasSucceeded = p.dispatch(argv[optind],
                                   const_cast<const char**>(argv + optind + 1),
                                   argc - optind - 1);
    if (hasSucceeded == false) {
      usage(p.m_programName);
      return 1;
    }
    
    face.processEvents();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }
  return 0;
}


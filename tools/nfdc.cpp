/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
 *                     The University of Memphis
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
#include "nfdc.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex_find_format.hpp>
#include <boost/regex.hpp>

void
usage(const char* programName)
{
  std::cout << "Usage:\n" << programName  << " [-h] COMMAND [<Command Options>]\n"
    "       -h print usage and exit\n"
    "\n"
    "   COMMAND can be one of the following:\n"
    "       add-nexthop [-c <cost>] <name> <faceId | faceUri>\n"
    "           Add a nexthop to a FIB entry\n"
    "           -c: specify cost\n"
    "       remove-nexthop <name> <faceId> \n"
    "           Remove a nexthop from a FIB entry\n"
    "       register [-I] [-C] [-c cost] name <faceId | faceUri>\n"
    "           register name to the given faceId or faceUri\n"
    "           -I: unset CHILD_INHERIT flag\n"
    "           -C: set CAPTURE flag\n"
    "           -c: specify cost\n"
    "       unregister name <faceId>\n"
    "           unregister name from the given faceId\n"
    "       create <faceUri> \n"
    "           Create a face in one of the following formats:\n"
    "           UDP unicast:    udp[4|6]://<remote-IP-or-host>[:<remote-port>]\n"
    "           TCP:            tcp[4|6]://<remote-IP-or-host>[:<remote-port>] \n"
    "       destroy <faceId | faceUri> \n"
    "           Destroy a face\n"
    "       set-strategy <name> <strategy> \n"
    "           Set the strategy for a namespace \n"
    "       unset-strategy <name> \n"
    "           Unset the strategy for a namespace \n"
    << std::endl;
}

namespace nfdc {

Nfdc::Nfdc(ndn::Face& face)
  : m_flags(ROUTE_FLAG_CHILD_INHERIT)
  , m_cost(0)
  , m_controller(face)
{
}

Nfdc::~Nfdc()
{
}

bool
Nfdc::dispatch(const std::string& command)
{
  if (command == "add-nexthop") {
    if (m_nOptions != 2)
      return false;
    fibAddNextHop();
  }
  else if (command == "remove-nexthop") {
    if (m_nOptions != 2)
      return false;
    fibRemoveNextHop();
  }
  else if (command == "register") {
    if (m_nOptions != 2)
      return false;
    ribRegisterPrefix();
  }
  else if (command == "unregister") {
    if (m_nOptions != 2)
      return false;
    ribUnregisterPrefix();
  }
  else if (command == "create") {
    if (m_nOptions != 1)
      return false;
    faceCreate();
  }
  else if (command == "destroy") {
    if (m_nOptions != 1)
      return false;
    faceDestroy();
  }
  else if (command == "set-strategy") {
    if (m_nOptions != 2)
      return false;
    strategyChoiceSet();
  }
  else if (command == "unset-strategy") {
    if (m_nOptions != 1)
      return false;
    strategyChoiceUnset();
  }
  else
    usage(m_programName);

  return true;
}

namespace {

inline bool
isValidUri(const std::string& input)
{
  // an extended regex to support the validation of uri structure
  // boost::regex e("^[a-z0-9]+-?+[a-z0-9]+\\:\\/\\/.*");
  boost::regex e("^[a-z0-9]+\\:.*");
  return boost::regex_match(input, e);
}

} // anonymous namespace

void
Nfdc::fibAddNextHop()
{
  m_name = m_commandLineArguments[0];
  ControlParameters parameters;
  parameters
    .setName(m_name)
    .setCost(m_cost);

  if (!isValidUri(m_commandLineArguments[1])) {
    try { //So the uri is not valid, may be a faceId is provided.
      m_faceId = boost::lexical_cast<int>(m_commandLineArguments[1]);
    }
    catch (const std::exception& e) {
      std::cerr << "No valid faceUri or faceId is provided"<< std::endl;
      return;
    }
    parameters.setFaceId(m_faceId);
    fibAddNextHop(parameters);
  }
  else {
    ControlParameters faceParameters;
    faceParameters.setUri(m_commandLineArguments[1]);

    m_controller.start<FaceCreateCommand>(faceParameters,
                                          bind(&Nfdc::fibAddNextHop, this, _1),
                                          bind(&Nfdc::onError, this, _1, _2,
                                               "Face creation failed"));
  }
}

void
Nfdc::fibAddNextHop(const ControlParameters& faceCreateResult)
{
  ControlParameters ribParameters;
  ribParameters
    .setName(m_name)
    .setCost(m_cost)
    .setFaceId(faceCreateResult.getFaceId());

  m_controller.start<FibAddNextHopCommand>(ribParameters,
                                           bind(&Nfdc::onSuccess, this, _1,
                                                "Nexthop insertion succeeded"),
                                           bind(&Nfdc::onError, this, _1, _2,
                                                "Nexthop insertion failed"));
}

void
Nfdc::fibRemoveNextHop()
{
  m_name = m_commandLineArguments[0];
  try {
    m_faceId = boost::lexical_cast<int>(m_commandLineArguments[1]);
  }
  catch (const std::exception& e) {
    std::cerr << "No valid faceUri or faceId is provided"<< std::endl;
    return;
  }

  ControlParameters parameters;
  parameters
    .setName(m_name)
    .setFaceId(m_faceId);

  m_controller.start<FibRemoveNextHopCommand>(parameters,
                                              bind(&Nfdc::onSuccess, this, _1,
                                                   "Nexthop removal succeeded"),
                                              bind(&Nfdc::onError, this, _1, _2,
                                                   "Nexthop removal failed"));
}

void
Nfdc::ribRegisterPrefix()
{
  m_name = m_commandLineArguments[0];
  ControlParameters parameters;
  parameters
    .setName(m_name)
    .setCost(m_cost)
    .setFlags(m_flags);

  if (!isValidUri(m_commandLineArguments[1])) {
    try { //So the uri is not valid, may be a faceId is provided.
      m_faceId = boost::lexical_cast<int>(m_commandLineArguments[1]);
    }
    catch (const std::exception& e) {
      std::cerr << "No valid faceUri or faceId is provided"<< std::endl;
      return;
    }
    parameters.setFaceId(m_faceId);
    ribRegisterPrefix(parameters);
  }
  else {
    ControlParameters faceParameters;
    faceParameters.setUri(m_commandLineArguments[1]);

    m_controller.start<FaceCreateCommand>(faceParameters,
                                          bind(&Nfdc::ribRegisterPrefix, this, _1),
                                          bind(&Nfdc::onError, this, _1, _2,
                                               "Face creation failed"));
  }
}

void
Nfdc::ribRegisterPrefix(const ControlParameters& faceCreateResult)
{
  ControlParameters ribParameters;
  ribParameters
    .setName(m_name)
    .setCost(m_cost)
    .setFlags(m_flags)
    .setFaceId(faceCreateResult.getFaceId());

  m_controller.start<RibRegisterCommand>(ribParameters,
                                         bind(&Nfdc::onSuccess, this, _1,
                                                "Successful in name registration"),
                                         bind(&Nfdc::onError, this, _1, _2,
                                                "Failed in name registration"));
}

void
Nfdc::ribUnregisterPrefix()
{
  m_name = m_commandLineArguments[0];
  try {
    m_faceId = boost::lexical_cast<int>(m_commandLineArguments[1]);
  }
  catch (const std::exception& e) {
    std::cerr << "No valid faceUri or faceId is provided"<< std::endl;
    return;
  }

  ControlParameters parameters;
  parameters
    .setName(m_name)
    .setFaceId(m_faceId);

  m_controller.start<RibUnregisterCommand>(parameters,
                                           bind(&Nfdc::onSuccess, this, _1,
                                                "Successful in unregistering name"),
                                           bind(&Nfdc::onError, this, _1, _2,
                                                "Failed in unregistering name"));
}

void
Nfdc::faceCreate()
{
  if (!isValidUri(m_commandLineArguments[0]))
    throw Error("invalid uri format");

  ControlParameters parameters;
  parameters.setUri(m_commandLineArguments[0]);

  m_controller.start<FaceCreateCommand>(parameters,
                                        bind(&Nfdc::onSuccess, this, _1,
                                             "Face creation succeeded"),
                                        bind(&Nfdc::onError, this, _1, _2,
                                             "Face creation failed"));
}

void
Nfdc::faceDestroy()
{
  ControlParameters parameters;
  if (!isValidUri(m_commandLineArguments[0])) {
    try { //So the uri is not valid, may be a faceId is provided.
      m_faceId = boost::lexical_cast<int>(m_commandLineArguments[0]);
    }
    catch (const std::exception& e) {
      std::cerr << "No valid faceUri or faceId is provided" << std::endl;
      return;
    }
    parameters.setFaceId(m_faceId);
    faceDestroy(parameters);
  }
  else{
    ControlParameters faceParameters;
    faceParameters.setUri(m_commandLineArguments[0]);
    m_controller.start<FaceCreateCommand>(faceParameters,
                                          bind(&Nfdc::faceDestroy, this, _1),
                                          bind(&Nfdc::onError, this, _1, _2,
                                               "Face destroy failed"));
  }
}

void
Nfdc::faceDestroy(const ControlParameters& faceCreateResult)
{
  ControlParameters faceParameters;
  faceParameters.setFaceId(faceCreateResult.getFaceId());

  m_controller.start<FaceDestroyCommand>(faceParameters,
                                         bind(&Nfdc::onSuccess, this, _1,
                                              "Face destroy succeeded"),
                                         bind(&Nfdc::onError, this, _1, _2,
                                              "Face destroy failed"));
}

void
Nfdc::strategyChoiceSet()
{
  const std::string& name = m_commandLineArguments[0];
  const std::string& strategy = m_commandLineArguments[1];

  ControlParameters parameters;
  parameters
    .setName(name)
    .setStrategy(strategy);

  m_controller.start<StrategyChoiceSetCommand>(parameters,
                                               bind(&Nfdc::onSuccess, this, _1,
                                                    "Successfully set strategy choice"),
                                               bind(&Nfdc::onError, this, _1, _2,
                                                    "Failed to set strategy choice"));
}

void
Nfdc::strategyChoiceUnset()
{
  const std::string& name = m_commandLineArguments[0];

  ControlParameters parameters;
  parameters.setName(name);

  m_controller.start<StrategyChoiceUnsetCommand>(parameters,
                                                 bind(&Nfdc::onSuccess, this, _1,
                                                      "Successfully unset strategy choice"),
                                                 bind(&Nfdc::onError, this, _1, _2,
                                                      "Failed to unset strategy choice"));
}

void
Nfdc::onSuccess(const ControlParameters& commandSuccessResult, const std::string& message)
{
  std::cout << message << ": " << commandSuccessResult << std::endl;
}

void
Nfdc::onError(uint32_t code, const std::string& error, const std::string& message)
{
  std::ostringstream os;
  os << message << ": " << error << " (code: " << code << ")";
  throw Error(os.str());
}



} // namespace nfdc

int
main(int argc, char** argv)
{
  ndn::Face face;
  nfdc::Nfdc p(face);

  p.m_programName = argv[0];

  if (argc < 2) {
    usage(p.m_programName);
    return 0;
  }

  if (!strcmp(argv[1], "-h")) {
    usage(p.m_programName);
    return 0;
  }


  int opt;
  //start reading options from 2nd argument i.e. Command
  optind=2;
  while ((opt = getopt(argc, argv, "ICc:")) != -1) {
    switch (opt) {
    case 'I':
      p.m_flags =  p.m_flags & ~(nfdc::ROUTE_FLAG_CHILD_INHERIT);
      break;

    case 'C':
      p.m_flags =  p.m_flags | nfdc::ROUTE_FLAG_CAPTURE;
      break;

    case 'c':
      try {
        p.m_cost =  boost::lexical_cast<int>(optarg);
      }
      catch (const std::exception& e) {
        std::cerr << "Error: cost must be in integer format" << std::endl;
        return 1;
      }
      break;
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
    p.m_commandLineArguments = const_cast<const char**>(argv + optind);
    p.m_nOptions = argc - optind;

    //argv[1] points to the command, so pass it to the dispatch
    bool isOk = p.dispatch(argv[1]);
    if (!isOk) {
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

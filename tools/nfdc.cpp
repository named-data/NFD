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
    "       add <name> <faceUri> [<cost>]\n"
    "           Create a face, and add a nexthop for this face to a FIB entry\n"
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

Nfdc::Nfdc(ndn::Face& face)
  : m_controller(face)
{
}

Nfdc::~Nfdc()
{
}

bool
Nfdc::dispatch(const std::string& command)
{
  if (command == "add") {
    if (m_nOptions == 2)
      addName();
    else if (m_nOptions == 3)
      addName();
    else
      return false;
  }
  else if (command == "add-nexthop") {
    if (m_nOptions == 2)
      fibAddNextHop(false);
    else if (m_nOptions == 3)
      fibAddNextHop(true);
    else
      return false;
  }
  else if (command == "remove-nexthop") {
    if (m_nOptions != 2)
      return false;
    fibRemoveNextHop();
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
Nfdc::addName()
{
  if (!isValidUri(m_commandLineArguments[1]))
    throw Error("invalid uri format");

  ControlParameters parameters;
  parameters
    .setUri(m_commandLineArguments[1]);

  m_controller.start<FaceCreateCommand>(
    parameters,
    bind(&Nfdc::onAddSuccess, this, _1, "nfdc add:Face creation succeeded"),
    bind(&Nfdc::onError, this, _1, _2, "nfdc add: Face creation failed"));

}

void
Nfdc::fibAddNextHop(bool hasCost)
{
  const std::string& name = m_commandLineArguments[0];
  const int faceId = boost::lexical_cast<int>(m_commandLineArguments[1]);

  fibAddNextHop(name, faceId, hasCost);

}

void
Nfdc::fibAddNextHop(const std::string& name, const uint64_t faceId, bool hasCost)
{
  ControlParameters parameters;
  parameters
    .setName(name)
    .setFaceId(faceId);

  if (hasCost)
  {
    const uint64_t cost = boost::lexical_cast<uint64_t>(m_commandLineArguments[2]);
    parameters.setCost(cost);
  }

  m_controller.start<FibAddNextHopCommand>(
    parameters,
    bind(&Nfdc::onSuccess, this, _1, "Nexthop insertion succeeded"),
    bind(&Nfdc::onError, this, _1, _2, "Nexthop insertion failed"));
}

void
Nfdc::fibRemoveNextHop()
{
  const std::string& name = m_commandLineArguments[0];
  const int faceId = boost::lexical_cast<int>(m_commandLineArguments[1]);

  ControlParameters parameters;
  parameters
    .setName(name)
    .setFaceId(faceId);

  m_controller.start<FibRemoveNextHopCommand>(
    parameters,
    bind(&Nfdc::onSuccess, this, _1, "Nexthop removal succeeded"),
    bind(&Nfdc::onError, this, _1, _2, "Nexthop removal failed"));
}

void
Nfdc::faceCreate()
{
  if (!isValidUri(m_commandLineArguments[0]))
    throw Error("invalid uri format");

  ControlParameters parameters;
  parameters
    .setUri(m_commandLineArguments[0]);

  m_controller.start<FaceCreateCommand>(
    parameters,
    bind(&Nfdc::onSuccess, this, _1, "Face creation succeeded"),
    bind(&Nfdc::onError, this, _1, _2, "Face creation failed"));
}

void
Nfdc::faceDestroy()
{
  const int faceId = boost::lexical_cast<int>(m_commandLineArguments[0]);

  ControlParameters parameters;
  parameters
    .setFaceId(faceId);

  m_controller.start<FaceDestroyCommand>(
    parameters,
    bind(&Nfdc::onSuccess, this, _1, "Face destroy succeeded"),
    bind(&Nfdc::onError, this, _1, _2, "Face destroy failed"));
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

  m_controller.start<StrategyChoiceSetCommand>(
    parameters,
    bind(&Nfdc::onSuccess, this, _1, "Successfully set strategy choice"),
    bind(&Nfdc::onError, this, _1, _2, "Failed to set strategy choice"));
}

void
Nfdc::strategyChoiceUnset()
{
  const std::string& name = m_commandLineArguments[0];

  ControlParameters parameters;
  parameters
    .setName(name);

  m_controller.start<StrategyChoiceUnsetCommand>(
    parameters,
    bind(&Nfdc::onSuccess, this, _1, "Successfully unset strategy choice"),
    bind(&Nfdc::onError, this, _1, _2, "Failed to unset strategy choice"));
}

void
Nfdc::onSuccess(const ControlParameters& parameters, const std::string& message)
{
  std::cout << message << ": " << parameters << std::endl;
}

void
Nfdc::onAddSuccess(const ControlParameters& parameters, const std::string& message)
{
  uint64_t faceId = parameters.getFaceId();

  if (m_nOptions == 2)
    fibAddNextHop(m_commandLineArguments[0], faceId, false);
  else if (m_nOptions == 3)
    fibAddNextHop(m_commandLineArguments[0], faceId, true);
  else
    throw Error("invalid number of arguments");
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
    p.m_commandLineArguments = const_cast<const char**>(argv + optind + 1);
    p.m_nOptions = argc - optind - 1;
    bool isOk = p.dispatch(argv[optind]);
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

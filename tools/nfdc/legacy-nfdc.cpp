/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "legacy-nfdc.hpp"
#include "face-id-fetcher.hpp"

#include <boost/program_options.hpp>
#include <boost/regex.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

using ndn::nfd::ControlParameters;

const time::milliseconds LegacyNfdc::DEFAULT_EXPIRATION_PERIOD = time::milliseconds::max();
const uint64_t LegacyNfdc::DEFAULT_COST = 0;

LegacyNfdc::LegacyNfdc(Face& face, KeyChain& keyChain)
  : m_flags(ndn::nfd::ROUTE_FLAG_CHILD_INHERIT)
  , m_cost(DEFAULT_COST)
  , m_origin(ndn::nfd::ROUTE_ORIGIN_STATIC)
  , m_expires(DEFAULT_EXPIRATION_PERIOD)
  , m_facePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
  , m_face(face)
  , m_controller(face, keyChain)
{
}

bool
LegacyNfdc::dispatch(const std::string& command)
{
  if (command == "add-nexthop") {
    if (m_commandLineArguments.size() != 2)
      return false;
    fibAddNextHop();
  }
  else if (command == "remove-nexthop") {
    if (m_commandLineArguments.size() != 2)
      return false;
    fibRemoveNextHop();
  }
  else if (command == "register") {
    if (m_commandLineArguments.size() != 2)
      return false;
    ribRegisterPrefix();
  }
  else if (command == "unregister") {
    if (m_commandLineArguments.size() != 2)
      return false;
    ribUnregisterPrefix();
  }
  else if (command == "create") {
    if (m_commandLineArguments.size() != 1)
      return false;
    faceCreate();
  }
  else if (command == "destroy") {
    if (m_commandLineArguments.size() != 1)
      return false;
    faceDestroy();
  }
  else if (command == "set-strategy") {
    if (m_commandLineArguments.size() != 2)
      return false;
    strategyChoiceSet();
  }
  else if (command == "unset-strategy") {
    if (m_commandLineArguments.size() != 1)
      return false;
    strategyChoiceUnset();
  }
  else
    return false;

  return true;
}

void
LegacyNfdc::fibAddNextHop()
{
  m_name = m_commandLineArguments[0];
  const std::string& faceName = m_commandLineArguments[1];

  FaceIdFetcher::start(m_face, m_controller, faceName, true,
    [this] (const uint32_t faceId) {
      ControlParameters parameters;
      parameters
        .setName(m_name)
        .setCost(m_cost)
        .setFaceId(faceId);

      m_controller.start<ndn::nfd::FibAddNextHopCommand>(parameters,
        bind(&LegacyNfdc::onSuccess, this, _1, "Nexthop insertion succeeded"),
        bind(&LegacyNfdc::onError, this, _1, "Nexthop insertion failed"));
    },
    bind(&LegacyNfdc::onObtainFaceIdFailure, this, _1));
}

void
LegacyNfdc::fibRemoveNextHop()
{
  m_name = m_commandLineArguments[0];
  const std::string& faceName = m_commandLineArguments[1];

  FaceIdFetcher::start(m_face, m_controller, faceName, false,
    [this] (const uint32_t faceId) {
      ControlParameters parameters;
      parameters
        .setName(m_name)
        .setFaceId(faceId);

      m_controller.start<ndn::nfd::FibRemoveNextHopCommand>(parameters,
        bind(&LegacyNfdc::onSuccess, this, _1, "Nexthop removal succeeded"),
        bind(&LegacyNfdc::onError, this, _1, "Nexthop removal failed"));
    },
    bind(&LegacyNfdc::onObtainFaceIdFailure, this, _1));
}

void
LegacyNfdc::ribRegisterPrefix()
{
  m_name = m_commandLineArguments[0];
  const std::string& faceName = m_commandLineArguments[1];

  FaceIdFetcher::start(m_face, m_controller, faceName, true,
    [this] (const uint32_t faceId) {
      ControlParameters parameters;
      parameters
        .setName(m_name)
        .setCost(m_cost)
        .setFlags(m_flags)
        .setOrigin(m_origin)
        .setFaceId(faceId);

      if (m_expires != DEFAULT_EXPIRATION_PERIOD)
        parameters.setExpirationPeriod(m_expires);

      m_controller.start<ndn::nfd::RibRegisterCommand>(parameters,
        bind(&LegacyNfdc::onSuccess, this, _1, "Successful in name registration"),
        bind(&LegacyNfdc::onError, this, _1, "Failed in name registration"));
    },
    bind(&LegacyNfdc::onObtainFaceIdFailure, this, _1));
}

void
LegacyNfdc::ribUnregisterPrefix()
{
  m_name = m_commandLineArguments[0];
  const std::string& faceName = m_commandLineArguments[1];

  FaceIdFetcher::start(m_face, m_controller, faceName, false,
    [this] (const uint32_t faceId) {
      ControlParameters parameters;
      parameters
        .setName(m_name)
        .setFaceId(faceId)
        .setOrigin(m_origin);

      m_controller.start<ndn::nfd::RibUnregisterCommand>(parameters,
        bind(&LegacyNfdc::onSuccess, this, _1, "Successful in unregistering name"),
        bind(&LegacyNfdc::onError, this, _1, "Failed in unregistering name"));
    },
    bind(&LegacyNfdc::onObtainFaceIdFailure, this, _1));
}

void
LegacyNfdc::onCanonizeFailure(const std::string& reason)
{
  BOOST_THROW_EXCEPTION(Error(reason));
}

void
LegacyNfdc::onObtainFaceIdFailure(const std::string& message)
{
  BOOST_THROW_EXCEPTION(Error(message));
}

void
LegacyNfdc::faceCreate()
{
  boost::regex e("^[a-z0-9]+\\:.*");
  if (!boost::regex_match(m_commandLineArguments[0], e))
    BOOST_THROW_EXCEPTION(Error("invalid uri format"));

  FaceUri faceUri;
  faceUri.parse(m_commandLineArguments[0]);

  faceUri.canonize(bind(&LegacyNfdc::startFaceCreate, this, _1),
                   bind(&LegacyNfdc::onCanonizeFailure, this, _1),
                   m_face.getIoService(), time::seconds(4));
}

void
LegacyNfdc::startFaceCreate(const FaceUri& canonicalUri)
{
  ControlParameters parameters;
  parameters.setUri(canonicalUri.toString());
  parameters.setFacePersistency(m_facePersistency);

  m_controller.start<ndn::nfd::FaceCreateCommand>(parameters,
    bind(&LegacyNfdc::onSuccess, this, _1, "Face creation succeeded"),
    bind(&LegacyNfdc::onError, this, _1, "Face creation failed"));
}

void
LegacyNfdc::faceDestroy()
{
  ControlParameters parameters;
  const std::string& faceName = m_commandLineArguments[0];

  FaceIdFetcher::start(m_face, m_controller, faceName, false,
    [this] (const uint32_t faceId) {
      ControlParameters faceParameters;
      faceParameters.setFaceId(faceId);

      m_controller.start<ndn::nfd::FaceDestroyCommand>(faceParameters,
        bind(&LegacyNfdc::onSuccess, this, _1, "Face destroy succeeded"),
        bind(&LegacyNfdc::onError, this, _1, "Face destroy failed"));
    },
    bind(&LegacyNfdc::onObtainFaceIdFailure, this, _1));
}

void
LegacyNfdc::strategyChoiceSet()
{
  const std::string& name = m_commandLineArguments[0];
  const std::string& strategy = m_commandLineArguments[1];

  ControlParameters parameters;
  parameters
    .setName(name)
    .setStrategy(strategy);

  m_controller.start<ndn::nfd::StrategyChoiceSetCommand>(parameters,
    bind(&LegacyNfdc::onSuccess, this, _1, "Successfully set strategy choice"),
    bind(&LegacyNfdc::onError, this, _1, "Failed to set strategy choice"));
}

void
LegacyNfdc::strategyChoiceUnset()
{
  const std::string& name = m_commandLineArguments[0];

  ControlParameters parameters;
  parameters.setName(name);

  m_controller.start<ndn::nfd::StrategyChoiceUnsetCommand>(parameters,
    bind(&LegacyNfdc::onSuccess, this, _1, "Successfully unset strategy choice"),
    bind(&LegacyNfdc::onError, this, _1, "Failed to unset strategy choice"));
}

void
LegacyNfdc::onSuccess(const ControlParameters& commandSuccessResult, const std::string& message)
{
  std::cout << message << ": " << commandSuccessResult << std::endl;
}

void
LegacyNfdc::onError(const ndn::nfd::ControlResponse& response, const std::string& message)
{
  std::ostringstream os;
  os << message << ": " << response.getText() << " (code: " << response.getCode() << ")";
  BOOST_THROW_EXCEPTION(Error(os.str()));
}

void
legacyNfdcUsage()
{
  std::cout << "Usage:\n"
    "nfdc [-h] [-V] COMMAND [<Command Options>]\n"
    "       -h print usage and exit\n"
    "       -V print version and exit\n"
    "\n"
    "   COMMAND can be one of the following:\n"
    "       register [-I] [-C] [-c cost] [-e expiration time] [-o origin] name <faceId | faceUri>\n"
    "           register name to the given faceId or faceUri\n"
    "           -I: unset CHILD_INHERIT flag\n"
    "           -C: set CAPTURE flag\n"
    "           -c: specify cost (default 0)\n"
    "           -e: specify expiration time in ms\n"
    "               (by default the entry remains in FIB for the lifetime of the associated face)\n"
    "           -o: specify origin\n"
    "               0 for Local producer applications, 128 for NLSR, 255(default) for static routes\n"
    "       unregister [-o origin] name <faceId | faceUri>\n"
    "           unregister name from the given faceId\n"
    "       create [-P] <faceUri> \n"
    "           Create a face in one of the following formats:\n"
    "           UDP unicast:    udp[4|6]://<remote-IP-or-host>[:<remote-port>]\n"
    "           TCP:            tcp[4|6]://<remote-IP-or-host>[:<remote-port>] \n"
    "           -P: create permanent (instead of persistent) face\n"
    "       destroy <faceId | faceUri> \n"
    "           Destroy a face\n"
    "       set-strategy <name> <strategy> \n"
    "           Set the strategy for a namespace \n"
    "       unset-strategy <name> \n"
    "           Unset the strategy for a namespace \n"
    "       add-nexthop [-c <cost>] <name> <faceId | faceUri>\n"
    "           Add a nexthop to a FIB entry\n"
    "           -c: specify cost (default 0)\n"
    "       remove-nexthop <name> <faceId | faceUri> \n"
    "           Remove a nexthop from a FIB entry\n"
    << std::endl;
}

int
legacyNfdcMain(ExecuteContext& ctx)
{
  LegacyNfdc p(ctx.face, ctx.keyChain);

  const std::string& subcommand = ctx.noun;
  auto args = ctx.args.get<std::vector<std::string>>("args");
  bool wantUnsetChildInherit = false;
  bool wantCapture = false;
  bool wantPermanentFace = false;
  int64_t expires = -1;

  namespace po = boost::program_options;
  po::options_description options;
  options.add_options()
    (",I", po::bool_switch(&wantUnsetChildInherit))
    (",C", po::bool_switch(&wantCapture))
    (",c", po::value<uint64_t>(&p.m_cost))
    (",e", po::value<int64_t>(&expires))
    (",o", po::value<uint64_t>(&p.m_origin))
    (",P", po::bool_switch(&wantPermanentFace));
  po::variables_map vm;
  std::vector<std::string> unparsed;
  try {
    po::parsed_options parsed = po::command_line_parser(args).options(options).allow_unregistered().run();
    unparsed = po::collect_unrecognized(parsed.options, po::include_positional);
    po::store(parsed, vm);
    po::notify(vm);
  }
  catch (const po::error& e) {
    std::cerr << e.what() << std::endl;
    legacyNfdcUsage();
    return 2;
  }

  if (wantUnsetChildInherit) {
    p.m_flags &= ~(ndn::nfd::ROUTE_FLAG_CHILD_INHERIT);
  }
  if (wantCapture) {
    p.m_flags |= ndn::nfd::ROUTE_FLAG_CAPTURE;
  }
  if (expires >= 0) {
    // accept negative values as no expiration
    p.m_expires = time::milliseconds(expires);
  }
  if (wantPermanentFace) {
    p.m_facePersistency = ndn::nfd::FACE_PERSISTENCY_PERMANENT;
  }

  if (std::any_of(unparsed.begin(), unparsed.end(),
      [] (const std::string& s) { return s.empty() || s[0] == '-'; })) {
    // unrecognized -option
    legacyNfdcUsage();
    return 2;
  }
  p.m_commandLineArguments = unparsed;

  bool isOk = p.dispatch(subcommand);
  if (!isOk) {
    legacyNfdcUsage();
    return 2;
  }
  ctx.face.processEvents();
  return 0;
}

} // namespace nfdc
} // namespace tools
} // namespace nfd

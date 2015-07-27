/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include "nfdc.hpp"
#include "version.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex_find_format.hpp>
#include <boost/regex.hpp>

#include <ndn-cxx/management/nfd-face-query-filter.hpp>
#include <ndn-cxx/management/nfd-face-status.hpp>
#include <ndn-cxx/util/segment-fetcher.hpp>

void
usage(const char* programName)
{
  std::cout << "Usage:\n" << programName  << " [-h] [-V] COMMAND [<Command Options>]\n"
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
    "       add-nexthop [-c <cost>] <name> <faceId | faceUri>\n"
    "           Add a nexthop to a FIB entry\n"
    "           -c: specify cost (default 0)\n"
    "       remove-nexthop <name> <faceId | faceUri> \n"
    "           Remove a nexthop from a FIB entry\n"
    << std::endl;
}

namespace nfdc {

using std::bind;

const ndn::time::milliseconds Nfdc::DEFAULT_EXPIRATION_PERIOD = ndn::time::milliseconds::max();
const uint64_t Nfdc::DEFAULT_COST = 0;

Nfdc::FaceIdFetcher::FaceIdFetcher(ndn::Face& face,
                                   Controller& controller,
                                   bool allowCreate,
                                   const SuccessCallback& onSucceed,
                                   const FailureCallback& onFail)
  : m_face(face)
  , m_controller(controller)
  , m_allowCreate(allowCreate)
  , m_onSucceed(onSucceed)
  , m_onFail(onFail)
{
}

void
Nfdc::FaceIdFetcher::start(ndn::Face& face,
                           Controller& controller,
                           const std::string& input,
                           bool allowCreate,
                           const SuccessCallback& onSucceed,
                           const FailureCallback& onFail)
{
  // 1. Try parse input as FaceId, if input is FaceId, succeed with parsed FaceId
  // 2. Try parse input as FaceUri, if input is not FaceUri, fail
  // 3. Canonize faceUri
  // 4. If canonization fails, fail
  // 5. Query for face
  // 6. If query succeeds and finds a face, succeed with found FaceId
  // 7. Create face
  // 8. If face creation succeeds, succeed with created FaceId
  // 9. Fail

  boost::regex e("^[a-z0-9]+\\:.*");
  if (!boost::regex_match(input, e)) {
    try
      {
        u_int32_t faceId = boost::lexical_cast<uint32_t>(input);
        onSucceed(faceId);
        return;
      }
    catch (boost::bad_lexical_cast&)
      {
        onFail("No valid faceId or faceUri is provided");
        return;
      }
  }
  else {
    ndn::util::FaceUri faceUri;
    if (!faceUri.parse(input)) {
      onFail("FaceUri parse failed");
      return;
    }

    auto fetcher = new FaceIdFetcher(std::ref(face), std::ref(controller),
                                     allowCreate, onSucceed, onFail);

    fetcher->startGetFaceId(faceUri);
  }
}

void
Nfdc::FaceIdFetcher::startGetFaceId(const ndn::util::FaceUri& faceUri)
{
  faceUri.canonize(bind(&FaceIdFetcher::onCanonizeSuccess, this, _1),
                   bind(&FaceIdFetcher::onCanonizeFailure, this, _1),
                   m_face.getIoService(), ndn::time::seconds(4));
}

void
Nfdc::FaceIdFetcher::onCanonizeSuccess(const ndn::util::FaceUri& canonicalUri)
{
  ndn::Name queryName("/localhost/nfd/faces/query");
  ndn::nfd::FaceQueryFilter queryFilter;
  queryFilter.setRemoteUri(canonicalUri.toString());
  queryName.append(queryFilter.wireEncode());

  ndn::Interest interestPacket(queryName);
  interestPacket.setMustBeFresh(true);
  interestPacket.setInterestLifetime(ndn::time::milliseconds(4000));
  auto interest = std::make_shared<ndn::Interest>(interestPacket);

  ndn::util::SegmentFetcher::fetch(m_face, *interest,
                                   ndn::util::DontVerifySegment(),
                                   bind(&FaceIdFetcher::onQuerySuccess,
                                        this, _1, canonicalUri),
                                   bind(&FaceIdFetcher::onQueryFailure,
                                        this, _1, canonicalUri));
}

void
Nfdc::FaceIdFetcher::onCanonizeFailure(const std::string& reason)
{
  fail("Canonize faceUri failed : " + reason);
}

void
Nfdc::FaceIdFetcher::onQuerySuccess(const ndn::ConstBufferPtr& data,
                                    const ndn::util::FaceUri& canonicalUri)
{
  size_t offset = 0;
  bool isOk = false;
  ndn::Block block;
  std::tie(isOk, block) = ndn::Block::fromBuffer(data, offset);

  if (!isOk) {
    if (m_allowCreate) {
      startFaceCreate(canonicalUri);
    }
    else {
      fail("Fail to find faceId");
    }
  }
  else {
    try {
      FaceStatus status(block);
      succeed(status.getFaceId());
    }
    catch (const ndn::tlv::Error& e) {
      std::string errorMessage(e.what());
      fail("ERROR: " + errorMessage);
    }
  }
}

void
Nfdc::FaceIdFetcher::onQueryFailure(uint32_t errorCode,
                                    const ndn::util::FaceUri& canonicalUri)
{
  std::stringstream ss;
  ss << "Cannot fetch data (code " << errorCode << ")";
  fail(ss.str());
}

void
Nfdc::FaceIdFetcher::onFaceCreateError(uint32_t code,
                                       const std::string& error,
                                       const std::string& message)
{
  std::stringstream ss;
  ss << message << " : " << error << " (code " << code << ")";
  fail(ss.str());
}

void
Nfdc::FaceIdFetcher::startFaceCreate(const ndn::util::FaceUri& canonicalUri)
{
  ControlParameters parameters;
  parameters.setUri(canonicalUri.toString());

  m_controller.start<FaceCreateCommand>(parameters,
                                        [this] (const ControlParameters& result) {
                                          succeed(result.getFaceId());
                                        },
                                        bind(&FaceIdFetcher::onFaceCreateError, this, _1, _2,
                                             "Face creation failed"));
}

void
Nfdc::FaceIdFetcher::succeed(uint32_t faceId)
{
  m_onSucceed(faceId);
  delete this;
}

void
Nfdc::FaceIdFetcher::fail(const std::string& reason)
{
  m_onFail(reason);
  delete this;
}

Nfdc::Nfdc(ndn::Face& face)
  : m_flags(ROUTE_FLAG_CHILD_INHERIT)
  , m_cost(DEFAULT_COST)
  , m_origin(ROUTE_ORIGIN_STATIC)
  , m_expires(DEFAULT_EXPIRATION_PERIOD)
  , m_face(face)
  , m_controller(face, m_keyChain)
  , m_ioService(face.getIoService())
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
    return false;

  return true;
}

void
Nfdc::fibAddNextHop()
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

                         m_controller
                           .start<FibAddNextHopCommand>(parameters,
                                                        bind(&Nfdc::onSuccess, this, _1,
                                                             "Nexthop insertion succeeded"),
                                                        bind(&Nfdc::onError, this, _1, _2,
                                                             "Nexthop insertion failed"));
                       },
                       bind(&Nfdc::onObtainFaceIdFailure, this, _1));
}

void
Nfdc::fibRemoveNextHop()
{
  m_name = m_commandLineArguments[0];
  const std::string& faceName = m_commandLineArguments[1];

  FaceIdFetcher::start(m_face, m_controller, faceName, false,
                       [this] (const uint32_t faceId) {
                         ControlParameters parameters;
                         parameters
                           .setName(m_name)
                           .setFaceId(faceId);

                         m_controller
                           .start<FibRemoveNextHopCommand>(parameters,
                                                           bind(&Nfdc::onSuccess, this, _1,
                                                                "Nexthop removal succeeded"),
                                                           bind(&Nfdc::onError, this, _1, _2,
                                                                "Nexthop removal failed"));
                       },
                       bind(&Nfdc::onObtainFaceIdFailure, this, _1));
}

void
Nfdc::ribRegisterPrefix()
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

                         m_controller
                           .start<RibRegisterCommand>(parameters,
                                                      bind(&Nfdc::onSuccess, this, _1,
                                                           "Successful in name registration"),
                                                      bind(&Nfdc::onError, this, _1, _2,
                                                           "Failed in name registration"));
                       },
                       bind(&Nfdc::onObtainFaceIdFailure, this, _1));
}

void
Nfdc::ribUnregisterPrefix()
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

                         m_controller
                           .start<RibUnregisterCommand>(parameters,
                                                        bind(&Nfdc::onSuccess, this, _1,
                                                             "Successful in unregistering name"),
                                                        bind(&Nfdc::onError, this, _1, _2,
                                                             "Failed in unregistering name"));
                       },
                       bind(&Nfdc::onObtainFaceIdFailure, this, _1));
}

void
Nfdc::onCanonizeFailure(const std::string& reason)
{
  BOOST_THROW_EXCEPTION(Error(reason));
}

void
Nfdc::onObtainFaceIdFailure(const std::string& message)
{
  BOOST_THROW_EXCEPTION(Error(message));
}

void
Nfdc::faceCreate()
{
  boost::regex e("^[a-z0-9]+\\:.*");
  if (!boost::regex_match(m_commandLineArguments[0], e))
    BOOST_THROW_EXCEPTION(Error("invalid uri format"));

  ndn::util::FaceUri faceUri;
  faceUri.parse(m_commandLineArguments[0]);

  faceUri.canonize(bind(&Nfdc::startFaceCreate, this, _1),
                   bind(&Nfdc::onCanonizeFailure, this, _1),
                   m_ioService, ndn::time::seconds(4));
}

void
Nfdc::startFaceCreate(const ndn::util::FaceUri& canonicalUri)
{
  ControlParameters parameters;
  parameters.setUri(canonicalUri.toString());

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
  const std::string& faceName = m_commandLineArguments[0];

  FaceIdFetcher::start(m_face, m_controller, faceName, false,
                       [this] (const uint32_t faceId) {
                         ControlParameters faceParameters;
                         faceParameters.setFaceId(faceId);

                         m_controller.start<FaceDestroyCommand>(faceParameters,
                                                                bind(&Nfdc::onSuccess, this, _1,
                                                                     "Face destroy succeeded"),
                                                                bind(&Nfdc::onError, this, _1, _2,
                                                                     "Face destroy failed"));
                       },
                       bind(&Nfdc::onObtainFaceIdFailure, this, _1));
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
  BOOST_THROW_EXCEPTION(Error(os.str()));
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

  if (!strcmp(argv[1], "-V")) {
    std::cout << NFD_VERSION_BUILD_STRING << std::endl;
    return 0;
  }

  ::optind = 2; //start reading options from 2nd argument i.e. Command
  int opt;
  while ((opt = ::getopt(argc, argv, "ICc:e:o:")) != -1) {
    switch (opt) {
    case 'I':
      p.m_flags =  p.m_flags & ~(nfdc::ROUTE_FLAG_CHILD_INHERIT);
      break;

    case 'C':
      p.m_flags =  p.m_flags | nfdc::ROUTE_FLAG_CAPTURE;
      break;

    case 'c':
      try {
        p.m_cost = boost::lexical_cast<uint64_t>(::optarg);
      }
      catch (boost::bad_lexical_cast&) {
        std::cerr << "Error: cost must be in unsigned integer format" << std::endl;
        return 1;
      }
      break;

    case 'e':
      uint64_t expires;
      try {
        expires = boost::lexical_cast<uint64_t>(::optarg);
      }
      catch (boost::bad_lexical_cast&) {
        std::cerr << "Error: expiration time must be in unsigned integer format" << std::endl;
        return 1;
      }
      p.m_expires = ndn::time::milliseconds(expires);
      break;

    case 'o':
      try {
        p.m_origin = boost::lexical_cast<uint64_t>(::optarg);
      }
      catch (boost::bad_lexical_cast&) {
        std::cerr << "Error: origin must be in unsigned integer format" << std::endl;
        return 1;
      }
      break;

    default:
      usage(p.m_programName);
      return 1;
    }
  }

  if (argc == ::optind) {
    usage(p.m_programName);
    return 1;
  }

  try {
    p.m_commandLineArguments = argv + ::optind;
    p.m_nOptions = argc - ::optind;

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

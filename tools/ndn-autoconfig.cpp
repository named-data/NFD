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

#include "version.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/management/nfd-controller.hpp>
#include <ndn-cxx/management/nfd-face-status.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <ndn-cxx/util/face-uri.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#ifdef __APPLE__
#include <arpa/nameser_compat.h>
#endif

namespace ndn {
namespace tools {

static const Name LOCALHOP_HUB_DISCOVERY_PREFIX = "/localhop/ndn-autoconf/hub";

void
usage(const char* programName)
{
  std::cout << "Usage:\n" << programName  << " [-h] [-V]\n"
            << "   -h  - print usage and exit\n"
            << "   -V  - print version number and exit\n"
            << std::endl;
}

class NdnAutoconfig : boost::noncopyable
{
public:
  union QueryAnswer
  {
    HEADER header;
    uint8_t buf[NS_PACKETSZ];
  };

  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  explicit
  NdnAutoconfig()
    : m_controller(m_face, m_keyChain)
  {
  }

  void
  run()
  {
    m_face.processEvents();
  }

  void
  fetchSegments(const Data& data, const shared_ptr<OBufferStream>& buffer,
                void (NdnAutoconfig::*onDone)(const shared_ptr<OBufferStream>&))
  {
    buffer->write(reinterpret_cast<const char*>(data.getContent().value()),
                  data.getContent().value_size());

    uint64_t currentSegment = data.getName().get(-1).toSegment();

    const name::Component& finalBlockId = data.getMetaInfo().getFinalBlockId();
    if (finalBlockId.empty() ||
        finalBlockId.toSegment() > currentSegment)
      {
        m_face.expressInterest(data.getName().getPrefix(-1).appendSegment(currentSegment+1),
                               ndn::bind(&NdnAutoconfig::fetchSegments, this, _2, buffer, onDone),
                               ndn::bind(&NdnAutoconfig::discoverHubStage2, this, "Timeout"));
      }
    else
      {
        return (this->*onDone)(buffer);
      }
  }

  void
  discoverHubStage1()
  {
    shared_ptr<OBufferStream> buffer = make_shared<OBufferStream>();

    Interest interest("/localhost/nfd/faces/list");
    interest.setChildSelector(1);
    interest.setMustBeFresh(true);

    m_face.expressInterest(interest,
                           ndn::bind(&NdnAutoconfig::fetchSegments, this, _2, buffer,
                                     &NdnAutoconfig::discoverHubStage1_registerHubDiscoveryPrefix),
                           ndn::bind(&NdnAutoconfig::discoverHubStage2, this, "Timeout"));
  }

  void
  discoverHubStage1_registerHubDiscoveryPrefix(const shared_ptr<OBufferStream>& buffer)
  {
    ConstBufferPtr buf = buffer->buf();
    std::vector<uint64_t> multicastFaces;

    size_t offset = 0;
    while (offset < buf->size())
      {
        Block block;
        bool ok = Block::fromBuffer(buf, offset, block);
        if (!ok)
          {
            std::cerr << "ERROR: cannot decode FaceStatus TLV" << std::endl;
            break;
          }

        offset += block.size();

        nfd::FaceStatus faceStatus(block);

        ndn::util::FaceUri uri(faceStatus.getRemoteUri());
        if (uri.getScheme() == "udp4") {
          namespace ip = boost::asio::ip;
          boost::system::error_code ec;
          ip::address address = ip::address::from_string(uri.getHost(), ec);

          if (!ec && address.is_multicast()) {
            multicastFaces.push_back(faceStatus.getFaceId());
          }
          else
            continue;
        }
      }

    if (multicastFaces.empty()) {
      discoverHubStage2("No multicast faces available, skipping stage 1");
    }
    else {
      shared_ptr<nfd::Controller> controller = make_shared<nfd::Controller>(ref(m_face));
      shared_ptr<std::pair<size_t, size_t> > nRegistrations =
        make_shared<std::pair<size_t, size_t> >(0, 0);

      nfd::ControlParameters parameters;
      parameters
        .setName(LOCALHOP_HUB_DISCOVERY_PREFIX)
        .setCost(1)
        .setExpirationPeriod(time::seconds(30));

      nRegistrations->first = multicastFaces.size();

      for (std::vector<uint64_t>::iterator i = multicastFaces.begin();
           i != multicastFaces.end(); ++i) {
        parameters.setFaceId(*i);

        controller->start<nfd::RibRegisterCommand>(parameters,
          bind(&NdnAutoconfig::discoverHubStage1_onRegisterSuccess,
               this, controller, nRegistrations),
          bind(&NdnAutoconfig::discoverHubStage1_onRegisterFailure,
               this, _1, _2, controller, nRegistrations));
      }
    }
  }

  void
  discoverHubStage1_onRegisterSuccess(const shared_ptr<nfd::Controller>& controller,
                                      const shared_ptr<std::pair<size_t, size_t> >& nRegistrations)
  {
    nRegistrations->second++;

    if (nRegistrations->first == nRegistrations->second) {
      discoverHubStage1_setStrategy(controller);
    }
  }

  void
  discoverHubStage1_onRegisterFailure(uint32_t code, const std::string& error,
                                      const shared_ptr<nfd::Controller>& controller,
                                      const shared_ptr<std::pair<size_t, size_t> >& nRegistrations)
  {
    std::cerr << "ERROR: " << error << " (code: " << code << ")" << std::endl;
    nRegistrations->first--;

    if (nRegistrations->first == nRegistrations->second) {
      if (nRegistrations->first > 0) {
        discoverHubStage1_setStrategy(controller);
      } else {
        discoverHubStage2("Failed to register " + LOCALHOP_HUB_DISCOVERY_PREFIX.toUri() +
                          " for all multicast faces");
      }
    }
  }

  void
  discoverHubStage1_setStrategy(const shared_ptr<nfd::Controller>& controller)
  {
    nfd::ControlParameters parameters;
    parameters
      .setName(LOCALHOP_HUB_DISCOVERY_PREFIX)
      .setStrategy("/localhost/nfd/strategy/broadcast");

    controller->start<nfd::StrategyChoiceSetCommand>(parameters,
      bind(&NdnAutoconfig::discoverHubStage1_onSetStrategySuccess,
           this, controller),
      bind(&NdnAutoconfig::discoverHubStage1_onSetStrategyFailure,
           this, _2, controller));
  }

  void
  discoverHubStage1_onSetStrategySuccess(const shared_ptr<nfd::Controller>& controller)
  {
    discoverHubStage1_requestHubData();
  }

  void
  discoverHubStage1_onSetStrategyFailure(const std::string& error,
                                         const shared_ptr<nfd::Controller>& controller)
  {
    discoverHubStage2("Failed to set broadcast strategy for " +
                      LOCALHOP_HUB_DISCOVERY_PREFIX.toUri() + " namespace (" + error + ")");
  }

  // Start to look for a hub (NDN hub discovery first stage)
  void
  discoverHubStage1_requestHubData()
  {
    Interest interest(LOCALHOP_HUB_DISCOVERY_PREFIX);
    interest.setInterestLifetime(time::milliseconds(4000)); // 4 seconds
    interest.setMustBeFresh(true);

    std::cerr << "Stage 1: Trying multicast discovery..." << std::endl;
    m_face.expressInterest(interest,
                           bind(&NdnAutoconfig::onDiscoverHubStage1Success, this, _1, _2),
                           bind(&NdnAutoconfig::discoverHubStage2, this, "Timeout"));
  }

  // First stage OnData Callback
  void
  onDiscoverHubStage1Success(const Interest& interest, Data& data)
  {
    const Block& content = data.getContent();
    content.parse();

    // Get Uri
    Block::element_const_iterator blockValue = content.find(tlv::nfd::Uri);
    if (blockValue == content.elements_end())
    {
      discoverHubStage2("Incorrect reply to stage1");
      return;
    }
    std::string faceMgmtUri(reinterpret_cast<const char*>(blockValue->value()),
                            blockValue->value_size());
    connectToHub(faceMgmtUri);
  }

  // First stage OnTimeout callback - start 2nd stage
  void
  discoverHubStage2(const std::string& message)
  {
    std::cerr << message << std::endl;
    std::cerr << "Stage 2: Trying DNS query with default suffix..." << std::endl;

    _res.retry = 2;
    _res.ndots = 10;

    QueryAnswer queryAnswer;

    int answerSize = res_search("_ndn._udp",
                                ns_c_in,
                                ns_t_srv,
                                queryAnswer.buf,
                                sizeof(queryAnswer));

    // 2nd stage failed - move on to the third stage
    if (answerSize < 0)
    {
      discoverHubStage3("Failed to find NDN router using default suffix DNS query");
    }
    else
    {
      bool isParsed = parseHostAndConnectToHub(queryAnswer, answerSize);
      if (isParsed == false)
      {
        // Failed to parse DNS response, try stage 3
        discoverHubStage3("Failed to parse DNS response");
      }
    }
  }

  // Second stage OnTimeout callback
  void
  discoverHubStage3(const std::string& message)
  {
    std::cerr << message << std::endl;
    std::cerr << "Stage 3: Trying to find home router..." << std::endl;

    KeyChain keyChain;
    Name identity = keyChain.getDefaultIdentity();
    std::string serverName = "_ndn._udp.";

    for (Name::const_reverse_iterator i = identity.rbegin(); i != identity.rend(); i++)
    {
      serverName.append(i->toUri());
      serverName.append(".");
    }
    serverName += "_homehub._autoconf.named-data.net";
    std::cerr << "Stage3: About to query for a home router: " << serverName << std::endl;

    QueryAnswer queryAnswer;

    int answerSize = res_query(serverName.c_str(),
                               ns_c_in,
                               ns_t_srv,
                               queryAnswer.buf,
                               sizeof(queryAnswer));


    // 3rd stage failed - abort
    if (answerSize < 0)
    {
      std::cerr << "Failed to find a home router" << std::endl;
      std::cerr << "exit" << std::endl;
    }
    else
    {
      bool isParsed = parseHostAndConnectToHub(queryAnswer, answerSize);
      if (isParsed == false)
      {
        // Failed to parse DNS response
        throw Error("Failed to parse DNS response");
      }
    }

  }

  void
  connectToHub(const std::string& uri)
  {
    std::cerr << "about to connect to: " << uri << std::endl;

    m_controller.start<nfd::FaceCreateCommand>(
      nfd::ControlParameters()
        .setUri(uri),
      bind(&NdnAutoconfig::onHubConnectSuccess, this, _1),
      bind(&NdnAutoconfig::onHubConnectError, this, _1, _2)
    );
  }

  void
  onHubConnectSuccess(const nfd::ControlParameters& resp)
  {
    std::cerr << "Successfully created face: " << resp << std::endl;

    // Register a prefix in RIB
    static const Name TESTBED_PREFIX("/ndn");
    m_controller.start<nfd::RibRegisterCommand>(
      nfd::ControlParameters()
        .setName(TESTBED_PREFIX)
        .setFaceId(resp.getFaceId())
        .setOrigin(nfd::ROUTE_ORIGIN_AUTOCONF)
        .setCost(100)
        .setExpirationPeriod(time::milliseconds::max()),
      bind(&NdnAutoconfig::onPrefixRegistrationSuccess, this, _1),
      bind(&NdnAutoconfig::onPrefixRegistrationError, this, _1, _2));
  }

  void
  onHubConnectError(uint32_t code, const std::string& error)
  {
    std::ostringstream os;
    os << "Failed to create face: " << error << " (code: " << code << ")";
    throw Error(os.str());
  }


  bool parseHostAndConnectToHub(QueryAnswer& queryAnswer, int answerSize)
  {
    // The references of the next classes are:
    // http://www.diablotin.com/librairie/networking/dnsbind/ch14_02.htm
    // https://gist.github.com/mologie/6027597

    struct rechdr
    {
      uint16_t type;
      uint16_t iclass;
      uint32_t ttl;
      uint16_t length;
    };

    struct srv_t
    {
      uint16_t priority;
      uint16_t weight;
      uint16_t port;
      uint8_t* target;
    };

    if (ntohs(queryAnswer.header.ancount) == 0)
    {
      std::cerr << "No records found\n" << std::endl;
      return false;
    }

    uint8_t* blob = queryAnswer.buf + NS_HFIXEDSZ;

    blob += dn_skipname(blob, queryAnswer.buf + answerSize) + NS_QFIXEDSZ;

    for (int i = 0; i < ntohs(queryAnswer.header.ancount); i++)
    {
      char srvName[NS_MAXDNAME];
      int serverNameSize = dn_expand(queryAnswer.buf,               // message pointer
                                     queryAnswer.buf + answerSize,  // end of message
                                     blob,                          // compressed server name
                                     srvName,                       // expanded server name
                                     NS_MAXDNAME);
      if (serverNameSize < 0)
      {
        return false;
      }

      srv_t* server = reinterpret_cast<srv_t*>(&blob[sizeof(rechdr)]);
      uint16_t convertedPort = be16toh(server->port);

      blob += serverNameSize + NS_HFIXEDSZ + NS_QFIXEDSZ;

      char hostName[NS_MAXDNAME];
      int hostNameSize = dn_expand(queryAnswer.buf,               // message pointer
                                   queryAnswer.buf + answerSize,  // end of message
                                   blob,                          // compressed host name
                                   hostName,                      // expanded host name
                                   NS_MAXDNAME);
      if (hostNameSize < 0)
      {
        return false;
      }

      std::string uri = "udp://";
      uri.append(hostName);
      uri.append(":");
      uri.append(boost::lexical_cast<std::string>(convertedPort));

      connectToHub(uri);
      return true;
    }

    return false;
  }

  void
  onPrefixRegistrationSuccess(const nfd::ControlParameters& commandSuccessResult)
  {
    std::cerr << "Successful in name registration: " << commandSuccessResult << std::endl;
  }

  void
  onPrefixRegistrationError(uint32_t code, const std::string& error)
  {
    std::ostringstream os;
    os << "Failed in name registration, " << error << " (code: " << code << ")";
    throw Error(os.str());
  }

private:
  Face m_face;
  KeyChain m_keyChain;
  nfd::Controller m_controller;
};

} // namespace tools
} // namespace ndn

int
main(int argc, char** argv)
{
  int opt;
  const char* programName = argv[0];

  while ((opt = getopt(argc, argv, "hV")) != -1) {
    switch (opt) {
    case 'h':
      ndn::tools::usage(programName);
      return 0;
    case 'V':
      std::cout << NFD_VERSION_BUILD_STRING << std::endl;
      return 0;
    }
  }

  try {
    ndn::tools::NdnAutoconfig autoConfigInstance;

    autoConfigInstance.discoverHubStage1();
    autoConfigInstance.run();
  }
  catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << std::endl;
    return 1;
  }
  return 0;
}

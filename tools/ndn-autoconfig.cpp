/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */
#include <ndn-cpp-dev/face.hpp>
#include <ndn-cpp-dev/management/nfd-controller.hpp>
#include <ndn-cpp-dev/management/nfd-face-management-options.hpp>
#include <ndn-cpp-dev/security/key-chain.hpp>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#ifdef __APPLE__
#include <arpa/nameser_compat.h>
#endif

class NdnAutoconfig : public ndn::nfd::Controller
{
public:
  union QueryAnswer
  {
    HEADER header;
    uint8_t buf[NS_PACKETSZ];
  };
  
  struct Error : public std::runtime_error
  {
    Error(const std::string& what) : std::runtime_error(what)
    {
    }
  };
  
  explicit
  NdnAutoconfig(ndn::Face& face)
    : ndn::nfd::Controller(face)
  {
  }
      
  // Start to look for a hub (NDN hub discovery first stage)
  void
  discoverHubStage1()
  {
    ndn::Interest interest(ndn::Name("/localhop/ndn-autoconf/hub"));
    interest.setInterestLifetime(ndn::time::milliseconds(4000)); // 4 seconds
    interest.setMustBeFresh(true);
    
    std::cerr << "Stage 1: Trying muticast discovery..." << std::endl;
    m_face.expressInterest(interest,
                           ndn::bind(&NdnAutoconfig::onDiscoverHubStage1Success, this, _1, _2),
                           ndn::bind(&NdnAutoconfig::discoverHubStage2, this, _1, "Timeout"));
    
    m_face.processEvents();
  }
  
  // First stage OnData Callback
  void
  onDiscoverHubStage1Success(const ndn::Interest& interest, ndn::Data& data)
  {
    const ndn::Block& content = data.getContent();
    content.parse();
    
    // Get Uri
    ndn::Block::element_const_iterator bockValue = content.find(ndn::tlv::nfd::Uri);
    if (bockValue == content.elements_end())
    {
      discoverHubStage2(interest, "Incorrect reply to stage1");
      return;
    }
    std::string faceMgmtUri(reinterpret_cast<const char*>(bockValue->value()), bockValue->value_size());
    connectToHub(faceMgmtUri);
  }
  
  // First stage OnTimeout callback - start 2nd stage
  void
  discoverHubStage2(const ndn::Interest& interest, const std::string& message)
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
  
    ndn::KeyChain keyChain;
    ndn::Name identity = keyChain.getDefaultIdentity();
    std::string serverName = "_ndn._udp.";
    
    for (ndn::Name::const_reverse_iterator i = identity.rbegin(); i != identity.rend(); i++)
    {
      serverName.append(i->toEscapedString());
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
    ndn::nfd::FaceManagementOptions faceOptions;
    
    faceOptions.setUri(uri);
    startFaceCommand("create",
                     faceOptions,
                     bind(&NdnAutoconfig::onHubConnectSuccess, this, _1, "Succesfully created face: "),
                     bind(&NdnAutoconfig::onHubConnectError, this, _1, "Failed to create face: "));
  }
  
  void
  onHubConnectSuccess(const ndn::nfd::FaceManagementOptions& resp, const std::string& message)
  {
    std::cerr << message << resp << std::endl;
  }
  
  void
  onHubConnectError(const std::string& error, const std::string& message)
  {
    throw Error(message + ": " + error);
  }

  
  bool parseHostAndConnectToHub(QueryAnswer& queryAnswer, int answerSize)
  {
    // The references of the next classes are:
    // http://www.diablotin.com/librairie/networking/dnsbind/ch14_02.htm
    // https://gist.github.com/mologie/6027597
    
    class rechdr
    {
    public:
      uint16_t type;
      uint16_t iclass;
      uint32_t ttl;
      uint16_t length;
    };
    
    class srv_t
    {
    public:
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
      char hostName[NS_MAXDNAME];
      int domainNameSize = dn_expand(queryAnswer.buf,
                                     queryAnswer.buf + answerSize,
                                     blob,
                                     hostName,
                                     NS_MAXDNAME);
      if (domainNameSize < 0)
      {
        return false;
      }
      
      blob += domainNameSize;
      
      rechdr* header = reinterpret_cast<rechdr*>(&blob[0]);
      srv_t* server = reinterpret_cast<srv_t*>(&blob[sizeof(rechdr)]);
      
      domainNameSize = dn_expand(queryAnswer.buf,
                                 queryAnswer.buf + answerSize,
                                 blob + 16,
                                 hostName,
                                 NS_MAXDNAME);
      if (domainNameSize < 0)
      {
        return false;
      }
      uint16_t convertedPort = be16toh(server->port);
      std::string uri = "udp://";
      uri.append(hostName);
      uri.append(":");
      uri.append(boost::lexical_cast<std::string>(convertedPort));
      
      connectToHub(uri);
      return true;
    }

    return false;
  }
};


int
main()
{
  ndn::Face face;
  NdnAutoconfig autoConfig(face);
  try
  {
    autoConfig.discoverHubStage1();
  }
  catch (const std::exception& error)
  {
    std::cerr << "ERROR: " << error.what() << std::endl;
  }
  return 0;
}

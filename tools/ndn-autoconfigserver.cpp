/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include <ndn-cpp-dev/face.hpp>
#include <ndn-cpp-dev/security/key-chain.hpp>


void
usage(const char* programName)
{
  std::cout << "Usage:\n" << programName  << " [-h] Uri \n"
  "   -h print usage and exit\n"
  "\n"
  "   Uri - a FaceMgmt URI\n"
  << std::endl;
}

using namespace ndn;

class NdnAutoconfigServer
{
public:
  explicit
  NdnAutoconfigServer(const std::string& uri)
    : m_faceMgmtUri(uri)
  {
  }
  
  void
  onInterest(const Name& name, const Interest& interest)
  {
    size_t total_len = 0;
    ndn::Data data(ndn::Name(interest.getName()).appendVersion());
    data.setFreshnessPeriod(1000); // 1 sec
    
    // create and encode uri block
    Block uriBlock = dataBlock(tlv::nfd::Uri,
                               reinterpret_cast<const uint8_t*>(m_faceMgmtUri.c_str()),
                               m_faceMgmtUri.size());
    data.setContent(uriBlock);
    m_keyChain.sign(data);
    m_face.put(data);
  }

  void
  onRegisterFailed(const ndn::Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix in local hub's daemon (" <<
              reason << ")" << std::endl;
    m_face.shutdown();
  }
  
  void
  listen()
  {
    m_face.setInterestFilter("/localhop/ndn-autoconf/hub",
                             ndn::bind(&NdnAutoconfigServer::onInterest, this, _1, _2),
                             ndn::bind(&NdnAutoconfigServer::onRegisterFailed, this, _1, _2));
    m_face.processEvents();
  }

private:
  ndn::Face m_face;
  KeyChain m_keyChain;
  std::string m_faceMgmtUri;

};

int
main(int argc, char** argv)
{
  int opt;
  const char* programName = argv[0];
  
  while ((opt = getopt(argc, argv, "h")) != -1)
  {
    switch (opt)
    {
      case 'h':
        usage(programName);
        return 0;
        
      default:
        usage(programName);
        return 1;
    }
  }

  if (argc != optind + 1)
  {
    usage(programName);
    return 1;
  }
  // get the configured face managment uri
  NdnAutoconfigServer producer(argv[optind]);

  try
  {
    producer.listen();
  }
  catch (std::exception& error)
  {
    std::cerr << "ERROR: " << error.what() << std::endl;
  }
  return 0;
}

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2014 University of Arizona.
 *
 * Author: Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 */

#include <boost/utility.hpp>
 
#include <ndn-cpp-dev/face.hpp>
#include <ndn-cpp-dev/security/key-chain.hpp>

namespace ndntlvpoke {

class NdnTlvPoke : boost::noncopyable
{

public:

  explicit
  NdnTlvPoke(char* programName)
    : m_programName(programName)
    , m_isForceDataSet(false)
    , m_isUseDigestSha256Set(false)
    , m_isLastAsFinalBlockIdSet(false)
    , m_freshnessPeriod(-1)
    , m_timeout(-1)
    , m_isDataSent(false)
  {
  }

  void
  usage()
  {
    std::cout << "\n Usage:\n " << m_programName << " "
      "[-f] [-D] [-i identity] [-F] [-x freshness] [-w timeout] ndn:/name\n"
      "   Reads payload from stdin and sends it to local NDN forwarder as a "
      "single Data packet\n"
      "   [-f]          - force, send Data without waiting for Interest\n"
      "   [-D]          - use DigestSha256 signing method instead of "
      "SignatureSha256WithRsa\n"
      "   [-i identity] - set identity to be used for signing\n"
      "   [-F]          - set FinalBlockId to the last component of Name\n"
      "   [-x]          - set FreshnessPeriod in milliseconds\n"
      "   [-w timeout]  - set Timeout in milliseconds\n"
      "   [-h]          - print help and exit\n\n";
    exit(1);
  }

  void
  setForceData()
  {
    m_isForceDataSet = true;
  }

  void
  setUseDigestSha256()
  {
    m_isUseDigestSha256Set = true;
  }

  void
  setIdentityName(char* identityName)
  {
    m_identityName = ndn::make_shared<ndn::Name>(identityName);
  }

  void
  setLastAsFinalBlockId()
  {
    m_isLastAsFinalBlockIdSet = true;
  }

  void
  setFreshnessPeriod(int freshnessPeriod)
  {
    if (freshnessPeriod < 0)
      usage();
    m_freshnessPeriod = freshnessPeriod;
  }

  void
  setTimeout(int timeout)
  {
    if (timeout < 0)
      usage();
    m_timeout = timeout;
  }

  void
  setPrefixName(char* prefixName)
  {
    m_prefixName = ndn::Name(prefixName);
  }

  ndn::Milliseconds
  getDefaultTimeout()
  {
    return 10000;
  }

  ndn::Data
  createDataPacket()
  {
    ndn::Data dataPacket(m_prefixName);
    std::stringstream payloadStream;
    payloadStream << std::cin.rdbuf();
    std::string payload = payloadStream.str();
    dataPacket.setContent(reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length());
    if (m_freshnessPeriod >= 0)
      dataPacket.setFreshnessPeriod(m_freshnessPeriod);
    if (m_isLastAsFinalBlockIdSet)
      {
        if (!m_prefixName.empty())
          dataPacket.setFinalBlockId(m_prefixName.get(-1));
        else
          {
            std::cerr << "Name Provided Has 0 Components" << std::endl;
            exit(1);
          }
      }
    if (m_isUseDigestSha256Set)
      m_keyChain.signWithSha256(dataPacket);
    else
      {
        if (!static_cast<bool>(m_identityName))
          m_keyChain.sign(dataPacket);
        else
          m_keyChain.signByIdentity(dataPacket, *m_identityName);
      }
    return dataPacket;
  }

  void
  onInterest(const ndn::Name& name,
             const ndn::Interest& interest,
             const ndn::Data& dataPacket)
  {
    m_face.put(dataPacket);
    m_isDataSent = true;
    m_face.shutdown();
  }

  void
  onRegisterFailed(const ndn::Name& prefix, const std::string& reason)
  {
    std::cerr << "Prefix Registration Failure." << std::endl;
    std::cerr << "Reason = " << reason << std::endl;
  }

  void
  run()
  {
    try
      {
        ndn::Data dataPacket = createDataPacket();
        if (m_isForceDataSet)
          {
            m_face.put(dataPacket);
            m_isDataSent = true;
          }
        else
          {
            m_face.setInterestFilter(m_prefixName,
                                     ndn::func_lib::bind(&NdnTlvPoke::onInterest,
                                                         this, _1, _2, dataPacket),
                                     ndn::func_lib::bind(&NdnTlvPoke::onRegisterFailed,
                                                         this, _1, _2));
          }
        if (m_timeout < 0)
          m_face.processEvents(getDefaultTimeout());
        else
          m_face.processEvents(m_timeout);
      }
    catch (std::exception& e)
      {
        std::cerr << "ERROR: " << e.what() << "\n" << std::endl;
        exit(1);
      }
  }

  bool
  isDataSent() const
  {
    return m_isDataSent;
  }

private:

  ndn::KeyChain m_keyChain;
  std::string m_programName;
  bool m_isForceDataSet;
  bool m_isUseDigestSha256Set;
  ndn::shared_ptr<ndn::Name> m_identityName;
  bool m_isLastAsFinalBlockIdSet;
  ndn::Milliseconds m_freshnessPeriod;
  ndn::Milliseconds m_timeout;
  ndn::Name m_prefixName;
  bool m_isDataSent;
  ndn::Face m_face;

};

}

int
main(int argc, char* argv[])
{
  int option;
  ndntlvpoke::NdnTlvPoke ndnTlvPoke(argv[0]);
  while ((option = getopt(argc, argv, "hfDi:Fx:w:")) != -1)
    {
      switch (option) {
        case 'h':
          ndnTlvPoke.usage();
          break;
        case 'f':
          ndnTlvPoke.setForceData();
          break;
        case 'D':
          ndnTlvPoke.setUseDigestSha256();
          break;
        case 'i':
          ndnTlvPoke.setIdentityName(optarg);
          break;
        case 'F':
          ndnTlvPoke.setLastAsFinalBlockId();
          break;
        case 'x':
          ndnTlvPoke.setFreshnessPeriod(atoi(optarg));
          break;
        case 'w':
          ndnTlvPoke.setTimeout(atoi(optarg));
          break;
        default:
          ndnTlvPoke.usage();
          break;
      }
    }

  argc -= optind;
  argv += optind;

  if (argv[0] == 0)
    ndnTlvPoke.usage();

  ndnTlvPoke.setPrefixName(argv[0]);
  ndnTlvPoke.run();

  if (ndnTlvPoke.isDataSent())
    return 0;
  else
    return 1;
}

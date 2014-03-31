/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2014 University of Arizona.
 *
 * Author: Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 */

#include <boost/asio.hpp>

#include <ndn-cpp-dev/face.hpp>

namespace ndntlvpeek {

class NdnTlvPeek
{
public:
  NdnTlvPeek(char* programName)
    : m_programName(programName)
    , m_mustBeFresh(false)
    , m_isChildSelectorRightmost(false)
    , m_minSuffixComponents(-1)
    , m_maxSuffixComponents(-1)
    , m_interestLifetime(-1)
    , m_isPayloadOnlySet(false)
    , m_timeout(-1)
    , m_prefixName("")
    , m_isDataReceived(false)
    , m_ioService(new boost::asio::io_service)
    , m_face(m_ioService)
  {
  }

  void
  usage()
  {
    std::cout << "\n Usage:\n " << m_programName << " "
      "[-f] [-r] [-m min] [-M max] [-l lifetime] [-p] [-w timeout] ndn:/name\n"
      "   Get one data item matching the name prefix and write it to stdout\n"
      "   [-f]          - set MustBeFresh\n"
      "   [-r]          - set ChildSelector to select rightmost child\n"
      "   [-m min]      - set MinSuffixComponents\n"
      "   [-M max]      - set MaxSuffixComponents\n"
      "   [-l lifetime] - set InterestLifetime in time::milliseconds\n"
      "   [-p]          - print payload only, not full packet\n"
      "   [-w timeout]  - set Timeout in time::milliseconds\n"
      "   [-h]          - print help and exit\n\n";
    exit(1);
  }

  void
  setMustBeFresh()
  {
    m_mustBeFresh = true;
  }

  void
  setRightmostChildSelector()
  {
    m_isChildSelectorRightmost = true;
  }

  void
  setMinSuffixComponents(int minSuffixComponents)
  {
    if (minSuffixComponents < 0)
      usage();
    m_minSuffixComponents = minSuffixComponents;
  }

  void
  setMaxSuffixComponents(int maxSuffixComponents)
  {
    if (maxSuffixComponents < 0)
      usage();
    m_maxSuffixComponents = maxSuffixComponents;
  }

  void
  setInterestLifetime(int interestLifetime)
  {
    if (interestLifetime < 0)
      usage();
    m_interestLifetime = ndn::time::milliseconds(interestLifetime);
  }

  void
  setPayloadOnly()
  {
    m_isPayloadOnlySet = true;
  }

  void
  setTimeout(int timeout)
  {
    if (timeout < 0)
      usage();
    m_timeout = ndn::time::milliseconds(timeout);
  }

  void
  setPrefixName(char* prefixName)
  {
    m_prefixName = prefixName;
    if (m_prefixName.length() == 0)
      usage();
  }

  ndn::time::milliseconds
  getDefaultInterestLifetime()
  {
    return ndn::time::seconds(4);
  }

  ndn::Interest
  createInterestPacket()
  {
    ndn::Name interestName(m_prefixName);
    ndn::Interest interestPacket(interestName);
    if (m_mustBeFresh)
      interestPacket.setMustBeFresh(true);
    if (m_isChildSelectorRightmost)
      interestPacket.setChildSelector(1);
    if (m_minSuffixComponents >= 0)
      interestPacket.setMinSuffixComponents(m_minSuffixComponents);
    if (m_maxSuffixComponents >= 0)
      interestPacket.setMaxSuffixComponents(m_maxSuffixComponents);
    if (m_interestLifetime < ndn::time::milliseconds::zero())
      interestPacket.setInterestLifetime(getDefaultInterestLifetime());
    else
      interestPacket.setInterestLifetime(m_interestLifetime);
    return interestPacket;
  }

  void
  onData(const ndn::Interest& interest, ndn::Data& data)
  {
    m_isDataReceived = true;
    if (m_isPayloadOnlySet)
      {
        const ndn::Block& block = data.getContent();
        std::cout.write(reinterpret_cast<const char*>(block.value()), block.value_size());
      }
    else
      {
        const ndn::Block& block = data.wireEncode();
        std::cout.write(reinterpret_cast<const char*>(block.wire()), block.size());
      }
  }

  void
  onTimeout(const ndn::Interest& interest)
  {
  }

  void
  run()
  {
    try
      {
        m_face.expressInterest(createInterestPacket(),
                               ndn::func_lib::bind(&NdnTlvPeek::onData,
                                                   this, _1, _2),
                               ndn::func_lib::bind(&NdnTlvPeek::onTimeout,
                                                   this, _1));
        if (m_timeout < ndn::time::milliseconds::zero())
          {
            if (m_interestLifetime < ndn::time::milliseconds::zero())
              m_face.processEvents(getDefaultInterestLifetime());
            else
              m_face.processEvents(m_interestLifetime);
          }
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
  isDataReceived() const
  {
    return m_isDataReceived;
  }

private:

  std::string m_programName;
  bool m_mustBeFresh;
  bool m_isChildSelectorRightmost;
  int m_minSuffixComponents;
  int m_maxSuffixComponents;
  ndn::time::milliseconds m_interestLifetime;
  bool m_isPayloadOnlySet;
  ndn::time::milliseconds m_timeout;
  std::string m_prefixName;
  bool m_isDataReceived;
  ndn::ptr_lib::shared_ptr<boost::asio::io_service> m_ioService;
  ndn::Face m_face;
};

}

int
main(int argc, char* argv[])
{
  int option;
  ndntlvpeek::NdnTlvPeek ndnTlvPeek (argv[0]);
  while ((option = getopt(argc, argv, "hfrm:M:l:pw:")) != -1)
    {
      switch (option) {
        case 'h':
          ndnTlvPeek.usage();
          break;
        case 'f':
          ndnTlvPeek.setMustBeFresh();
          break;
        case 'r':
          ndnTlvPeek.setRightmostChildSelector();
          break;
        case 'm':
          ndnTlvPeek.setMinSuffixComponents(atoi(optarg));
          break;
        case 'M':
          ndnTlvPeek.setMaxSuffixComponents(atoi(optarg));
          break;
        case 'l':
          ndnTlvPeek.setInterestLifetime(atoi(optarg));
          break;
        case 'p':
          ndnTlvPeek.setPayloadOnly();
          break;
        case 'w':
          ndnTlvPeek.setTimeout(atoi(optarg));
          break;
        default:
          ndnTlvPeek.usage();
      }
    }

  argc -= optind;
  argv += optind;

  if (argv[0] == 0)
    ndnTlvPeek.usage();

  ndnTlvPeek.setPrefixName(argv[0]);
  ndnTlvPeek.run();

  if (ndnTlvPeek.isDataReceived())
    return 0;
  else
    return 1;
}

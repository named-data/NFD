/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2014 University of Arizona.
 *
 * Author: Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 */

#include <ndn-cpp-dev/face.hpp>
#include <ndn-cpp-dev/name.hpp>
#include <ndn-cpp-dev/interest.hpp>

#include <ndn-cpp-dev/management/nfd-fib-entry.hpp>
#include <ndn-cpp-dev/management/nfd-face-status.hpp>
#include <ndn-cpp-dev/management/nfd-status.hpp>

namespace ndn {

class NfdStatus
{
public:
  NfdStatus(char* toolName)
    : m_toolName(toolName)
    , m_needVersionRetrieval(false)
    , m_needFaceStatusRetrieval(false)
    , m_needFibEnumerationRetrieval(false)
    , m_face()
  {
  }

  void
  usage()
  {
    std::cout << "\nUsage: " << m_toolName << " [options]\n"
      "Shows NFD Status Information\n"
      "Displays Version Information (NFD Version, Start Timestamp, Current Timestamp).\n"
      "Face Status Information via NFD Face Status Protocol (FaceID, URI, Counters).\n"
      "FIB Information via NFD FIB Enumeration Protocol (Prefix, Nexthop).\n"
      "If no options are provided, all information is retrieved.\n"
      "  [-v] - retrieve version information\n"
      "  [-f] - retrieve face status information\n"
      "  [-b] - retrieve FIB information\n"
      "  [-h] - print help and exit\n\n";
  }

  void
  enableVersionRetrieval()
  {
    m_needVersionRetrieval = true;
  }

  void
  enableFaceStatusRetrieval()
  {
    m_needFaceStatusRetrieval = true;
  }

  void
  enableFibEnumerationRetrieval()
  {
    m_needFibEnumerationRetrieval = true;
  }

  void
  onTimeout()
  {
    std::cerr << "Request timed out" << std::endl;
  }

  void
  fetchSegments(const Data& data, void (NfdStatus::*onDone)())
  {
    m_buffer->write((const char*)data.getContent().value(),
                    data.getContent().value_size());

    uint64_t currentSegment = data.getName().get(-1).toSegment();

    const name::Component& finalBlockId = data.getMetaInfo().getFinalBlockId();
    if (finalBlockId.empty() ||
        finalBlockId.toSegment() > currentSegment)
      {
        m_face.expressInterest(data.getName().getPrefix(-1).appendSegment(currentSegment+1),
                               bind(&NfdStatus::fetchSegments, this, _2, onDone),
                               bind(&NfdStatus::onTimeout, this));
      }
    else
      {
        return (this->*onDone)();
      }
  }

  void
  afterFetchedVersionInformation(const Data& data)
  {
    std::cout << "General NFD status:" << std::endl;

    nfd::Status status(data.getContent());
    std::cout << "               version="
              << status.getNfdVersion() << std::endl;
    std::cout << "             startTime="
              << time::toIsoString(status.getStartTimestamp()) << std::endl;
    std::cout << "           currentTime="
              << time::toIsoString(status.getCurrentTimestamp()) << std::endl;
    std::cout << "                uptime="
              << time::duration_cast<time::seconds>(status.getCurrentTimestamp()
                                                    - status.getStartTimestamp()) << std::endl;

    std::cout << "      nNameTreeEntries=" << status.getNNameTreeEntries()     << std::endl;
    std::cout << "           nFibEntries=" << status.getNFibEntries()          << std::endl;
    std::cout << "           nPitEntries=" << status.getNPitEntries()          << std::endl;
    std::cout << "  nMeasurementsEntries=" << status.getNMeasurementsEntries() << std::endl;
    std::cout << "            nCsEntries=" << status.getNCsEntries()           << std::endl;
    std::cout << "          nInInterests=" << status.getNInInterests()         << std::endl;
    std::cout << "         nOutInterests=" << status.getNOutInterests()        << std::endl;
    std::cout << "              nInDatas=" << status.getNInDatas()             << std::endl;
    std::cout << "             nOutDatas=" << status.getNOutDatas()            << std::endl;

    if (m_needFaceStatusRetrieval)
      {
        fetchFaceStatusInformation();
      }
    else if (m_needFibEnumerationRetrieval)
      {
        fetchFibEnumerationInformation();
      }
  }

  void
  fetchVersionInformation()
  {
    Interest interest("/localhost/nfd/status");
    interest.setChildSelector(1);
    interest.setMustBeFresh(true);
    m_face.expressInterest(
                           interest,
                           bind(&NfdStatus::afterFetchedVersionInformation, this, _2),
                           bind(&NfdStatus::onTimeout, this));
  }

  void
  afterFetchedFaceStatusInformation()
  {
    try
      {
        std::cout << "Faces:" << std::endl;

        ConstBufferPtr buf = m_buffer->buf();

        size_t offset = 0;
        while (offset < buf->size())
          {
            Block block(buf, buf->begin() + offset, buf->end(), false);
            offset += block.size();

            nfd::FaceStatus faceStatus(block);

            std::cout << "  faceid=" << faceStatus.getFaceId()
                      << " uri=" << faceStatus.getUri()
                      << " counters={"
                      << "in={" << faceStatus.getInInterest() << "i "
                                << faceStatus.getInData() << "d}"
                      << " out={" << faceStatus.getOutInterest() << "i "
                                 << faceStatus.getOutData() << "d}"
                      << "}" << std::endl;
          }

      }
    catch(Tlv::Error& e)
      {
        std::cerr << e.what() << std::endl;
      }

    if (m_needFibEnumerationRetrieval)
      {
        fetchFibEnumerationInformation();
      }
  }

  void
  fetchFaceStatusInformation()
  {
    m_buffer = make_shared<OBufferStream>();

    Interest interest("/localhost/nfd/faces/list");
    interest.setChildSelector(1);
    interest.setMustBeFresh(true);

    m_face.expressInterest(interest,
                           bind(&NfdStatus::fetchSegments, this, _2,
                                &NfdStatus::afterFetchedFaceStatusInformation),
                           bind(&NfdStatus::onTimeout, this));
  }

  void
  afterFetchedFibEnumerationInformation()
  {
    try
      {
        std::cout << "FIB:" << std::endl;

        ConstBufferPtr buf = m_buffer->buf();

        size_t offset = 0;
        while (offset < buf->size())
          {
            Block block(buf, buf->begin() + offset, buf->end(), false);
            offset += block.size();

            nfd::FibEntry fibEntry(block);

            std::cout << "  " << fibEntry.getPrefix() << " nexthops={";
            for (std::list<nfd::NextHopRecord>::const_iterator
                   nextHop = fibEntry.getNextHopRecords().begin();
                 nextHop != fibEntry.getNextHopRecords().end();
                 ++nextHop)
              {
                if (nextHop != fibEntry.getNextHopRecords().begin())
                  std::cout << ", ";
                std::cout << "faceid=" << nextHop->getFaceId()
                          << " (cost=" << nextHop->getCost() << ")";
              }
            std::cout << "}" << std::endl;
          }
      }
    catch(Tlv::Error& e)
      {
        std::cerr << e.what() << std::endl;
      }
  }

  void
  fetchFibEnumerationInformation()
  {
    m_buffer = make_shared<OBufferStream>();

    Interest interest("/localhost/nfd/fib/list");
    interest.setChildSelector(1);
    interest.setMustBeFresh(true);
    m_face.expressInterest(interest,
                           bind(&NfdStatus::fetchSegments, this, _2,
                                &NfdStatus::afterFetchedFibEnumerationInformation),
                           bind(&NfdStatus::onTimeout, this));
  }

  void
  fetchInformation()
  {
    if (!m_needVersionRetrieval &&
        !m_needFaceStatusRetrieval &&
        !m_needFibEnumerationRetrieval)
      {
        enableVersionRetrieval();
        enableFaceStatusRetrieval();
        enableFibEnumerationRetrieval();
      }

    if (m_needVersionRetrieval)
      {
        fetchVersionInformation();
      }
    else if (m_needFaceStatusRetrieval)
      {
        fetchFaceStatusInformation();
      }
    else if (m_needFibEnumerationRetrieval)
      {
        fetchFibEnumerationInformation();
      }

    m_face.processEvents();
  }

private:
  std::string m_toolName;
  bool m_needVersionRetrieval;
  bool m_needFaceStatusRetrieval;
  bool m_needFibEnumerationRetrieval;
  Face m_face;

  shared_ptr<OBufferStream> m_buffer;
};

}

int main( int argc, char* argv[] )
{
  int option;
  ndn::NfdStatus nfdStatus (argv[0]);

  while ((option = getopt(argc, argv, "hvfb")) != -1) {
    switch (option) {
    case 'h':
      nfdStatus.usage();
      break;
    case 'v':
      nfdStatus.enableVersionRetrieval();
      break;
    case 'f':
      nfdStatus.enableFaceStatusRetrieval();
      break;
    case 'b':
      nfdStatus.enableFibEnumerationRetrieval();
      break;
    default :
      nfdStatus.usage();
      return 1;
      break;
    }
  }

  try {
    nfdStatus.fetchInformation();
  }
  catch(std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }

  return 0;
}

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
 *
 * @author Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 */

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/interest.hpp>

#include <ndn-cxx/management/nfd-fib-entry.hpp>
#include <ndn-cxx/management/nfd-face-status.hpp>
#include <ndn-cxx/management/nfd-forwarder-status.hpp>

namespace ndn {

class NfdStatus
{
public:
  explicit
  NfdStatus(char* toolName)
    : m_toolName(toolName)
    , m_needVersionRetrieval(false)
    , m_needFaceStatusRetrieval(false)
    , m_needFibEnumerationRetrieval(false)
  {
  }

  void
  usage()
  {
    std::cout << "Usage: \n  " << m_toolName << " [options]\n\n"
      "Show NFD version and status information\n\n"
      "Options:\n"
      "  [-h] - print this help message\n"
      "  [-v] - retrieve version information\n"
      "  [-f] - retrieve face status information\n"
      "  [-b] - retrieve FIB information\n\n"
      "If no options are provided, all information is retrieved.\n"
      ;
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

    nfd::ForwarderStatus status(data.getContent());
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
    std::cout << "Faces:" << std::endl;

    ConstBufferPtr buf = m_buffer->buf();

    Block block;
    size_t offset = 0;
    while (offset < buf->size())
      {
        bool ok = Block::fromBuffer(buf, offset, block);
        if (!ok)
          {
            std::cerr << "ERROR: cannot decode FaceStatus TLV" << std::endl;
            break;
          }

        offset += block.size();

        nfd::FaceStatus faceStatus(block);

        std::cout << "  faceid=" << faceStatus.getFaceId()
                  << " remote=" << faceStatus.getRemoteUri()
                  << " local=" << faceStatus.getLocalUri()
                  << " counters={"
                  << "in={" << faceStatus.getNInInterests() << "i "
                  << faceStatus.getNInDatas() << "d}"
                  << " out={" << faceStatus.getNOutInterests() << "i "
                  << faceStatus.getNOutDatas() << "d}"
                  << "}" << std::endl;
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
    std::cout << "FIB:" << std::endl;

    ConstBufferPtr buf = m_buffer->buf();

    Block block;
    size_t offset = 0;
    while (offset < buf->size())
      {
        bool ok = Block::fromBuffer(buf, offset, block);
        if (!ok)
          {
            std::cerr << "ERROR: cannot decode FibEntry TLV" << std::endl;
            break;
          }
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
      return 0;
    case 'v':
      nfdStatus.enableVersionRetrieval();
      break;
    case 'f':
      nfdStatus.enableFaceStatusRetrieval();
      break;
    case 'b':
      nfdStatus.enableFibEnumerationRetrieval();
      break;
    default:
      nfdStatus.usage();
      return 1;
    }
  }

  try {
    nfdStatus.fetchInformation();
  }
  catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }

  return 0;
}

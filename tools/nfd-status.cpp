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
 *
 * @author Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 */

#include "version.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>

#include <ndn-cxx/management/nfd-forwarder-status.hpp>
#include <ndn-cxx/management/nfd-channel-status.hpp>
#include <ndn-cxx/management/nfd-face-status.hpp>
#include <ndn-cxx/management/nfd-fib-entry.hpp>
#include <ndn-cxx/management/nfd-rib-entry.hpp>
#include <ndn-cxx/management/nfd-strategy-choice.hpp>
#include <ndn-cxx/util/segment-fetcher.hpp>

#include <boost/algorithm/string/replace.hpp>
#include <list>

namespace ndn {

using util::SegmentFetcher;

class NfdStatus
{
public:
  explicit
  NfdStatus(char* toolName)
    : m_toolName(toolName)
    , m_needVersionRetrieval(false)
    , m_needChannelStatusRetrieval(false)
    , m_needFaceStatusRetrieval(false)
    , m_needFibEnumerationRetrieval(false)
    , m_needRibStatusRetrieval(false)
    , m_needStrategyChoiceRetrieval(false)
    , m_isOutputXml(false)
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
      "  [-c] - retrieve channel status information\n"
      "  [-f] - retrieve face status information\n"
      "  [-b] - retrieve FIB information\n"
      "  [-r] - retrieve RIB information\n"
      "  [-s] - retrieve configured strategy choice for NDN namespaces\n"
      "  [-x] - output NFD status information in XML format\n"
      "\n"
      "  [-V] - show version information of nfd-status and exit\n"
      "\n"
      "If no options are provided, all information is retrieved.\n"
      "If -x is provided, other options(-v, -c, etc.) are ignored, and all information is printed in XML format.\n"
      ;
  }

  void
  enableVersionRetrieval()
  {
    m_needVersionRetrieval = true;
  }

  void
  enableChannelStatusRetrieval()
  {
    m_needChannelStatusRetrieval = true;
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
  enableStrategyChoiceRetrieval()
  {
    m_needStrategyChoiceRetrieval = true;
  }

  void
  enableRibStatusRetrieval()
  {
    m_needRibStatusRetrieval = true;
  }

  void
  enableXmlOutput()
  {
    m_isOutputXml = true;
  }

  void
  onTimeout()
  {
    std::cerr << "Request timed out" << std::endl;

    runNextStep();
  }


  void
  onErrorFetch(uint32_t errorCode, const std::string& errorMsg)
  {
    std::cerr << "Error code:" << errorCode << ", message:" << errorMsg << std::endl;

    runNextStep();
  }

  void
  escapeSpecialCharacters(std::string *data)
  {
    using boost::algorithm::replace_all;
    replace_all(*data, "&",  "&amp;");
    replace_all(*data, "\"", "&quot;");
    replace_all(*data, "\'", "&apos;");
    replace_all(*data, "<",  "&lt;");
    replace_all(*data, ">",  "&gt;");
  }

  //////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////

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
  afterFetchedVersionInformation(const Data& data)
  {
    nfd::ForwarderStatus status(data.getContent());
    std::string nfdId;
    if (data.getSignature().hasKeyLocator())
      {
        const ndn::KeyLocator& locator = data.getSignature().getKeyLocator();
        if (locator.getType() == KeyLocator::KeyLocator_Name)
          nfdId = locator.getName().toUri();
        //todo: KeyDigest supporting
      }

    if (m_isOutputXml)
      {
        std::cout << "<generalStatus>";
        std::cout << "<nfdId>"
                  << nfdId << "</nfdId>";
        std::cout << "<version>"
                  << status.getNfdVersion() << "</version>";
        std::cout << "<startTime>"
                  << time::toString(status.getStartTimestamp(), "%Y-%m-%dT%H:%M:%S%F")
                  << "</startTime>";
        std::cout << "<currentTime>"
                  << time::toString(status.getCurrentTimestamp(), "%Y-%m-%dT%H:%M:%S%F")
                  << "</currentTime>";
        std::cout << "<uptime>PT"
                  << time::duration_cast<time::seconds>(status.getCurrentTimestamp()
                                                        - status.getStartTimestamp()).count()
                  << "S</uptime>";
        std::cout << "<nNameTreeEntries>"     << status.getNNameTreeEntries()
                  << "</nNameTreeEntries>";
        std::cout << "<nFibEntries>"          << status.getNFibEntries()
                  << "</nFibEntries>";
        std::cout << "<nPitEntries>"          << status.getNPitEntries()
                  << "</nPitEntries>";
        std::cout << "<nMeasurementsEntries>" << status.getNMeasurementsEntries()
                  << "</nMeasurementsEntries>";
        std::cout << "<nCsEntries>"           << status.getNCsEntries()
                  << "</nCsEntries>";
        std::cout << "<packetCounters>";
        std::cout << "<incomingPackets>";
        std::cout << "<nInterests>"           << status.getNInInterests()
                  << "</nInterests>";
        std::cout << "<nDatas>"               << status.getNInDatas()
                  << "</nDatas>";
        std::cout << "</incomingPackets>";
        std::cout << "<outgoingPackets>";
        std::cout << "<nInterests>"           << status.getNOutInterests()
                  << "</nInterests>";
        std::cout << "<nDatas>"               << status.getNOutDatas()
                  << "</nDatas>";
        std::cout << "</outgoingPackets>";
        std::cout << "</packetCounters>";
        std::cout << "</generalStatus>";
      }
    else
      {
        std::cout << "General NFD status:" << std::endl;
        std::cout << "                 nfdId="
                  << nfdId << std::endl;
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
      }

    runNextStep();
  }

  //////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////

  void
  fetchChannelStatusInformation()
  {
    m_buffer = make_shared<OBufferStream>();

    Interest interest("/localhost/nfd/faces/channels");
    interest.setChildSelector(1);
    interest.setMustBeFresh(true);

    SegmentFetcher::fetch(m_face, interest,
                          util::DontVerifySegment(),
                          bind(&NfdStatus::afterFetchedChannelStatusInformation, this, _1),
                          bind(&NfdStatus::onErrorFetch, this, _1, _2));
  }

  void
  afterFetchedChannelStatusInformation(const ConstBufferPtr& dataset)
  {
    if (m_isOutputXml) {
      std::cout << "<channels>";

      size_t offset = 0;
      while (offset < dataset->size()) {
        bool isOk = false;
        Block block;
        std::tie(isOk, block) = Block::fromBuffer(dataset, offset);
        if (!isOk) {
          std::cerr << "ERROR: cannot decode ChannelStatus TLV" << std::endl;
          break;
        }

        offset += block.size();

        nfd::ChannelStatus channelStatus(block);

        std::cout << "<channel>";
        std::string localUri(channelStatus.getLocalUri());
        escapeSpecialCharacters(&localUri);
        std::cout << "<localUri>" << localUri << "</localUri>";
        std::cout << "</channel>";
      }

      std::cout << "</channels>";
    }
    else {
      std::cout << "Channels:" << std::endl;

      size_t offset = 0;
      while (offset < dataset->size()) {
        bool isOk = false;
        Block block;
        std::tie(isOk, block) = Block::fromBuffer(dataset, offset);
        if (!isOk) {
          std::cerr << "ERROR: cannot decode ChannelStatus TLV" << std::endl;
          break;
        }

        offset += block.size();

        nfd::ChannelStatus channelStatus(block);
        std::cout << "  " << channelStatus.getLocalUri() << std::endl;
      }
    }

    runNextStep();
  }

  //////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////

  void
  fetchFaceStatusInformation()
  {
    m_buffer = make_shared<OBufferStream>();

    Interest interest("/localhost/nfd/faces/list");
    interest.setChildSelector(1);
    interest.setMustBeFresh(true);

    SegmentFetcher::fetch(m_face, interest,
                          util::DontVerifySegment(),
                          bind(&NfdStatus::afterFetchedFaceStatusInformation, this, _1),
                          bind(&NfdStatus::onErrorFetch, this, _1, _2));
  }

  void
  afterFetchedFaceStatusInformation(const ConstBufferPtr& dataset)
  {
    if (m_isOutputXml) {
      std::cout << "<faces>";

      size_t offset = 0;
      while (offset < dataset->size()) {
        bool isOk = false;
        Block block;
        std::tie(isOk, block) = Block::fromBuffer(dataset, offset);
        if (!isOk) {
          std::cerr << "ERROR: cannot decode FaceStatus TLV" << std::endl;
          break;
        }

        offset += block.size();

        nfd::FaceStatus faceStatus(block);

        std::cout << "<face>";
        std::cout << "<faceId>" << faceStatus.getFaceId() << "</faceId>";

        std::string remoteUri(faceStatus.getRemoteUri());
        escapeSpecialCharacters(&remoteUri);
        std::cout << "<remoteUri>" << remoteUri << "</remoteUri>";

        std::string localUri(faceStatus.getLocalUri());
        escapeSpecialCharacters(&localUri);
        std::cout << "<localUri>" << localUri << "</localUri>";

        if (faceStatus.hasExpirationPeriod()) {
          std::cout << "<expirationPeriod>PT"
                    << time::duration_cast<time::seconds>(faceStatus.getExpirationPeriod())
                       .count() << "S"
                    << "</expirationPeriod>";
        }

        std::cout << "<faceScope>" << faceStatus.getFaceScope()
                  << "</faceScope>";
        std::cout << "<facePersistency>" << faceStatus.getFacePersistency()
                  << "</facePersistency>";
        std::cout << "<linkType>" << faceStatus.getLinkType()
                  << "</linkType>";

        std::cout << "<packetCounters>";
        std::cout << "<incomingPackets>";
        std::cout << "<nInterests>"       << faceStatus.getNInInterests()
                  << "</nInterests>";
        std::cout << "<nDatas>"           << faceStatus.getNInDatas()
                  << "</nDatas>";
        std::cout << "</incomingPackets>";
        std::cout << "<outgoingPackets>";
        std::cout << "<nInterests>"       << faceStatus.getNOutInterests()
                  << "</nInterests>";
        std::cout << "<nDatas>"           << faceStatus.getNOutDatas()
                  << "</nDatas>";
        std::cout << "</outgoingPackets>";
        std::cout << "</packetCounters>";

        std::cout << "<byteCounters>";
        std::cout << "<incomingBytes>"    << faceStatus.getNInBytes()
                  << "</incomingBytes>";
        std::cout << "<outgoingBytes>"    << faceStatus.getNOutBytes()
                  << "</outgoingBytes>";
        std::cout << "</byteCounters>";

        std::cout << "</face>";
      }
      std::cout << "</faces>";
    }
    else {
      std::cout << "Faces:" << std::endl;

      size_t offset = 0;
      while (offset < dataset->size()) {
        bool isOk = false;
        Block block;
        std::tie(isOk, block) = Block::fromBuffer(dataset, offset);
        if (!isOk) {
          std::cerr << "ERROR: cannot decode FaceStatus TLV" << std::endl;
          break;
        }

        offset += block.size();

        nfd::FaceStatus faceStatus(block);

        std::cout << "  faceid=" << faceStatus.getFaceId()
                  << " remote=" << faceStatus.getRemoteUri()
                  << " local=" << faceStatus.getLocalUri();
        if (faceStatus.hasExpirationPeriod()) {
          std::cout  << " expires="
                     << time::duration_cast<time::seconds>(faceStatus.getExpirationPeriod())
                        .count() << "s";
        }
        std::cout << " counters={"
                  << "in={" << faceStatus.getNInInterests() << "i "
                  << faceStatus.getNInDatas() << "d "
                  << faceStatus.getNInBytes() << "B}"
                  << " out={" << faceStatus.getNOutInterests() << "i "
                  << faceStatus.getNOutDatas() << "d "
                  << faceStatus.getNOutBytes() << "B}"
                  << "}";
        std::cout << " " << faceStatus.getFaceScope()
                  << " " << faceStatus.getFacePersistency()
                  << " " << faceStatus.getLinkType();
        std::cout << std::endl;
      }
    }

    runNextStep();
  }

  //////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////

  void
  fetchFibEnumerationInformation()
  {
    m_buffer = make_shared<OBufferStream>();

    Interest interest("/localhost/nfd/fib/list");
    interest.setChildSelector(1);
    interest.setMustBeFresh(true);

    SegmentFetcher::fetch(m_face, interest,
                          util::DontVerifySegment(),
                          bind(&NfdStatus::afterFetchedFibEnumerationInformation, this, _1),
                          bind(&NfdStatus::onErrorFetch, this, _1, _2));
  }

  void
  afterFetchedFibEnumerationInformation(const ConstBufferPtr& dataset)
  {
    if (m_isOutputXml) {
      std::cout << "<fib>";

      size_t offset = 0;
      while (offset < dataset->size()) {
        bool isOk = false;
        Block block;
        std::tie(isOk, block) = Block::fromBuffer(dataset, offset);
        if (!isOk) {
          std::cerr << "ERROR: cannot decode FibEntry TLV";
          break;
        }
        offset += block.size();

        nfd::FibEntry fibEntry(block);

        std::cout << "<fibEntry>";
        std::string prefix(fibEntry.getPrefix().toUri());
        escapeSpecialCharacters(&prefix);
        std::cout << "<prefix>" << prefix << "</prefix>";
        std::cout << "<nextHops>";
        for (const nfd::NextHopRecord& nextHop : fibEntry.getNextHopRecords()) {
          std::cout << "<nextHop>" ;
          std::cout << "<faceId>"  << nextHop.getFaceId() << "</faceId>";
          std::cout << "<cost>"    << nextHop.getCost()   << "</cost>";
          std::cout << "</nextHop>";
        }
        std::cout << "</nextHops>";
        std::cout << "</fibEntry>";
      }

      std::cout << "</fib>";
    }
    else {
      std::cout << "FIB:" << std::endl;

      size_t offset = 0;
      while (offset < dataset->size()) {
        bool isOk = false;
        Block block;
        std::tie(isOk, block) = Block::fromBuffer(dataset, offset);
        if (!isOk) {
          std::cerr << "ERROR: cannot decode FibEntry TLV" << std::endl;
          break;
        }
        offset += block.size();

        nfd::FibEntry fibEntry(block);

        std::cout << "  " << fibEntry.getPrefix() << " nexthops={";
        bool isFirst = true;
        for (const nfd::NextHopRecord& nextHop : fibEntry.getNextHopRecords()) {
          if (isFirst)
            isFirst = false;
          else
            std::cout << ", ";

          std::cout << "faceid=" << nextHop.getFaceId()
                    << " (cost=" << nextHop.getCost() << ")";
        }
        std::cout << "}" << std::endl;
      }
    }

    runNextStep();
  }

  //////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////

  void
  fetchStrategyChoiceInformation()
  {
    m_buffer = make_shared<OBufferStream>();

    Interest interest("/localhost/nfd/strategy-choice/list");
    interest.setChildSelector(1);
    interest.setMustBeFresh(true);

    SegmentFetcher::fetch(m_face, interest,
                          util::DontVerifySegment(),
                          bind(&NfdStatus::afterFetchedStrategyChoiceInformationInformation, this, _1),
                          bind(&NfdStatus::onErrorFetch, this, _1, _2));
  }

  void
  afterFetchedStrategyChoiceInformationInformation(const ConstBufferPtr& dataset)
  {
    if (m_isOutputXml) {
      std::cout << "<strategyChoices>";

      size_t offset = 0;
      while (offset < dataset->size()) {
        bool isOk = false;
        Block block;
        std::tie(isOk, block) = Block::fromBuffer(dataset, offset);
        if (!isOk) {
          std::cerr << "ERROR: cannot decode StrategyChoice TLV";
          break;
        }
        offset += block.size();

        nfd::StrategyChoice strategyChoice(block);

        std::cout << "<strategyChoice>";

        std::string name(strategyChoice.getName().toUri());
        escapeSpecialCharacters(&name);
        std::cout << "<namespace>" << name << "</namespace>";
        std::cout << "<strategy>";

        std::string strategy(strategyChoice.getStrategy().toUri());
        escapeSpecialCharacters(&strategy);

        std::cout << "<name>" << strategy << "</name>";
        std::cout << "</strategy>";
        std::cout << "</strategyChoice>";
      }

      std::cout << "</strategyChoices>";
    }
    else {
      std::cout << "Strategy choices:" << std::endl;

      size_t offset = 0;
      while (offset < dataset->size()) {
        bool isOk = false;
        Block block;
        std::tie(isOk, block) = Block::fromBuffer(dataset, offset);
        if (!isOk) {
          std::cerr << "ERROR: cannot decode StrategyChoice TLV" << std::endl;
          break;
        }
        offset += block.size();

        nfd::StrategyChoice strategyChoice(block);

        std::cout << "  " << strategyChoice.getName()
                  << " strategy=" << strategyChoice.getStrategy() << std::endl;
      }
    }

    runNextStep();
  }

  void
  fetchRibStatusInformation()
  {
    m_buffer = make_shared<OBufferStream>();

    Interest interest("/localhost/nfd/rib/list");
    interest.setChildSelector(1);
    interest.setMustBeFresh(true);

    SegmentFetcher::fetch(m_face, interest,
                          util::DontVerifySegment(),
                          bind(&NfdStatus::afterFetchedRibStatusInformation, this, _1),
                          bind(&NfdStatus::onErrorFetch, this, _1, _2));
  }

  void
  afterFetchedRibStatusInformation(const ConstBufferPtr& dataset)
  {
    if (m_isOutputXml) {
      std::cout << "<rib>";

      size_t offset = 0;
      while (offset < dataset->size()) {
        bool isOk = false;
        Block block;
        std::tie(isOk, block) = Block::fromBuffer(dataset, offset);
        if (!isOk) {
          std::cerr << "ERROR: cannot decode RibEntry TLV";
          break;
        }
        offset += block.size();

        nfd::RibEntry ribEntry(block);

        std::cout << "<ribEntry>";
        std::string prefix(ribEntry.getName().toUri());
        escapeSpecialCharacters(&prefix);
        std::cout << "<prefix>" << prefix << "</prefix>";
        std::cout << "<routes>";
        for (const nfd::Route& nextRoute : ribEntry) {
          std::cout << "<route>" ;
          std::cout << "<faceId>"  << nextRoute.getFaceId() << "</faceId>";
          std::cout << "<origin>"  << nextRoute.getOrigin() << "</origin>";
          std::cout << "<cost>"    << nextRoute.getCost()   << "</cost>";

          std::cout << "<flags>";
          if (nextRoute.isChildInherit())
            std::cout << "<childInherit/>";
          if (nextRoute.isRibCapture())
            std::cout << "<ribCapture/>";
          std::cout << "</flags>";

          if (!nextRoute.hasInfiniteExpirationPeriod()) {
            std::cout << "<expirationPeriod>PT"
                      << time::duration_cast<time::seconds>(nextRoute.getExpirationPeriod())
                         .count() << "S"
                      << "</expirationPeriod>";
          }
          std::cout << "</route>";
        }
        std::cout << "</routes>";
        std::cout << "</ribEntry>";
      }

      std::cout << "</rib>";
    }
    else {
      std::cout << "RIB:" << std::endl;

      size_t offset = 0;
      while (offset < dataset->size()) {
        bool isOk = false;
        Block block;
        std::tie(isOk, block) = Block::fromBuffer(dataset, offset);
        if (!isOk) {
          std::cerr << "ERROR: cannot decode RibEntry TLV" << std::endl;
          break;
        }

        offset += block.size();

        nfd::RibEntry ribEntry(block);

        std::cout << "  " << ribEntry.getName().toUri() << " route={";
        bool isFirst = true;
        for (const nfd::Route& nextRoute : ribEntry) {
          if (isFirst)
            isFirst = false;
          else
            std::cout << ", ";

          std::cout << "faceid="   << nextRoute.getFaceId()
                    << " (origin="  << nextRoute.getOrigin()
                    << " cost="    << nextRoute.getCost();
          if (!nextRoute.hasInfiniteExpirationPeriod()) {
            std::cout << " expires="
                      << time::duration_cast<time::seconds>(nextRoute.getExpirationPeriod())
                         .count() << "s";
          }

          if (nextRoute.isChildInherit())
            std::cout << " ChildInherit";
          if (nextRoute.isRibCapture())
            std::cout << " RibCapture";

          std::cout << ")";
        }
        std::cout << "}" << std::endl;
      }
    }

    runNextStep();
  }


  void
  fetchInformation()
  {
    if (m_isOutputXml ||
        (!m_needVersionRetrieval &&
         !m_needChannelStatusRetrieval &&
         !m_needFaceStatusRetrieval &&
         !m_needFibEnumerationRetrieval &&
         !m_needRibStatusRetrieval &&
         !m_needStrategyChoiceRetrieval))
      {
        enableVersionRetrieval();
        enableChannelStatusRetrieval();
        enableFaceStatusRetrieval();
        enableFibEnumerationRetrieval();
        enableRibStatusRetrieval();
        enableStrategyChoiceRetrieval();
      }

    if (m_isOutputXml)
      m_fetchSteps.push_back(bind(&NfdStatus::printXmlHeader, this));

    if (m_needVersionRetrieval)
      m_fetchSteps.push_back(bind(&NfdStatus::fetchVersionInformation, this));

    if (m_needChannelStatusRetrieval)
      m_fetchSteps.push_back(bind(&NfdStatus::fetchChannelStatusInformation, this));

    if (m_needFaceStatusRetrieval)
      m_fetchSteps.push_back(bind(&NfdStatus::fetchFaceStatusInformation, this));

    if (m_needFibEnumerationRetrieval)
      m_fetchSteps.push_back(bind(&NfdStatus::fetchFibEnumerationInformation, this));

    if (m_needRibStatusRetrieval)
      m_fetchSteps.push_back(bind(&NfdStatus::fetchRibStatusInformation, this));

    if (m_needStrategyChoiceRetrieval)
      m_fetchSteps.push_back(bind(&NfdStatus::fetchStrategyChoiceInformation, this));

    if (m_isOutputXml)
      m_fetchSteps.push_back(bind(&NfdStatus::printXmlFooter, this));

    runNextStep();
    m_face.processEvents();
  }

private:
  void
  printXmlHeader()
  {
    std::cout << "<?xml version=\"1.0\"?>";
    std::cout << "<nfdStatus xmlns=\"ndn:/localhost/nfd/status/1\">";

    runNextStep();
  }

  void
  printXmlFooter()
  {
    std::cout << "</nfdStatus>";

    runNextStep();
  }

  void
  runNextStep()
  {
    if (m_fetchSteps.empty())
      return;

    function<void()> nextStep = m_fetchSteps.front();
    m_fetchSteps.pop_front();
    nextStep();
  }

private:
  std::string m_toolName;
  bool m_needVersionRetrieval;
  bool m_needChannelStatusRetrieval;
  bool m_needFaceStatusRetrieval;
  bool m_needFibEnumerationRetrieval;
  bool m_needRibStatusRetrieval;
  bool m_needStrategyChoiceRetrieval;
  bool m_isOutputXml;
  Face m_face;

  shared_ptr<OBufferStream> m_buffer;

  std::deque<function<void()> > m_fetchSteps;
};

}

int main(int argc, char* argv[])
{
  int option;
  ndn::NfdStatus nfdStatus(argv[0]);

  while ((option = getopt(argc, argv, "hvcfbrsxV")) != -1) {
    switch (option) {
    case 'h':
      nfdStatus.usage();
      return 0;
    case 'v':
      nfdStatus.enableVersionRetrieval();
      break;
    case 'c':
      nfdStatus.enableChannelStatusRetrieval();
      break;
    case 'f':
      nfdStatus.enableFaceStatusRetrieval();
      break;
    case 'b':
      nfdStatus.enableFibEnumerationRetrieval();
      break;
    case 'r':
      nfdStatus.enableRibStatusRetrieval();
      break;
    case 's':
      nfdStatus.enableStrategyChoiceRetrieval();
      break;
    case 'x':
      nfdStatus.enableXmlOutput();
      break;
    case 'V':
      std::cout << NFD_VERSION_BUILD_STRING << std::endl;
      return 0;
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

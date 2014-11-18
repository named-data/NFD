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

#include "face-manager.hpp"

#include "core/logger.hpp"
#include "core/network-interface.hpp"
#include "fw/face-table.hpp"
#include "face/tcp-factory.hpp"
#include "face/udp-factory.hpp"
#include "core/config-file.hpp"

#ifdef HAVE_UNIX_SOCKETS
#include "face/unix-stream-factory.hpp"
#endif // HAVE_UNIX_SOCKETS

#ifdef HAVE_LIBPCAP
#include "face/ethernet-factory.hpp"
#endif // HAVE_LIBPCAP

#ifdef HAVE_WEBSOCKET
#include "face/websocket-factory.hpp"
#endif // HAVE_WEBSOCKET

#include <ndn-cxx/management/nfd-face-event-notification.hpp>
#include <ndn-cxx/management/nfd-face-query-filter.hpp>

namespace nfd {

NFD_LOG_INIT("FaceManager");

const Name FaceManager::COMMAND_PREFIX("/localhost/nfd/faces");

const size_t FaceManager::COMMAND_UNSIGNED_NCOMPS =
  FaceManager::COMMAND_PREFIX.size() +
  1 + // verb
  1;  // verb parameters

const size_t FaceManager::COMMAND_SIGNED_NCOMPS =
  FaceManager::COMMAND_UNSIGNED_NCOMPS +
  4; // (timestamp, nonce, signed info tlv, signature tlv)

const FaceManager::SignedVerbAndProcessor FaceManager::SIGNED_COMMAND_VERBS[] =
  {
    SignedVerbAndProcessor(
                           Name::Component("create"),
                           &FaceManager::createFace
                           ),

    SignedVerbAndProcessor(
                           Name::Component("destroy"),
                           &FaceManager::destroyFace
                           ),

    SignedVerbAndProcessor(
                           Name::Component("enable-local-control"),
                           &FaceManager::enableLocalControl
                           ),

    SignedVerbAndProcessor(
                           Name::Component("disable-local-control"),
                           &FaceManager::disableLocalControl
                           ),
  };

const FaceManager::UnsignedVerbAndProcessor FaceManager::UNSIGNED_COMMAND_VERBS[] =
  {
    UnsignedVerbAndProcessor(
                             Name::Component("list"),
                             &FaceManager::listFaces
                             ),

    UnsignedVerbAndProcessor(
                             Name::Component("events"),
                             &FaceManager::ignoreUnsignedVerb
                             ),

    UnsignedVerbAndProcessor(
                             Name::Component("channels"),
                             &FaceManager::listChannels
                             ),

    UnsignedVerbAndProcessor(
                             Name::Component("query"),
                             &FaceManager::listQueriedFaces
                             ),
  };

const Name FaceManager::FACES_LIST_DATASET_PREFIX("/localhost/nfd/faces/list");
const size_t FaceManager::FACES_LIST_DATASET_NCOMPS = FACES_LIST_DATASET_PREFIX.size();

const Name FaceManager::FACE_EVENTS_PREFIX("/localhost/nfd/faces/events");

const Name FaceManager::CHANNELS_LIST_DATASET_PREFIX("/localhost/nfd/faces/channels");
const size_t FaceManager::CHANNELS_LIST_DATASET_NCOMPS = CHANNELS_LIST_DATASET_PREFIX.size();

const Name FaceManager::FACES_QUERY_DATASET_PREFIX("/localhost/nfd/faces/query");
const size_t FaceManager::FACES_QUERY_DATASET_NCOMPS = FACES_QUERY_DATASET_PREFIX.size() + 1;

FaceManager::FaceManager(FaceTable& faceTable,
                         shared_ptr<InternalFace> face,
                         ndn::KeyChain& keyChain)
  : ManagerBase(face, FACE_MANAGER_PRIVILEGE, keyChain)
  , m_faceTable(faceTable)
  , m_faceStatusPublisher(m_faceTable, *m_face, FACES_LIST_DATASET_PREFIX, keyChain)
  , m_channelStatusPublisher(m_factories, *m_face, CHANNELS_LIST_DATASET_PREFIX, keyChain)
  , m_notificationStream(*m_face, FACE_EVENTS_PREFIX, keyChain)
  , m_signedVerbDispatch(SIGNED_COMMAND_VERBS,
                         SIGNED_COMMAND_VERBS +
                         (sizeof(SIGNED_COMMAND_VERBS) / sizeof(SignedVerbAndProcessor)))
  , m_unsignedVerbDispatch(UNSIGNED_COMMAND_VERBS,
                           UNSIGNED_COMMAND_VERBS +
                           (sizeof(UNSIGNED_COMMAND_VERBS) / sizeof(UnsignedVerbAndProcessor)))

{
  face->setInterestFilter("/localhost/nfd/faces",
                          bind(&FaceManager::onFaceRequest, this, _2));

  m_faceTable.onAdd    += bind(&FaceManager::onAddFace, this, _1);
  m_faceTable.onRemove += bind(&FaceManager::onRemoveFace, this, _1);
}

FaceManager::~FaceManager()
{

}

void
FaceManager::setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler("face_system",
                               bind(&FaceManager::onConfig, this, _1, _2, _3));
}


void
FaceManager::onConfig(const ConfigSection& configSection,
                      bool isDryRun,
                      const std::string& filename)
{
  bool hasSeenUnix = false;
  bool hasSeenTcp = false;
  bool hasSeenUdp = false;
  bool hasSeenEther = false;
  bool hasSeenWebSocket = false;

  const std::vector<NetworkInterfaceInfo> nicList(listNetworkInterfaces());

  for (const auto& item : configSection)
    {
      if (item.first == "unix")
        {
          if (hasSeenUnix)
            throw Error("Duplicate \"unix\" section");
          hasSeenUnix = true;

          processSectionUnix(item.second, isDryRun);
        }
      else if (item.first == "tcp")
        {
          if (hasSeenTcp)
            throw Error("Duplicate \"tcp\" section");
          hasSeenTcp = true;

          processSectionTcp(item.second, isDryRun);
        }
      else if (item.first == "udp")
        {
          if (hasSeenUdp)
            throw Error("Duplicate \"udp\" section");
          hasSeenUdp = true;

          processSectionUdp(item.second, isDryRun, nicList);
        }
      else if (item.first == "ether")
        {
          if (hasSeenEther)
            throw Error("Duplicate \"ether\" section");
          hasSeenEther = true;

          processSectionEther(item.second, isDryRun, nicList);
        }
      else if (item.first == "websocket")
        {
          if (hasSeenWebSocket)
            throw Error("Duplicate \"websocket\" section");
          hasSeenWebSocket = true;

          processSectionWebSocket(item.second, isDryRun);
        }
      else
        {
          throw Error("Unrecognized option \"" + item.first + "\"");
        }
    }
}

void
FaceManager::processSectionUnix(const ConfigSection& configSection, bool isDryRun)
{
  // ; the unix section contains settings of Unix stream faces and channels
  // unix
  // {
  //   path /var/run/nfd.sock ; Unix stream listener path
  // }

#if defined(HAVE_UNIX_SOCKETS)

  std::string path = "/var/run/nfd.sock";

  for (ConfigSection::const_iterator i = configSection.begin();
       i != configSection.end();
       ++i)
    {
      if (i->first == "path")
        {
          path = i->second.get_value<std::string>();
        }
      else
        {
          throw ConfigFile::Error("Unrecognized option \"" + i->first + "\" in \"unix\" section");
        }
    }

  if (!isDryRun)
    {
      if (m_factories.count("unix") > 0)
        {
          return;
          // shared_ptr<UnixStreamFactory> factory
          //   = static_pointer_cast<UnixStreamFactory>(m_factories["unix"]);
          // shared_ptr<UnixStreamChannel> unixChannel = factory->findChannel(path);

          // if (static_cast<bool>(unixChannel))
          //   {
          //     return;
          //   }
        }

      shared_ptr<UnixStreamFactory> factory = make_shared<UnixStreamFactory>();
      shared_ptr<UnixStreamChannel> unixChannel = factory->createChannel(path);

      // Should acceptFailed callback be used somehow?
      unixChannel->listen(bind(&FaceTable::add, &m_faceTable, _1),
                          UnixStreamChannel::ConnectFailedCallback());

      m_factories.insert(std::make_pair("unix", factory));
    }
#else
  throw ConfigFile::Error("NFD was compiled without Unix sockets support, "
                          "cannot process \"unix\" section");
#endif // HAVE_UNIX_SOCKETS

}

void
FaceManager::processSectionTcp(const ConfigSection& configSection, bool isDryRun)
{
  // ; the tcp section contains settings of TCP faces and channels
  // tcp
  // {
  //   listen yes ; set to 'no' to disable TCP listener, default 'yes'
  //   port 6363 ; TCP listener port number
  // }

  std::string port = "6363";
  bool needToListen = true;
  bool enableV4 = true;
  bool enableV6 = true;

  for (ConfigSection::const_iterator i = configSection.begin();
       i != configSection.end();
       ++i)
    {
      if (i->first == "port")
        {
          port = i->second.get_value<std::string>();
          try
            {
              uint16_t portNo = boost::lexical_cast<uint16_t>(port);
              NFD_LOG_TRACE("TCP port set to " << portNo);
            }
          catch (const std::bad_cast& error)
            {
              throw ConfigFile::Error("Invalid value for option " +
                                      i->first + "\" in \"tcp\" section");
            }
        }
      else if (i->first == "listen")
        {
          needToListen = parseYesNo(i, i->first, "tcp");
        }
      else if (i->first == "enable_v4")
        {
          enableV4 = parseYesNo(i, i->first, "tcp");
        }
      else if (i->first == "enable_v6")
        {
          enableV6 = parseYesNo(i, i->first, "tcp");
        }
      else
        {
          throw ConfigFile::Error("Unrecognized option \"" + i->first + "\" in \"tcp\" section");
        }
    }

  if (!enableV4 && !enableV6)
    {
      throw ConfigFile::Error("IPv4 and IPv6 channels have been disabled."
                              " Remove \"tcp\" section to disable TCP channels or"
                              " re-enable at least one channel type.");
    }

  if (!isDryRun)
    {
      if (m_factories.count("tcp") > 0)
        {
          return;
        }

      shared_ptr<TcpFactory> factory = make_shared<TcpFactory>(port);
      m_factories.insert(std::make_pair("tcp", factory));

      if (enableV4)
        {
          shared_ptr<TcpChannel> ipv4Channel = factory->createChannel("0.0.0.0", port);
          if (needToListen)
            {
              // Should acceptFailed callback be used somehow?
              ipv4Channel->listen(bind(&FaceTable::add, &m_faceTable, _1),
                                  TcpChannel::ConnectFailedCallback());
            }

          m_factories.insert(std::make_pair("tcp4", factory));
        }

      if (enableV6)
        {
          shared_ptr<TcpChannel> ipv6Channel = factory->createChannel("::", port);
          if (needToListen)
            {
              // Should acceptFailed callback be used somehow?
              ipv6Channel->listen(bind(&FaceTable::add, &m_faceTable, _1),
                                  TcpChannel::ConnectFailedCallback());
            }

          m_factories.insert(std::make_pair("tcp6", factory));
        }
    }
}

void
FaceManager::processSectionUdp(const ConfigSection& configSection,
                               bool isDryRun,
                               const std::vector<NetworkInterfaceInfo>& nicList)
{
  // ; the udp section contains settings of UDP faces and channels
  // udp
  // {
  //   port 6363 ; UDP unicast port number
  //   idle_timeout 30 ; idle time (seconds) before closing a UDP unicast face
  //   keep_alive_interval 25; interval (seconds) between keep-alive refreshes

  //   ; NFD creates one UDP multicast face per NIC
  //   mcast yes ; set to 'no' to disable UDP multicast, default 'yes'
  //   mcast_port 56363 ; UDP multicast port number
  //   mcast_group 224.0.23.170 ; UDP multicast group (IPv4 only)
  // }

  std::string port = "6363";
  bool enableV4 = true;
  bool enableV6 = true;
  size_t timeout = 30;
  size_t keepAliveInterval = 25;
  bool useMcast = true;
  std::string mcastGroup = "224.0.23.170";
  std::string mcastPort = "56363";


  for (ConfigSection::const_iterator i = configSection.begin();
       i != configSection.end();
       ++i)
    {
      if (i->first == "port")
        {
          port = i->second.get_value<std::string>();
          try
            {
              uint16_t portNo = boost::lexical_cast<uint16_t>(port);
              NFD_LOG_TRACE("UDP port set to " << portNo);
            }
          catch (const std::bad_cast& error)
            {
              throw ConfigFile::Error("Invalid value for option " +
                                      i->first + "\" in \"udp\" section");
            }
        }
      else if (i->first == "enable_v4")
        {
          enableV4 = parseYesNo(i, i->first, "udp");
        }
      else if (i->first == "enable_v6")
        {
          enableV6 = parseYesNo(i, i->first, "udp");
        }
      else if (i->first == "idle_timeout")
        {
          try
            {
              timeout = i->second.get_value<size_t>();
            }
          catch (const std::exception& e)
            {
              throw ConfigFile::Error("Invalid value for option \"" +
                                      i->first + "\" in \"udp\" section");
            }
        }
      else if (i->first == "keep_alive_interval")
        {
          try
            {
              keepAliveInterval = i->second.get_value<size_t>();

              /// \todo Make use of keepAliveInterval
              (void)(keepAliveInterval);
            }
          catch (const std::exception& e)
            {
              throw ConfigFile::Error("Invalid value for option \"" +
                                      i->first + "\" in \"udp\" section");
            }
        }
      else if (i->first == "mcast")
        {
          useMcast = parseYesNo(i, i->first, "udp");
        }
      else if (i->first == "mcast_port")
        {
          mcastPort = i->second.get_value<std::string>();
          try
            {
              uint16_t portNo = boost::lexical_cast<uint16_t>(mcastPort);
              NFD_LOG_TRACE("UDP multicast port set to " << portNo);
            }
          catch (const std::bad_cast& error)
            {
              throw ConfigFile::Error("Invalid value for option " +
                                      i->first + "\" in \"udp\" section");
            }
        }
      else if (i->first == "mcast_group")
        {
          using namespace boost::asio::ip;
          mcastGroup = i->second.get_value<std::string>();
          try
            {
              address mcastGroupTest = address::from_string(mcastGroup);
              if (!mcastGroupTest.is_v4())
                {
                  throw ConfigFile::Error("Invalid value for option \"" +
                                          i->first + "\" in \"udp\" section");
                }
            }
          catch(const std::runtime_error& e)
            {
              throw ConfigFile::Error("Invalid value for option \"" +
                                      i->first + "\" in \"udp\" section");
            }
        }
      else
        {
          throw ConfigFile::Error("Unrecognized option \"" + i->first + "\" in \"udp\" section");
        }
    }

  if (!enableV4 && !enableV6)
    {
      throw ConfigFile::Error("IPv4 and IPv6 channels have been disabled."
                              " Remove \"udp\" section to disable UDP channels or"
                              " re-enable at least one channel type.");
    }
  else if (useMcast && !enableV4)
    {
      throw ConfigFile::Error("IPv4 multicast requested, but IPv4 channels"
                              " have been disabled (conflicting configuration options set)");
    }

  /// \todo what is keep alive interval used for?

  if (!isDryRun)
    {
      shared_ptr<UdpFactory> factory;
      bool isReload = false;
      if (m_factories.count("udp") > 0) {
        isReload = true;
        factory = static_pointer_cast<UdpFactory>(m_factories["udp"]);
      }
      else {
        factory = make_shared<UdpFactory>(port);
        m_factories.insert(std::make_pair("udp", factory));
      }

      if (!isReload && enableV4)
        {
          shared_ptr<UdpChannel> v4Channel =
            factory->createChannel("0.0.0.0", port, time::seconds(timeout));

          v4Channel->listen(bind(&FaceTable::add, &m_faceTable, _1),
                            UdpChannel::ConnectFailedCallback());

          m_factories.insert(std::make_pair("udp4", factory));
        }

      if (!isReload && enableV6)
        {
          shared_ptr<UdpChannel> v6Channel =
            factory->createChannel("::", port, time::seconds(timeout));

          v6Channel->listen(bind(&FaceTable::add, &m_faceTable, _1),
                            UdpChannel::ConnectFailedCallback());
          m_factories.insert(std::make_pair("udp6", factory));
        }

      if (useMcast && enableV4)
        {
          std::vector<NetworkInterfaceInfo> ipv4MulticastInterfaces;
          for (const auto& nic : nicList)
            {
              if (nic.isUp() && nic.isMulticastCapable() && !nic.ipv4Addresses.empty())
                {
                  ipv4MulticastInterfaces.push_back(nic);
                }
            }

          bool isNicNameNecessary = false;
#if defined(__linux__)
          if (ipv4MulticastInterfaces.size() > 1)
            {
              // On Linux if we have more than one MulticastUdpFace
              // we need to specify the name of the interface
              isNicNameNecessary = true;
            }
#endif

          std::list<shared_ptr<MulticastUdpFace> > multicastFacesToRemove;
          for (UdpFactory::MulticastFaceMap::const_iterator i =
                 factory->getMulticastFaces().begin();
               i != factory->getMulticastFaces().end();
               ++i)
            {
              multicastFacesToRemove.push_back(i->second);
            }

          for (const auto& nic : ipv4MulticastInterfaces)
            {
              shared_ptr<MulticastUdpFace> newFace;
              newFace = factory->createMulticastFace(nic.ipv4Addresses[0].to_string(),
                                                     mcastGroup,
                                                     mcastPort,
                                                     isNicNameNecessary ? nic.name : "");
              addCreatedFaceToForwarder(newFace);
              multicastFacesToRemove.remove(newFace);
            }

          for (std::list<shared_ptr<MulticastUdpFace> >::iterator i =
                 multicastFacesToRemove.begin();
               i != multicastFacesToRemove.end();
               ++i)
            {
              (*i)->close();
            }
        }
      else
        {
          std::list<shared_ptr<MulticastUdpFace> > multicastFacesToRemove;
          for (UdpFactory::MulticastFaceMap::const_iterator i =
                 factory->getMulticastFaces().begin();
               i != factory->getMulticastFaces().end();
               ++i)
            {
              multicastFacesToRemove.push_back(i->second);
            }

          for (std::list<shared_ptr<MulticastUdpFace> >::iterator i =
                 multicastFacesToRemove.begin();
               i != multicastFacesToRemove.end();
               ++i)
            {
              (*i)->close();
            }
        }
    }
}

void
FaceManager::processSectionEther(const ConfigSection& configSection,
                                 bool isDryRun,
                                 const std::vector<NetworkInterfaceInfo>& nicList)
{
  // ; the ether section contains settings of Ethernet faces and channels
  // ether
  // {
  //   ; NFD creates one Ethernet multicast face per NIC
  //   mcast yes ; set to 'no' to disable Ethernet multicast, default 'yes'
  //   mcast_group 01:00:5E:00:17:AA ; Ethernet multicast group
  // }

#if defined(HAVE_LIBPCAP)
  bool useMcast = true;
  ethernet::Address mcastGroup(ethernet::getDefaultMulticastAddress());

  for (ConfigSection::const_iterator i = configSection.begin();
       i != configSection.end();
       ++i)
    {
      if (i->first == "mcast")
        {
          useMcast = parseYesNo(i, i->first, "ether");
        }

      else if (i->first == "mcast_group")
        {
          mcastGroup = ethernet::Address::fromString(i->second.get_value<std::string>());
          if (mcastGroup.isNull())
            {
              throw ConfigFile::Error("Invalid value for option \"" +
                                      i->first + "\" in \"ether\" section");
            }
        }
      else
        {
          throw ConfigFile::Error("Unrecognized option \"" + i->first + "\" in \"ether\" section");
        }
    }

  if (!isDryRun)
    {
      shared_ptr<EthernetFactory> factory;
      if (m_factories.count("ether") > 0) {
        factory = static_pointer_cast<EthernetFactory>(m_factories["ether"]);
      }
      else {
        factory = make_shared<EthernetFactory>();
        m_factories.insert(std::make_pair("ether", factory));
      }

      if (useMcast)
        {
          std::list<shared_ptr<EthernetFace> > multicastFacesToRemove;
          for (EthernetFactory::MulticastFaceMap::const_iterator i =
                 factory->getMulticastFaces().begin();
               i != factory->getMulticastFaces().end();
               ++i)
            {
              multicastFacesToRemove.push_back(i->second);
            }

          for (const auto& nic : nicList)
            {
              if (nic.isUp() && nic.isMulticastCapable())
                {
                  try
                    {
                      shared_ptr<EthernetFace> newFace =
                        factory->createMulticastFace(nic, mcastGroup);

                      addCreatedFaceToForwarder(newFace);
                      multicastFacesToRemove.remove(newFace);
                    }
                  catch (const EthernetFactory::Error& factoryError)
                    {
                      NFD_LOG_ERROR(factoryError.what() << ", continuing");
                    }
                  catch (const EthernetFace::Error& faceError)
                    {
                      NFD_LOG_ERROR(faceError.what() << ", continuing");
                    }
                }
            }

          for (std::list<shared_ptr<EthernetFace> >::iterator i =
                 multicastFacesToRemove.begin();
               i != multicastFacesToRemove.end();
               ++i)
            {
              (*i)->close();
            }
        }
      else
        {
          std::list<shared_ptr<EthernetFace> > multicastFacesToRemove;
          for (EthernetFactory::MulticastFaceMap::const_iterator i =
                 factory->getMulticastFaces().begin();
               i != factory->getMulticastFaces().end();
               ++i)
            {
              multicastFacesToRemove.push_back(i->second);
            }

          for (std::list<shared_ptr<EthernetFace> >::iterator i =
                 multicastFacesToRemove.begin();
               i != multicastFacesToRemove.end();
               ++i)
            {
              (*i)->close();
            }
        }
    }
#else
  throw ConfigFile::Error("NFD was compiled without libpcap, cannot process \"ether\" section");
#endif // HAVE_LIBPCAP
}

void
FaceManager::processSectionWebSocket(const ConfigSection& configSection, bool isDryRun)
{
  // ; the websocket section contains settings of WebSocket faces and channels
  // websocket
  // {
  //   listen yes ; set to 'no' to disable WebSocket listener, default 'yes'
  //   port 9696 ; WebSocket listener port number
  //   enable_v4 yes ; set to 'no' to disable listening on IPv4 socket, default 'yes'
  //   enable_v6 yes ; set to 'no' to disable listening on IPv6 socket, default 'yes'
  // }

#if defined(HAVE_WEBSOCKET)

  std::string port = "9696";
  bool needToListen = true;
  bool enableV4 = true;
  bool enableV6 = true;

  for (ConfigSection::const_iterator i = configSection.begin();
       i != configSection.end();
       ++i)
    {
      if (i->first == "port")
        {
          port = i->second.get_value<std::string>();
          try
            {
              uint16_t portNo = boost::lexical_cast<uint16_t>(port);
              NFD_LOG_TRACE("WebSocket port set to " << portNo);
            }
          catch (const std::bad_cast& error)
            {
              throw ConfigFile::Error("Invalid value for option " +
                                      i->first + "\" in \"websocket\" section");
            }
        }
      else if (i->first == "listen")
        {
          needToListen = parseYesNo(i, i->first, "websocket");
        }
      else if (i->first == "enable_v4")
        {
          enableV4 = parseYesNo(i, i->first, "websocket");
        }
      else if (i->first == "enable_v6")
        {
          enableV6 = parseYesNo(i, i->first, "websocket");
        }
      else
        {
          throw ConfigFile::Error("Unrecognized option \"" +
                                  i->first + "\" in \"websocket\" section");
        }
    }

  if (!enableV4 && !enableV6)
    {
      throw ConfigFile::Error("IPv4 and IPv6 channels have been disabled."
                              " Remove \"websocket\" section to disable WebSocket channels or"
                              " re-enable at least one channel type.");
    }

  if (!enableV4 && enableV6)
    {
      throw ConfigFile::Error("NFD does not allow pure IPv6 WebSocket channel.");
    }

  if (!isDryRun)
    {
      if (m_factories.count("websocket") > 0)
        {
          return;
        }

      shared_ptr<WebSocketFactory> factory = make_shared<WebSocketFactory>(port);
      m_factories.insert(std::make_pair("websocket", factory));

      if (enableV6 && enableV4)
        {
          shared_ptr<WebSocketChannel> ip46Channel = factory->createChannel("::", port);
          if (needToListen)
            {
              ip46Channel->listen(bind(&FaceTable::add, &m_faceTable, _1));
            }

          m_factories.insert(std::make_pair("websocket46", factory));
        }
      else if (enableV4)
        {
          shared_ptr<WebSocketChannel> ipv4Channel = factory->createChannel("0.0.0.0", port);
          if (needToListen)
            {
              ipv4Channel->listen(bind(&FaceTable::add, &m_faceTable, _1));
            }

          m_factories.insert(std::make_pair("websocket4", factory));
        }
    }
#else
  throw ConfigFile::Error("NFD was compiled without WebSocket, "
                          "cannot process \"websocket\" section");
#endif // HAVE_WEBSOCKET
}


void
FaceManager::onFaceRequest(const Interest& request)
{
  const Name& command = request.getName();
  const size_t commandNComps = command.size();
  const Name::Component& verb = command.get(COMMAND_PREFIX.size());

  UnsignedVerbDispatchTable::const_iterator unsignedVerbProcessor =
    m_unsignedVerbDispatch.find(verb);
  if (unsignedVerbProcessor != m_unsignedVerbDispatch.end())
    {
      NFD_LOG_DEBUG("command result: processing verb: " << verb);
      (unsignedVerbProcessor->second)(this, request);
    }
  else if (COMMAND_UNSIGNED_NCOMPS <= commandNComps &&
           commandNComps < COMMAND_SIGNED_NCOMPS)
    {
      NFD_LOG_DEBUG("command result: unsigned verb: " << command);
      sendResponse(command, 401, "Signature required");
    }
  else if (commandNComps < COMMAND_SIGNED_NCOMPS ||
           !COMMAND_PREFIX.isPrefixOf(command))
    {
      NFD_LOG_DEBUG("command result: malformed");
      sendResponse(command, 400, "Malformed command");
    }
  else
    {
      validate(request,
               bind(&FaceManager::onValidatedFaceRequest, this, _1),
               bind(&ManagerBase::onCommandValidationFailed, this, _1, _2));
    }
}

void
FaceManager::onValidatedFaceRequest(const shared_ptr<const Interest>& request)
{
  const Name& command = request->getName();
  const Name::Component& verb = command[COMMAND_PREFIX.size()];
  const Name::Component& parameterComponent = command[COMMAND_PREFIX.size() + 1];

  SignedVerbDispatchTable::const_iterator signedVerbProcessor = m_signedVerbDispatch.find(verb);
  if (signedVerbProcessor != m_signedVerbDispatch.end())
    {
      ControlParameters parameters;
      if (!extractParameters(parameterComponent, parameters))
        {
          sendResponse(command, 400, "Malformed command");
          return;
        }

      NFD_LOG_DEBUG("command result: processing verb: " << verb);
      (signedVerbProcessor->second)(this, *request, parameters);
    }
  else
    {
      NFD_LOG_DEBUG("command result: unsupported verb: " << verb);
      sendResponse(command, 501, "Unsupported command");
    }

}

void
FaceManager::addCreatedFaceToForwarder(const shared_ptr<Face>& newFace)
{
  m_faceTable.add(newFace);

  //NFD_LOG_DEBUG("Created face " << newFace->getRemoteUri() << " ID " << newFace->getId());
}

void
FaceManager::onCreated(const Name& requestName,
                       ControlParameters& parameters,
                       const shared_ptr<Face>& newFace)
{
  addCreatedFaceToForwarder(newFace);
  parameters.setFaceId(newFace->getId());
  parameters.setUri(newFace->getRemoteUri().toString());

  sendResponse(requestName, 200, "Success", parameters.wireEncode());
}

void
FaceManager::onConnectFailed(const Name& requestName, const std::string& reason)
{
  NFD_LOG_DEBUG("Failed to create face: " << reason);
  sendResponse(requestName, 408, reason);
}

void
FaceManager::createFace(const Interest& request,
                        ControlParameters& parameters)
{
  const Name& requestName = request.getName();
  ndn::nfd::FaceCreateCommand command;

  if (!validateParameters(command, parameters))
    {
      sendResponse(requestName, 400, "Malformed command");
      NFD_LOG_TRACE("invalid control parameters URI");
      return;
    }

  FaceUri uri;
  if (!uri.parse(parameters.getUri()))
    {
      sendResponse(requestName, 400, "Malformed command");
      NFD_LOG_TRACE("failed to parse URI");
      return;
    }

  FactoryMap::iterator factory = m_factories.find(uri.getScheme());
  if (factory == m_factories.end())
    {
      sendResponse(requestName, 501, "Unsupported protocol");
      return;
    }

  try
    {
      factory->second->createFace(uri,
                                  bind(&FaceManager::onCreated,
                                       this, requestName, parameters, _1),
                                  bind(&FaceManager::onConnectFailed,
                                       this, requestName, _1));
    }
  catch (const std::runtime_error& error)
    {
      std::string errorMessage = "NFD error: ";
      errorMessage += error.what();

      NFD_LOG_ERROR(errorMessage);
      sendResponse(requestName, 500, errorMessage);
    }
  catch (const std::logic_error& error)
    {
      std::string errorMessage = "NFD error: ";
      errorMessage += error.what();

      NFD_LOG_ERROR(errorMessage);
      sendResponse(requestName, 500, errorMessage);
    }
}


void
FaceManager::destroyFace(const Interest& request,
                         ControlParameters& parameters)
{
  const Name& requestName = request.getName();
  ndn::nfd::FaceDestroyCommand command;

  if (!validateParameters(command, parameters))
    {
      sendResponse(requestName, 400, "Malformed command");
      return;
    }

  shared_ptr<Face> target = m_faceTable.get(parameters.getFaceId());
  if (static_cast<bool>(target))
    {
      target->close();
    }

  sendResponse(requestName, 200, "Success", parameters.wireEncode());

}

void
FaceManager::onAddFace(shared_ptr<Face> face)
{
  ndn::nfd::FaceEventNotification notification;
  notification.setKind(ndn::nfd::FACE_EVENT_CREATED);
  face->copyStatusTo(notification);

  m_notificationStream.postNotification(notification);
}

void
FaceManager::onRemoveFace(shared_ptr<Face> face)
{
  ndn::nfd::FaceEventNotification notification;
  notification.setKind(ndn::nfd::FACE_EVENT_DESTROYED);
  face->copyStatusTo(notification);

  m_notificationStream.postNotification(notification);
}

bool
FaceManager::extractLocalControlParameters(const Interest& request,
                                           ControlParameters& parameters,
                                           ControlCommand& command,
                                           shared_ptr<LocalFace>& outFace,
                                           LocalControlFeature& outFeature)
{
  if (!validateParameters(command, parameters))
    {
      sendResponse(request.getName(), 400, "Malformed command");
      return false;
    }

  shared_ptr<Face> face = m_faceTable.get(request.getIncomingFaceId());

  if (!static_cast<bool>(face))
    {
      NFD_LOG_DEBUG("command result: faceid " << request.getIncomingFaceId() << " not found");
      sendResponse(request.getName(), 410, "Face not found");
      return false;
    }
  else if (!face->isLocal())
    {
      NFD_LOG_DEBUG("command result: cannot enable local control on non-local faceid " <<
                    face->getId());
      sendResponse(request.getName(), 412, "Face is non-local");
      return false;
    }

  outFace = dynamic_pointer_cast<LocalFace>(face);
  outFeature = static_cast<LocalControlFeature>(parameters.getLocalControlFeature());

  return true;
}

void
FaceManager::enableLocalControl(const Interest& request,
                                ControlParameters& parameters)
{
  ndn::nfd::FaceEnableLocalControlCommand command;


  shared_ptr<LocalFace> face;
  LocalControlFeature feature;

  if (extractLocalControlParameters(request, parameters, command, face, feature))
    {
      face->setLocalControlHeaderFeature(feature, true);
      sendResponse(request.getName(), 200, "Success", parameters.wireEncode());
    }
}

void
FaceManager::disableLocalControl(const Interest& request,
                                 ControlParameters& parameters)
{
  ndn::nfd::FaceDisableLocalControlCommand command;
  shared_ptr<LocalFace> face;
  LocalControlFeature feature;

  if (extractLocalControlParameters(request, parameters, command, face, feature))
    {
      face->setLocalControlHeaderFeature(feature, false);
      sendResponse(request.getName(), 200, "Success", parameters.wireEncode());
    }
}

void
FaceManager::listFaces(const Interest& request)
{
  const Name& command = request.getName();
  const size_t commandNComps = command.size();

  if (commandNComps < FACES_LIST_DATASET_NCOMPS ||
      !FACES_LIST_DATASET_PREFIX.isPrefixOf(command))
    {
      NFD_LOG_DEBUG("command result: malformed");
      sendResponse(command, 400, "Malformed command");
      return;
    }

  m_faceStatusPublisher.publish();
}

void
FaceManager::listChannels(const Interest& request)
{
  NFD_LOG_DEBUG("in listChannels");
  const Name& command = request.getName();
  const size_t commandNComps = command.size();

  if (commandNComps < CHANNELS_LIST_DATASET_NCOMPS ||
      !CHANNELS_LIST_DATASET_PREFIX.isPrefixOf(command))
    {
      NFD_LOG_DEBUG("command result: malformed");
      sendResponse(command, 400, "Malformed command");
      return;
    }

  NFD_LOG_DEBUG("publishing");
  m_channelStatusPublisher.publish();
}

void
FaceManager::listQueriedFaces(const Interest& request)
{
  NFD_LOG_DEBUG("in listQueriedFaces");
  const Name& query = request.getName();
  const size_t queryNComps = query.size();

  if (queryNComps < FACES_QUERY_DATASET_NCOMPS ||
      !FACES_QUERY_DATASET_PREFIX.isPrefixOf(query))
    {
      NFD_LOG_DEBUG("query result: malformed");
      sendNack(query);
      return;
    }

  ndn::nfd::FaceQueryFilter faceFilter;
  try
    {
      faceFilter.wireDecode(query[-1].blockFromValue());
    }
  catch (tlv::Error&)
    {
      NFD_LOG_DEBUG("query result: malformed filter");
      sendNack(query);
      return;
    }

  FaceQueryStatusPublisher
    faceQueryStatusPublisher(m_faceTable, *m_face, query, faceFilter, m_keyChain);

  faceQueryStatusPublisher.publish();
}

shared_ptr<ProtocolFactory>
FaceManager::findFactory(const std::string& protocol)
{
  FactoryMap::iterator factory = m_factories.find(protocol);
  if (factory != m_factories.end())
    return factory->second;
  else
    return shared_ptr<ProtocolFactory>();
}

} // namespace nfd

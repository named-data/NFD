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

#ifndef NFD_DAEMON_FACE_ETHERNET_FACE_HPP
#define NFD_DAEMON_FACE_ETHERNET_FACE_HPP

#include "config.hpp"
#include "ethernet.hpp"
#include "face.hpp"

#ifndef HAVE_LIBPCAP
#error "Cannot include this file when libpcap is not available"
#endif

// forward declarations
struct pcap;
typedef pcap pcap_t;

namespace nfd {

class NetworkInterfaceInfo;

/**
 * \brief Implementation of Face abstraction that uses raw
 *        Ethernet frames as underlying transport mechanism
 */
class EthernetFace : public Face
{
public:
  /**
   * \brief EthernetFace-related error
   */
  struct Error : public Face::Error
  {
    Error(const std::string& what) : Face::Error(what) {}
  };

  EthernetFace(const shared_ptr<boost::asio::posix::stream_descriptor>& socket,
               const shared_ptr<NetworkInterfaceInfo>& interface,
               const ethernet::Address& address);

  virtual
  ~EthernetFace();

  /// send an Interest
  virtual void
  sendInterest(const Interest& interest);

  /// send a Data
  virtual void
  sendData(const Data& data);

  /**
   * \brief Close the face
   *
   * This terminates all communication on the face and cause
   * onFail() method event to be invoked
   */
  virtual void
  close();

private:
  void
  pcapInit();

  void
  setPacketFilter(const char* filterString);

  void
  sendPacket(const ndn::Block& block);

  void
  handleRead(const boost::system::error_code& error,
             size_t nBytesRead);

  void
  processErrorCode(const boost::system::error_code& error);

  size_t
  getInterfaceMtu() const;

private:
  shared_ptr<boost::asio::posix::stream_descriptor> m_socket;
  std::string m_interfaceName;
  ethernet::Address m_srcAddress;
  ethernet::Address m_destAddress;
  size_t m_interfaceMtu;
  pcap_t* m_pcap;
};

} // namespace nfd

#endif // NFD_DAEMON_FACE_ETHERNET_FACE_HPP

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_ETHERNET_FACE_HPP
#define NFD_FACE_ETHERNET_FACE_HPP

#include "ethernet.hpp"
#include "face.hpp"

#ifndef HAVE_PCAP
#error "Cannot include this file when pcap is not available"
#endif

// forward declarations
struct pcap;
typedef pcap pcap_t;

namespace nfd {

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
               const ethernet::Endpoint& interface,
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

  ethernet::Address
  getInterfaceAddress() const;

  size_t
  getInterfaceMtu() const;

private:
  shared_ptr<boost::asio::posix::stream_descriptor> m_socket;
  ethernet::Endpoint m_interface;
  ethernet::Address m_sourceAddress;
  ethernet::Address m_destAddress;
  size_t m_interfaceMtu;
  pcap_t* m_pcap;
};

} // namespace nfd

#endif // NFD_FACE_ETHERNET_FACE_HPP

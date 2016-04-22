/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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
 */

#ifndef NFD_DAEMON_FACE_DATAGRAM_TRANSPORT_HPP
#define NFD_DAEMON_FACE_DATAGRAM_TRANSPORT_HPP

#include "transport.hpp"
#include "core/global-io.hpp"

#include <array>

namespace nfd {
namespace face {

struct Unicast {};
struct Multicast {};

/** \brief Implements Transport for datagram-based protocols.
 *
 *  \tparam Protocol a datagram-based protocol in Boost.Asio
 */
template<class Protocol, class Addressing = Unicast>
class DatagramTransport : public Transport
{
public:
  typedef Protocol protocol;

  /** \brief Construct datagram transport.
   *
   *  \param socket Protocol-specific socket for the created transport
   */
  explicit
  DatagramTransport(typename protocol::socket&& socket);

  /** \brief Receive datagram, translate buffer into packet, deliver to parent class.
   */
  void
  receiveDatagram(const uint8_t* buffer, size_t nBytesReceived,
                  const boost::system::error_code& error);

protected:
  virtual void
  doClose() override;

  virtual void
  doSend(Transport::Packet&& packet) override;

  void
  handleSend(const boost::system::error_code& error,
             size_t nBytesSent, const Block& payload);

  void
  handleReceive(const boost::system::error_code& error,
                size_t nBytesReceived);

  void
  processErrorCode(const boost::system::error_code& error);

  bool
  hasBeenUsedRecently() const;

  void
  resetRecentUsage();

  static EndpointId
  makeEndpointId(const typename protocol::endpoint& ep);

protected:
  typename protocol::socket m_socket;
  typename protocol::endpoint m_sender;

  NFD_LOG_INCLASS_DECLARE();

private:
  std::array<uint8_t, ndn::MAX_NDN_PACKET_SIZE> m_receiveBuffer;
  bool m_hasBeenUsedRecently;
};


template<class T, class U>
DatagramTransport<T, U>::DatagramTransport(typename DatagramTransport::protocol::socket&& socket)
  : m_socket(std::move(socket))
  , m_hasBeenUsedRecently(false)
{
  m_socket.async_receive_from(boost::asio::buffer(m_receiveBuffer), m_sender,
                              bind(&DatagramTransport<T, U>::handleReceive, this,
                                   boost::asio::placeholders::error,
                                   boost::asio::placeholders::bytes_transferred));
}

template<class T, class U>
void
DatagramTransport<T, U>::doClose()
{
  NFD_LOG_FACE_TRACE(__func__);

  if (m_socket.is_open()) {
    // Cancel all outstanding operations and close the socket.
    // Use the non-throwing variants and ignore errors, if any.
    boost::system::error_code error;
    m_socket.cancel(error);
    m_socket.close(error);
  }

  // Ensure that the Transport stays alive at least until
  // all pending handlers are dispatched
  getGlobalIoService().post([this] {
    this->setState(TransportState::CLOSED);
  });
}

template<class T, class U>
void
DatagramTransport<T, U>::doSend(Transport::Packet&& packet)
{
  NFD_LOG_FACE_TRACE(__func__);

  m_socket.async_send(boost::asio::buffer(packet.packet),
                      bind(&DatagramTransport<T, U>::handleSend, this,
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred,
                           packet.packet));
}

template<class T, class U>
void
DatagramTransport<T, U>::receiveDatagram(const uint8_t* buffer, size_t nBytesReceived,
                                         const boost::system::error_code& error)
{
  if (error)
    return processErrorCode(error);

  NFD_LOG_FACE_TRACE("Received: " << nBytesReceived << " bytes");

  bool isOk = false;
  Block element;
  std::tie(isOk, element) = Block::fromBuffer(buffer, nBytesReceived);
  if (!isOk) {
    NFD_LOG_FACE_WARN("Failed to parse incoming packet");
    // This packet won't extend the face lifetime
    return;
  }
  if (element.size() != nBytesReceived) {
    NFD_LOG_FACE_WARN("Received datagram size and decoded element size don't match");
    // This packet won't extend the face lifetime
    return;
  }
  m_hasBeenUsedRecently = true;

  Transport::Packet tp(std::move(element));
  tp.remoteEndpoint = makeEndpointId(m_sender);
  this->receive(std::move(tp));
}

template<class T, class U>
void
DatagramTransport<T, U>::handleReceive(const boost::system::error_code& error,
                                       size_t nBytesReceived)
{
  receiveDatagram(m_receiveBuffer.data(), nBytesReceived, error);

  if (m_socket.is_open())
    m_socket.async_receive_from(boost::asio::buffer(m_receiveBuffer), m_sender,
                                bind(&DatagramTransport<T, U>::handleReceive, this,
                                     boost::asio::placeholders::error,
                                     boost::asio::placeholders::bytes_transferred));
}

template<class T, class U>
void
DatagramTransport<T, U>::handleSend(const boost::system::error_code& error,
                                    size_t nBytesSent, const Block& payload)
// 'payload' is unused; it's needed to retain the underlying Buffer
{
  if (error)
    return processErrorCode(error);

  NFD_LOG_FACE_TRACE("Successfully sent: " << nBytesSent << " bytes");
}

template<class T, class U>
void
DatagramTransport<T, U>::processErrorCode(const boost::system::error_code& error)
{
  NFD_LOG_FACE_TRACE(__func__);

  if (getState() == TransportState::CLOSING ||
      getState() == TransportState::FAILED ||
      getState() == TransportState::CLOSED ||
      error == boost::asio::error::operation_aborted)
    // transport is shutting down, ignore any errors
    return;

  if (getPersistency() == ndn::nfd::FacePersistency::FACE_PERSISTENCY_PERMANENT) {
    NFD_LOG_FACE_DEBUG("Permanent face ignores error: " << error.message());
    return;
  }

  if (error != boost::asio::error::eof)
    NFD_LOG_FACE_WARN("Send or receive operation failed: " << error.message());

  this->setState(TransportState::FAILED);
  doClose();
}

template<class T, class U>
inline bool
DatagramTransport<T, U>::hasBeenUsedRecently() const
{
  return m_hasBeenUsedRecently;
}

template<class T, class U>
inline void
DatagramTransport<T, U>::resetRecentUsage()
{
  m_hasBeenUsedRecently = false;
}

template<class T, class U>
inline Transport::EndpointId
DatagramTransport<T, U>::makeEndpointId(const typename protocol::endpoint& ep)
{
  return 0;
}

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_DATAGRAM_TRANSPORT_HPP

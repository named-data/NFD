/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FACE_STREAM_TRANSPORT_HPP
#define NFD_DAEMON_FACE_STREAM_TRANSPORT_HPP

#include "transport.hpp"
#include "socket-utils.hpp"
#include "core/global-io.hpp"

#include <queue>

namespace nfd {
namespace face {

/** \brief Implements Transport for stream-based protocols.
 *
 *  \tparam Protocol a stream-based protocol in Boost.Asio
 */
template<class Protocol>
class StreamTransport : public Transport
{
public:
  typedef Protocol protocol;

  /** \brief Construct stream transport.
   *
   *  \param socket Protocol-specific socket for the created transport
   */
  explicit
  StreamTransport(typename protocol::socket&& socket);

  ssize_t
  getSendQueueLength() override;

protected:
  void
  doClose() override;

  void
  deferredClose();

  void
  doSend(Transport::Packet&& packet) override;

  void
  sendFromQueue();

  void
  handleSend(const boost::system::error_code& error,
             size_t nBytesSent);

  void
  startReceive();

  void
  handleReceive(const boost::system::error_code& error,
                size_t nBytesReceived);

  void
  processErrorCode(const boost::system::error_code& error);

  virtual void
  handleError(const boost::system::error_code& error);

  void
  resetReceiveBuffer();

  void
  resetSendQueue();

  size_t
  getSendQueueBytes() const;

protected:
  typename protocol::socket m_socket;

  NFD_LOG_INCLASS_DECLARE();

private:
  uint8_t m_receiveBuffer[ndn::MAX_NDN_PACKET_SIZE];
  size_t m_receiveBufferSize;
  std::queue<Block> m_sendQueue;
  size_t m_sendQueueBytes;
};


template<class T>
StreamTransport<T>::StreamTransport(typename StreamTransport::protocol::socket&& socket)
  : m_socket(std::move(socket))
  , m_receiveBufferSize(0)
  , m_sendQueueBytes(0)
{
  // No queue capacity is set because there is no theoretical limit to the size of m_sendQueue.
  // Therefore, protecting against send queue overflows is less critical than in other transport
  // types. Instead, we use the default threshold specified in the GenericLinkService options.

  startReceive();
}

template<class T>
ssize_t
StreamTransport<T>::getSendQueueLength()
{
  ssize_t queueLength = getTxQueueLength(m_socket.native_handle());
  if (queueLength == QUEUE_ERROR) {
    NFD_LOG_FACE_WARN("Failed to obtain send queue length from socket: " << std::strerror(errno));
  }
  return getSendQueueBytes() + std::max<ssize_t>(0, queueLength);
}

template<class T>
void
StreamTransport<T>::doClose()
{
  NFD_LOG_FACE_TRACE(__func__);

  if (m_socket.is_open()) {
    // Cancel all outstanding operations and shutdown the socket
    // so that no further sends or receives are possible.
    // Use the non-throwing variants and ignore errors, if any.
    boost::system::error_code error;
    m_socket.cancel(error);
    m_socket.shutdown(protocol::socket::shutdown_both, error);
  }

  // Ensure that the Transport stays alive at least until
  // all pending handlers are dispatched
  getGlobalIoService().post([this] { deferredClose(); });

  // Some bug or feature of Boost.Asio (see https://redmine.named-data.net/issues/1856):
  //
  // When doClose is called from a socket event handler (e.g., from handleReceive),
  // m_socket.shutdown() does not trigger the cancellation of the handleSend callback.
  // Instead, handleSend is invoked as nothing bad happened.
  //
  // In order to prevent the assertion in handleSend from failing, we clear the queue
  // and close the socket in deferredClose, i.e., after all callbacks scheduled up to
  // this point have been executed.  If more send operations are scheduled after this
  // point, they will fail because the socket has been shutdown, and their callbacks
  // will be invoked with error code == asio::error::shut_down.
}

template<class T>
void
StreamTransport<T>::deferredClose()
{
  NFD_LOG_FACE_TRACE(__func__);

  resetSendQueue();

  // use the non-throwing variant and ignore errors, if any
  boost::system::error_code error;
  m_socket.close(error);

  this->setState(TransportState::CLOSED);
}

template<class T>
void
StreamTransport<T>::doSend(Transport::Packet&& packet)
{
  NFD_LOG_FACE_TRACE(__func__);

  if (getState() != TransportState::UP)
    return;

  bool wasQueueEmpty = m_sendQueue.empty();
  m_sendQueue.push(packet.packet);
  m_sendQueueBytes += packet.packet.size();

  if (wasQueueEmpty)
    sendFromQueue();
}

template<class T>
void
StreamTransport<T>::sendFromQueue()
{
  boost::asio::async_write(m_socket, boost::asio::buffer(m_sendQueue.front()),
                           bind(&StreamTransport<T>::handleSend, this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
}

template<class T>
void
StreamTransport<T>::handleSend(const boost::system::error_code& error,
                               size_t nBytesSent)
{
  if (error)
    return processErrorCode(error);

  NFD_LOG_FACE_TRACE("Successfully sent: " << nBytesSent << " bytes");

  BOOST_ASSERT(!m_sendQueue.empty());
  BOOST_ASSERT(m_sendQueue.front().size() == nBytesSent);
  m_sendQueueBytes -= nBytesSent;
  m_sendQueue.pop();

  if (!m_sendQueue.empty())
    sendFromQueue();
}

template<class T>
void
StreamTransport<T>::startReceive()
{
  BOOST_ASSERT(getState() == TransportState::UP);

  m_socket.async_receive(boost::asio::buffer(m_receiveBuffer + m_receiveBufferSize,
                                             ndn::MAX_NDN_PACKET_SIZE - m_receiveBufferSize),
                         bind(&StreamTransport<T>::handleReceive, this,
                              boost::asio::placeholders::error,
                              boost::asio::placeholders::bytes_transferred));
}

template<class T>
void
StreamTransport<T>::handleReceive(const boost::system::error_code& error,
                                  size_t nBytesReceived)
{
  if (error)
    return processErrorCode(error);

  NFD_LOG_FACE_TRACE("Received: " << nBytesReceived << " bytes");

  m_receiveBufferSize += nBytesReceived;
  size_t offset = 0;
  bool isOk = true;
  while (m_receiveBufferSize - offset > 0) {
    Block element;
    std::tie(isOk, element) = Block::fromBuffer(m_receiveBuffer + offset, m_receiveBufferSize - offset);
    if (!isOk)
      break;

    offset += element.size();
    BOOST_ASSERT(offset <= m_receiveBufferSize);

    this->receive(Transport::Packet(std::move(element)));
  }

  if (!isOk && m_receiveBufferSize == ndn::MAX_NDN_PACKET_SIZE && offset == 0) {
    NFD_LOG_FACE_ERROR("Failed to parse incoming packet or packet too large to process");
    this->setState(TransportState::FAILED);
    doClose();
    return;
  }

  if (offset > 0) {
    if (offset != m_receiveBufferSize) {
      std::copy(m_receiveBuffer + offset, m_receiveBuffer + m_receiveBufferSize, m_receiveBuffer);
      m_receiveBufferSize -= offset;
    }
    else {
      m_receiveBufferSize = 0;
    }
  }

  startReceive();
}

template<class T>
void
StreamTransport<T>::processErrorCode(const boost::system::error_code& error)
{
  NFD_LOG_FACE_TRACE(__func__);

  if (getState() == TransportState::CLOSING ||
      getState() == TransportState::FAILED ||
      getState() == TransportState::CLOSED ||
      error == boost::asio::error::operation_aborted || // when cancel() is called
      error == boost::asio::error::shut_down)           // after shutdown() is called
    // transport is shutting down, ignore any errors
    return;

  handleError(error);
}

template<class T>
void
StreamTransport<T>::handleError(const boost::system::error_code& error)
{
  if (error == boost::asio::error::eof) {
    this->setState(TransportState::CLOSING);
  }
  else {
    NFD_LOG_FACE_ERROR("Send or receive operation failed: " << error.message());
    this->setState(TransportState::FAILED);
  }
  doClose();
}

template<class T>
void
StreamTransport<T>::resetReceiveBuffer()
{
  m_receiveBufferSize = 0;
}

template<class T>
void
StreamTransport<T>::resetSendQueue()
{
  std::queue<Block> emptyQueue;
  std::swap(emptyQueue, m_sendQueue);
  m_sendQueueBytes = 0;
}

template<class T>
size_t
StreamTransport<T>::getSendQueueBytes() const
{
  return m_sendQueueBytes;
}

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_STREAM_TRANSPORT_HPP

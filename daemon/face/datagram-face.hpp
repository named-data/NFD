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
 */

#ifndef NFD_DAEMON_FACE_DATAGRAM_FACE_HPP
#define NFD_DAEMON_FACE_DATAGRAM_FACE_HPP

#include "face.hpp"
#include "core/global-io.hpp"

namespace nfd {

struct Unicast {};
struct Multicast {};

template<class Protocol, class Addressing = Unicast>
class DatagramFace : public Face
{
public:
  typedef Protocol protocol;

  /** \brief Construct datagram face
   *
   * \param socket      Protocol-specific socket for the created face
   */
  DatagramFace(const FaceUri& remoteUri, const FaceUri& localUri,
               typename protocol::socket socket);

  // from Face
  void
  sendInterest(const Interest& interest) DECL_OVERRIDE;

  void
  sendData(const Data& data) DECL_OVERRIDE;

  void
  close() DECL_OVERRIDE;

  void
  receiveDatagram(const uint8_t* buffer,
                  size_t nBytesReceived,
                  const boost::system::error_code& error);

protected:
  void
  processErrorCode(const boost::system::error_code& error);

  void
  handleSend(const boost::system::error_code& error,
             size_t nBytesSent,
             const Block& payload);

  void
  handleReceive(const boost::system::error_code& error,
                size_t nBytesReceived);

  void
  keepFaceAliveUntilAllHandlersExecuted(const shared_ptr<Face>& face);

  void
  closeSocket();

  bool
  hasBeenUsedRecently() const;

  /**
   * \brief Set m_hasBeenUsedRecently to false
   */
  void
  resetRecentUsage();

protected:
  typename protocol::socket m_socket;

  NFD_LOG_INCLASS_DECLARE();

private:
  uint8_t m_inputBuffer[ndn::MAX_NDN_PACKET_SIZE];
  bool m_hasBeenUsedRecently;
};


template<class T, class U>
inline
DatagramFace<T, U>::DatagramFace(const FaceUri& remoteUri, const FaceUri& localUri,
                                 typename DatagramFace::protocol::socket socket)
  : Face(remoteUri, localUri, false, std::is_same<U, Multicast>::value)
  , m_socket(std::move(socket))
{
  NFD_LOG_FACE_INFO("Creating face");

  m_socket.async_receive(boost::asio::buffer(m_inputBuffer, ndn::MAX_NDN_PACKET_SIZE),
                         bind(&DatagramFace<T, U>::handleReceive, this,
                              boost::asio::placeholders::error,
                              boost::asio::placeholders::bytes_transferred));
}

template<class T, class U>
inline void
DatagramFace<T, U>::sendInterest(const Interest& interest)
{
  NFD_LOG_FACE_TRACE(__func__);

  this->emitSignal(onSendInterest, interest);

  const Block& payload = interest.wireEncode();
  m_socket.async_send(boost::asio::buffer(payload.wire(), payload.size()),
                      bind(&DatagramFace<T, U>::handleSend, this,
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred,
                           payload));
}

template<class T, class U>
inline void
DatagramFace<T, U>::sendData(const Data& data)
{
  NFD_LOG_FACE_TRACE(__func__);

  this->emitSignal(onSendData, data);

  const Block& payload = data.wireEncode();
  m_socket.async_send(boost::asio::buffer(payload.wire(), payload.size()),
                      bind(&DatagramFace<T, U>::handleSend, this,
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred,
                           payload));
}

template<class T, class U>
inline void
DatagramFace<T, U>::close()
{
  if (!m_socket.is_open())
    return;

  NFD_LOG_FACE_INFO("Closing face");

  closeSocket();
  this->fail("Face closed");
}

template<class T, class U>
inline void
DatagramFace<T, U>::processErrorCode(const boost::system::error_code& error)
{
  if (error == boost::asio::error::operation_aborted) // when cancel() is called
    return;

  if (getPersistency() == ndn::nfd::FacePersistency::FACE_PERSISTENCY_PERMANENT) {
    NFD_LOG_FACE_DEBUG("Permanent face ignores error: " << error.message());
    return;
  }

  if (!m_socket.is_open()) {
    this->fail("Tunnel closed");
    return;
  }

  if (error != boost::asio::error::eof)
    NFD_LOG_FACE_WARN("Send or receive operation failed: " << error.message());

  closeSocket();

  if (error == boost::asio::error::eof)
    this->fail("Tunnel closed");
  else
    this->fail(error.message());
}

template<class T, class U>
inline void
DatagramFace<T, U>::handleSend(const boost::system::error_code& error,
                               size_t nBytesSent,
                               const Block& payload)
// 'payload' is unused; it's needed to retain the underlying Buffer
{
  if (error)
    return processErrorCode(error);

  NFD_LOG_FACE_TRACE("Successfully sent: " << nBytesSent << " bytes");
  this->getMutableCounters().getNOutBytes() += nBytesSent;
}

template<class T, class U>
inline void
DatagramFace<T, U>::handleReceive(const boost::system::error_code& error,
                                  size_t nBytesReceived)
{
  receiveDatagram(m_inputBuffer, nBytesReceived, error);

  if (m_socket.is_open())
    m_socket.async_receive(boost::asio::buffer(m_inputBuffer, ndn::MAX_NDN_PACKET_SIZE),
                           bind(&DatagramFace<T, U>::handleReceive, this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
}

template<class T, class U>
inline void
DatagramFace<T, U>::receiveDatagram(const uint8_t* buffer,
                                    size_t nBytesReceived,
                                    const boost::system::error_code& error)
{
  if (error)
    return processErrorCode(error);

  NFD_LOG_FACE_TRACE("Received: " << nBytesReceived << " bytes");
  this->getMutableCounters().getNInBytes() += nBytesReceived;

  bool isOk = false;
  Block element;
  std::tie(isOk, element) = Block::fromBuffer(buffer, nBytesReceived);
  if (!isOk)
    {
      NFD_LOG_FACE_WARN("Failed to parse incoming packet");
      // This message won't extend the face lifetime
      return;
    }

  if (element.size() != nBytesReceived)
    {
      NFD_LOG_FACE_WARN("Received datagram size and decoded element size don't match");
      // This message won't extend the face lifetime
      return;
    }

  if (!this->decodeAndDispatchInput(element))
    {
      NFD_LOG_FACE_WARN("Received unrecognized TLV block of type " << element.type());
      // This message won't extend the face lifetime
      return;
    }

  m_hasBeenUsedRecently = true;
}

template<class T, class U>
inline void
DatagramFace<T, U>::keepFaceAliveUntilAllHandlersExecuted(const shared_ptr<Face>& face)
{
  NFD_LOG_FACE_TRACE(__func__);
}

template<class T, class U>
inline void
DatagramFace<T, U>::closeSocket()
{
  NFD_LOG_FACE_TRACE(__func__);

  // use the non-throwing variants and ignore errors, if any
  boost::system::error_code error;
  m_socket.shutdown(protocol::socket::shutdown_both, error);
  m_socket.close(error);
  // after this, handlers will be called with an error code

  // ensure that the Face object is alive at least until all pending
  // handlers are dispatched
  getGlobalIoService().post(bind(&DatagramFace<T, U>::keepFaceAliveUntilAllHandlersExecuted,
                                 this, this->shared_from_this()));
}

template<class T, class U>
inline bool
DatagramFace<T, U>::hasBeenUsedRecently() const
{
  return m_hasBeenUsedRecently;
}

template<class T, class U>
inline void
DatagramFace<T, U>::resetRecentUsage()
{
  m_hasBeenUsedRecently = false;
}

} // namespace nfd

#endif // NFD_DAEMON_FACE_DATAGRAM_FACE_HPP

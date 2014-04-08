/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#ifndef NFD_FACE_DATAGRAM_FACE_HPP
#define NFD_FACE_DATAGRAM_FACE_HPP

#include "face.hpp"
#include "core/logger.hpp"

namespace nfd {

template <class Protocol>
class DatagramFace : public Face
{
public:
  typedef Protocol protocol;

  /** \brief Construct datagram face
   *
   * \param socket      Protocol-specific socket for the created face
   * \param isOnDemand  If true, the face can be closed after it remains
   *                    unused for a certain amount of time
   */
  DatagramFace(const FaceUri& remoteUri, const FaceUri& localUri,
               const shared_ptr<typename protocol::socket>& socket,
               bool isOnDemand);

  virtual
  ~DatagramFace();

  // from Face
  virtual void
  sendInterest(const Interest& interest);

  virtual void
  sendData(const Data& data);

  virtual void
  close();

  void
  handleSend(const boost::system::error_code& error,
             const Block& wire);

  void
  handleReceive(const boost::system::error_code& error,
                size_t nBytesReceived);

  /**
   * \brief Set m_hasBeenUsedRecently to false
   */
  void
  resetRecentUsage();

  bool
  hasBeenUsedRecently() const;

  void
  setOnDemand(bool isOnDemand);

protected:
  void
  receiveDatagram(const uint8_t* buffer,
                  size_t nBytesReceived,
                  const boost::system::error_code& error);

  void
  keepFaceAliveUntilAllHandlersExecuted(const shared_ptr<Face>& face);

  void
  closeSocket();

protected:
  shared_ptr<typename protocol::socket> m_socket;
  uint8_t m_inputBuffer[MAX_NDN_PACKET_SIZE];
  bool m_hasBeenUsedRecently;

  NFD_LOG_INCLASS_DECLARE();
};


template <class T>
inline
DatagramFace<T>::DatagramFace(const FaceUri& remoteUri, const FaceUri& localUri,
                              const shared_ptr<typename DatagramFace::protocol::socket>& socket,
                              bool isOnDemand)
  : Face(remoteUri, localUri)
  , m_socket(socket)
{
  setOnDemand(isOnDemand);

  m_socket->async_receive(boost::asio::buffer(m_inputBuffer, MAX_NDN_PACKET_SIZE), 0,
                          bind(&DatagramFace<T>::handleReceive, this, _1, _2));
}

template <class T>
inline
DatagramFace<T>::~DatagramFace()
{
}

template <class T>
inline void
DatagramFace<T>::sendInterest(const Interest& interest)
{
  this->onSendInterest(interest);
  m_socket->async_send(boost::asio::buffer(interest.wireEncode().wire(),
                                           interest.wireEncode().size()),
                       bind(&DatagramFace<T>::handleSend, this, _1, interest.wireEncode()));

  // anything else should be done here?
}

template <class T>
inline void
DatagramFace<T>::sendData(const Data& data)
{
  this->onSendData(data);
  m_socket->async_send(boost::asio::buffer(data.wireEncode().wire(),
                                           data.wireEncode().size()),
                       bind(&DatagramFace<T>::handleSend, this, _1, data.wireEncode()));

  // anything else should be done here?
}

template <class T>
inline void
DatagramFace<T>::handleSend(const boost::system::error_code& error,
                            const Block& wire)
{
  if (error != 0) {
    if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
      return;

    if (!m_socket->is_open())
    {
      onFail("Tunnel closed");
      return;
    }

    NFD_LOG_WARN("[id:" << this->getId()
                  << ",endpoint:" << m_socket->local_endpoint()
                  << "] Send operation failed, closing socket: "
                  << error.category().message(error.value()));

    closeSocket();

    if (error == boost::asio::error::eof)
    {
      onFail("Tunnel closed");
    }
    else
    {
      onFail("Send operation failed, closing socket: " +
             error.category().message(error.value()));
    }
    return;
  }

  NFD_LOG_TRACE("[id:" << this->getId()
                << ",endpoint:" << m_socket->local_endpoint()
                << "] Successfully sent: " << wire.size() << " bytes");
  // do nothing (needed to retain validity of wire memory block
}

template <class T>
inline void
DatagramFace<T>::close()
{
  if (!m_socket->is_open())
    return;

  NFD_LOG_INFO("[id:" << this->getId()
               << ",endpoint:" << m_socket->local_endpoint()
               << "] Close tunnel");

  closeSocket();
  onFail("Close tunnel");
}

template <class T>
inline void
DatagramFace<T>::handleReceive(const boost::system::error_code& error,
                               size_t nBytesReceived)
{
  NFD_LOG_DEBUG("handleReceive: " << nBytesReceived);
  receiveDatagram(m_inputBuffer, nBytesReceived, error);
  if (m_socket->is_open())
    m_socket->async_receive(boost::asio::buffer(m_inputBuffer, MAX_NDN_PACKET_SIZE), 0,
                            bind(&DatagramFace<T>::handleReceive, this, _1, _2));
}

template <class T>
inline void
DatagramFace<T>::receiveDatagram(const uint8_t* buffer,
                                 size_t nBytesReceived,
                                 const boost::system::error_code& error)
{
  if (error != 0 || nBytesReceived == 0) {
    if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
      return;

    // this should be unnecessary, but just in case
    if (!m_socket->is_open())
    {
      onFail("Tunnel closed");
      return;
    }

    NFD_LOG_WARN("[id:" << this->getId()
                 << ",endpoint:" << m_socket->local_endpoint()
                 << "] Receive operation failed: "
                 << error.category().message(error.value()));

    closeSocket();

    if (error == boost::asio::error::eof)
    {
      onFail("Tunnel closed");
    }
    else
    {
      onFail("Receive operation failed, closing socket: " +
             error.category().message(error.value()));
    }
    return;
  }

  NFD_LOG_TRACE("[id:" << this->getId()
                << ",endpoint:" << m_socket->local_endpoint()
                << "] Received: " << nBytesReceived << " bytes");

  Block element;
  bool isOk = Block::fromBuffer(buffer, nBytesReceived, element);
  if (!isOk)
    {
      NFD_LOG_WARN("[id:" << this->getId()
                   << ",endpoint:" << m_socket->local_endpoint()
                   << "] Failed to parse incoming packet");
      // This message won't extend the face lifetime
      return;
    }

  if (element.size() != nBytesReceived)
    {
      NFD_LOG_WARN("[id:" << this->getId()
                   << ",endpoint:" << m_socket->local_endpoint()
                   << "] Received datagram size and decoded "
                   << "element size don't match");
      // This message won't extend the face lifetime
      return;
    }

  if (!this->decodeAndDispatchInput(element))
    {
      NFD_LOG_WARN("[id:" << this->getId()
                   << ",endpoint:" << m_socket->local_endpoint()
                   << "] Received unrecognized block of type ["
                   << element.type() << "]");
      // This message won't extend the face lifetime
      return;
    }

  m_hasBeenUsedRecently = true;
}


template <class T>
inline void
DatagramFace<T>::keepFaceAliveUntilAllHandlersExecuted(const shared_ptr<Face>& face)
{
}

template <class T>
inline void
DatagramFace<T>::closeSocket()
{
  NFD_LOG_DEBUG("closeSocket  " << m_socket->local_endpoint());
  boost::asio::io_service& io = m_socket->get_io_service();

  // use the non-throwing variants and ignore errors, if any
  boost::system::error_code error;
  m_socket->shutdown(protocol::socket::shutdown_both, error);
  m_socket->close(error);
  // after this, handlers will be called with an error code

  // ensure that the Face object is alive at least until all pending
  // handlers are dispatched
  io.post(bind(&DatagramFace<T>::keepFaceAliveUntilAllHandlersExecuted,
               this, this->shared_from_this()));
}

template <class T>
inline void
DatagramFace<T>::setOnDemand(bool isOnDemand)
{
  Face::setOnDemand(isOnDemand);
}

template <class T>
inline void
DatagramFace<T>::resetRecentUsage()
{
  m_hasBeenUsedRecently = false;
}

template <class T>
inline bool
DatagramFace<T>::hasBeenUsedRecently() const
{
  return m_hasBeenUsedRecently;
}

} // namespace nfd

#endif // NFD_FACE_DATAGRAM_FACE_HPP

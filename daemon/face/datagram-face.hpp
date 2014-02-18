/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_DATAGRAM_FACE_HPP
#define NFD_FACE_DATAGRAM_FACE_HPP

#include "face.hpp"

namespace nfd {

template <class T>
class DatagramFace : public Face
{
public:
  typedef T protocol;
  
  explicit
  DatagramFace(const shared_ptr<typename protocol::socket>& socket);

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

  NFD_LOG_INCLASS_DECLARE();

};

template <class T>
inline
DatagramFace<T>::DatagramFace(const shared_ptr<typename DatagramFace::protocol::socket>& socket)
  : m_socket(socket)
{
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
  m_socket->async_send(boost::asio::buffer(interest.wireEncode().wire(),
                                           interest.wireEncode().size()),
                       bind(&DatagramFace<T>::handleSend, this, _1, interest.wireEncode()));
    
  // anything else should be done here?
}
  
template <class T>
inline void
DatagramFace<T>::sendData(const Data& data)
{
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
  receiveDatagram(m_inputBuffer, nBytesReceived, error);
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

  /// @todo Eliminate reliance on exceptions in this path
  try {
    Block element(buffer, nBytesReceived);

    if (element.size() != nBytesReceived)
      {
        NFD_LOG_WARN("[id:" << this->getId()
                     << ",endpoint:" << m_socket->local_endpoint()
                     << "] Received datagram size and decoded "
                     << "element size don't match");
        /// @todo this message should not extend the face lifetime
        return;
      }
    if (!this->decodeAndDispatchInput(element))
      {
        NFD_LOG_WARN("[id:" << this->getId()
                     << ",endpoint:" << m_socket->local_endpoint()
                     << "] Received unrecognized block of type ["
                     << element.type() << "]");
        // ignore unknown packet and proceed
        /// @todo this message should not extend the face lifetime
        return;
      }
  }
  catch(const tlv::Error& e) {
    NFD_LOG_WARN("[id:" << this->getId()
                 << ",endpoint:" << m_socket->local_endpoint()
                 << "] Received input is invalid");
    /// @todo this message should not extend the face lifetime
    return;
  }
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

} // namespace nfd

#endif // NFD_FACE_DATAGRAM_FACE_HPP

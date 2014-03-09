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
  
  DatagramFace(const FaceUri& uri,
               const shared_ptr<typename protocol::socket>& socket,
               bool isPermanent);

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

  void
  setPermanent(bool isPermanent);
  
  bool
  isPermanent() const;

  /**
   * \brief Set m_hasBeenUsedRecently to false
   */
  void
  resetRecentUsage();
  
  bool
  hasBeenUsedRecently() const;
  
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

  /**
   * If false, the face can be closed after it remains unused for a certain
   * amount of time
   */
  bool m_isPermanent;
  
  bool m_hasBeenUsedRecently;

  NFD_LOG_INCLASS_DECLARE();

};

template <class T>
inline
DatagramFace<T>::DatagramFace(const FaceUri& uri,
                              const shared_ptr<typename DatagramFace::protocol::socket>& socket,
                              bool isPermanent)
  : Face(uri)
  , m_socket(socket)
  , m_isPermanent(isPermanent)
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

  /// @todo Eliminate reliance on exceptions in this path
  try {
    Block element(buffer, nBytesReceived);

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
        // ignore unknown packet and proceed
        // This message won't extend the face lifetime
        return;
      }
  }
  catch(const tlv::Error& e) {
    NFD_LOG_WARN("[id:" << this->getId()
                 << ",endpoint:" << m_socket->local_endpoint()
                 << "] Received input is invalid");
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
DatagramFace<T>::setPermanent(bool isPermanent)
{
  m_isPermanent = isPermanent;
}

template <class T>
inline bool
DatagramFace<T>::isPermanent() const
{
  return m_isPermanent;
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

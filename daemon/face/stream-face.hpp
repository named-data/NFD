/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_STREAM_FACE_HPP
#define NFD_FACE_STREAM_FACE_HPP

#include "face.hpp"

namespace nfd {

template <class T>
class StreamFace : public Face
{
public:
  typedef T protocol;

  /**
   * \brief Create instance of StreamFace
   */
  StreamFace(const shared_ptr<typename protocol::socket>& socket);

  virtual
  ~StreamFace();

  // from Face
  virtual void
  sendInterest(const Interest& interest);

  virtual void
  sendData(const Data& data);

  virtual void
  close();
  
protected:
  void
  handleSend(const boost::system::error_code& error,
             const Block& wire);

  void
  handleReceive(const boost::system::error_code& error,
                std::size_t bytes_recvd);

  void
  keepFaceAliveUntilAllHandlersExecuted(const shared_ptr<Face>& face);
  
protected:
  shared_ptr<typename protocol::socket> m_socket;
  
private:
  uint8_t m_inputBuffer[MAX_NDN_PACKET_SIZE];
  std::size_t m_inputBufferSize;

#ifdef _DEBUG
  typename protocol::endpoint m_localEndpoint;
#endif
  
  NFD_LOG_INCLASS_DECLARE();
};

// All inherited classes must use
// NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(StreamFace, <specialization-parameter>, "Name");

template <class T>
inline
StreamFace<T>::StreamFace(const shared_ptr<typename StreamFace::protocol::socket>& socket)
  : m_socket(socket)
{
  m_socket->async_receive(boost::asio::buffer(m_inputBuffer, MAX_NDN_PACKET_SIZE), 0,
                          bind(&StreamFace<T>::handleReceive, this, _1, _2));
}

template <class T>
inline
StreamFace<T>::~StreamFace()
{
}


template <class T>
inline void
StreamFace<T>::sendInterest(const Interest& interest)
{
  m_socket->async_send(boost::asio::buffer(interest.wireEncode().wire(),
                                           interest.wireEncode().size()),
                       bind(&StreamFace<T>::handleSend, this, _1, interest.wireEncode()));
  
  // anything else should be done here?
}

template <class T>
inline void
StreamFace<T>::sendData(const Data& data)
{
  m_socket->async_send(boost::asio::buffer(data.wireEncode().wire(),
                                           data.wireEncode().size()),
                       bind(&StreamFace<T>::handleSend, this, _1, data.wireEncode()));
  
  // anything else should be done here?
}

template <class T>
inline void
StreamFace<T>::close()
{
  if (!m_socket->is_open())
    return;
  
  NFD_LOG_INFO("[id:" << this->getId()
               << ",endpoint:" << m_socket->local_endpoint()
               << "] Close connection");
  
  
  boost::asio::io_service& io = m_socket->get_io_service();
  m_socket->close();
  // after this, handleSend/handleReceive will be called with set error code

  // ensure that if that Face object is alive at least until all pending
  // methods are dispatched
  io.post(bind(&StreamFace<T>::keepFaceAliveUntilAllHandlersExecuted,
               this, this->shared_from_this()));

  onFail("Close connection");
}

template <class T>
inline void
StreamFace<T>::handleSend(const boost::system::error_code& error,
                          const Block& wire)
{
  if (error) {
    if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
      return;

    if (!m_socket->is_open())
      {
        onFail("Connection closed");
        return;
      }

    if (error == boost::asio::error::eof)
      {
        NFD_LOG_INFO("[id:" << this->getId()
                     << ",endpoint:" << m_socket->local_endpoint()
                     << "] Connection closed");
      }
    else
      {
        NFD_LOG_WARN("[id:" << this->getId()
                     << ",endpoint:" << m_socket->local_endpoint()
                     << "] Send operation failed, closing socket: "
                     << error.category().message(error.value()));
      }

    boost::asio::io_service& io = m_socket->get_io_service();
    m_socket->close();
    
    // ensure that if that Face object is alive at least until all pending
    // methods are dispatched
    io.post(bind(&StreamFace<T>::keepFaceAliveUntilAllHandlersExecuted,
                 this, this->shared_from_this()));
    
    if (error == boost::asio::error::eof)
      {
        onFail("Connection closed");
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
StreamFace<T>::handleReceive(const boost::system::error_code& error,
                             std::size_t bytes_recvd)
{
  if (error || bytes_recvd == 0) {
    if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
      return;

    // this should be unnecessary, but just in case
    if (!m_socket->is_open())
      {
        onFail("Connection closed");
        return;
      }

    if (error == boost::asio::error::eof)
      {
        NFD_LOG_INFO("[id:" << this->getId()
                     << ",endpoint:" << m_socket->local_endpoint()
                     << "] Connection closed");
      }
    else
      {
        NFD_LOG_WARN("[id:" << this->getId()
                     << ",endpoint:" << m_socket->local_endpoint()
                     << "] Receive operation failed: "
                     << error.category().message(error.value()));
      }
    
    boost::asio::io_service& io = m_socket->get_io_service();
    m_socket->close();
    
    // ensure that if that Face object is alive at least until all pending
    // methods are dispatched
    io.post(bind(&StreamFace<T>::keepFaceAliveUntilAllHandlersExecuted,
                 this, this->shared_from_this()));

    if (error == boost::asio::error::eof)
      {
        onFail("Connection closed");
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
                << "] Received: " << bytes_recvd << " bytes");

  m_inputBufferSize += bytes_recvd;
  // do magic

  std::size_t offset = 0;
  /// @todo Eliminate reliance on exceptions in this path
  try {
    while(m_inputBufferSize - offset > 0)
      {
        Block element(m_inputBuffer + offset, m_inputBufferSize - offset);
        offset += element.size();

        BOOST_ASSERT(offset <= m_inputBufferSize);
        
        /// @todo Ensure lazy field decoding process
        if (element.type() == tlv::Interest)
          {
            shared_ptr<Interest> i = make_shared<Interest>();
            i->wireDecode(element);
            onReceiveInterest(*i);
          }
        else if (element.type() == tlv::Data)
          {
            shared_ptr<Data> d = make_shared<Data>();
            d->wireDecode(element);
            onReceiveData(*d);
          }
        // @todo Add local header support
        // else if (element.type() == tlv::LocalHeader)
        //   {
        //     shared_ptr<Interest> i = make_shared<Interest>();
        //     i->wireDecode(element);
        //   }
        else
          {
            NFD_LOG_WARN("[id:" << this->getId()
                         << ",endpoint:" << m_socket->local_endpoint()
                         << "] Received unrecognized block of type ["
                         << element.type() << "]");
            // ignore unknown packet and proceed
          }
      }
  }
  catch(const tlv::Error& e) {
    if (m_inputBufferSize == MAX_NDN_PACKET_SIZE && offset == 0)
      {
        NFD_LOG_WARN("[id:" << this->getId()
                     << ",endpoint:" << m_socket->local_endpoint()
                     << "] Received input is invalid or too large to process, "
                     << "closing down the face");
        
        boost::asio::io_service& io = m_socket->get_io_service();
        m_socket->close();
    
        // ensure that if that Face object is alive at least until all pending
        // methods are dispatched
        io.post(bind(&StreamFace<T>::keepFaceAliveUntilAllHandlersExecuted,
                     this, this->shared_from_this()));
        
        onFail("Received input is invalid or too large to process, closing down the face");
        return;
      }
  }

  if (offset > 0)
    {
      if (offset != m_inputBufferSize)
        {
          std::copy(m_inputBuffer + offset, m_inputBuffer + m_inputBufferSize,
                    m_inputBuffer);
          m_inputBufferSize -= offset;
        }
      else
        {
          m_inputBufferSize = 0;
        }
    }

  m_socket->async_receive(boost::asio::buffer(m_inputBuffer + m_inputBufferSize,
                                              MAX_NDN_PACKET_SIZE - m_inputBufferSize), 0,
                          bind(&StreamFace<T>::handleReceive, this, _1, _2));
}

template <class T>
inline void
StreamFace<T>::keepFaceAliveUntilAllHandlersExecuted(const shared_ptr<Face>& face)
{
}


} // namespace nfd

#endif // NFD_FACE_STREAM_FACE_HPP

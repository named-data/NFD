/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_STREAM_FACE_HPP
#define NFD_FACE_STREAM_FACE_HPP

#include "face.hpp"
#include "local-face.hpp"
#include "core/logger.hpp"

namespace nfd {

// forward declaration
template<class T, class U, class V> struct StreamFaceSenderImpl;

template<class Protocol, class FaceBase = Face>
class StreamFace : public FaceBase
{
public:
  typedef Protocol protocol;

  /**
   * \brief Create instance of StreamFace
   */
  StreamFace(const FaceUri& remoteUri, const FaceUri& localUri,
             const shared_ptr<typename protocol::socket>& socket,
             bool isOnDemand);

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
  processErrorCode(const boost::system::error_code& error);

  void
  handleSend(const boost::system::error_code& error,
             const Block& header, const Block& payload);
  void
  handleSend(const boost::system::error_code& error,
             const Block& wire);

  void
  handleReceive(const boost::system::error_code& error,
                std::size_t bytes_recvd);

  void
  keepFaceAliveUntilAllHandlersExecuted(const shared_ptr<Face>& face);

  void
  closeSocket();

protected:
  shared_ptr<typename protocol::socket> m_socket;

private:
  uint8_t m_inputBuffer[MAX_NDN_PACKET_SIZE];
  std::size_t m_inputBufferSize;

  friend struct StreamFaceSenderImpl<Protocol, FaceBase, Interest>;
  friend struct StreamFaceSenderImpl<Protocol, FaceBase, Data>;

  NFD_LOG_INCLASS_DECLARE();
};

// All inherited classes must use
// NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(StreamFace, <specialization-parameter>, "Name");


/** \brief Class allowing validation of the StreamFace use
 *
 *  For example, partial specialization based on boost::asio::ip::tcp should check
 *  that local endpoint is loopback
 *
 *  @throws Face::Error if validation failed
 */
template<class Protocol, class U>
struct StreamFaceValidator
{
  static void
  validateSocket(typename Protocol::socket& socket)
  {
  }
};


template<class T, class FaceBase>
inline
StreamFace<T, FaceBase>::StreamFace(const FaceUri& remoteUri, const FaceUri& localUri,
                const shared_ptr<typename StreamFace::protocol::socket>& socket,
                bool isOnDemand)
  : FaceBase(remoteUri, localUri)
  , m_socket(socket)
  , m_inputBufferSize(0)
{
  FaceBase::setOnDemand(isOnDemand);
  StreamFaceValidator<T, FaceBase>::validateSocket(*socket);
  m_socket->async_receive(boost::asio::buffer(m_inputBuffer, MAX_NDN_PACKET_SIZE), 0,
                          bind(&StreamFace<T, FaceBase>::handleReceive, this, _1, _2));
}

template<class T, class U>
inline
StreamFace<T, U>::~StreamFace()
{
}

template<class Protocol, class FaceBase, class Packet>
struct StreamFaceSenderImpl
{
  static void
  send(StreamFace<Protocol, FaceBase>& face, const Packet& packet)
  {
    face.m_socket->async_send(boost::asio::buffer(packet.wireEncode().wire(),
                                                  packet.wireEncode().size()),
                              bind(&StreamFace<Protocol, FaceBase>::handleSend,
                                   &face, _1, packet.wireEncode()));
  }
};

// partial specialization (only classes can be partially specialized)
template<class Protocol, class Packet>
struct StreamFaceSenderImpl<Protocol, LocalFace, Packet>
{
  static void
  send(StreamFace<Protocol, LocalFace>& face, const Packet& packet)
  {
    using namespace boost::asio;

    if (face.isEmptyFilteredLocalControlHeader(packet.getLocalControlHeader()))
      {
        const Block& payload = packet.wireEncode();
        face.m_socket->async_send(buffer(payload.wire(), payload.size()),
                                  bind(&StreamFace<Protocol, LocalFace>::handleSend,
                                       &face, _1, packet.wireEncode()));
      }
    else
      {
        Block header = face.filterAndEncodeLocalControlHeader(packet);
        const Block& payload = packet.wireEncode();

        std::vector<const_buffer> buffers;
        buffers.reserve(2);
        buffers.push_back(buffer(header.wire(),  header.size()));
        buffers.push_back(buffer(payload.wire(), payload.size()));

        face.m_socket->async_send(buffers,
                                  bind(&StreamFace<Protocol, LocalFace>::handleSend,
                                       &face, _1, header, payload));
      }
  }
};


template<class T, class U>
inline void
StreamFace<T, U>::sendInterest(const Interest& interest)
{
  this->onSendInterest(interest);
  StreamFaceSenderImpl<T, U, Interest>::send(*this, interest);
}

template<class T, class U>
inline void
StreamFace<T, U>::sendData(const Data& data)
{
  this->onSendData(data);
  StreamFaceSenderImpl<T, U, Data>::send(*this, data);
}

template<class T, class U>
inline void
StreamFace<T, U>::close()
{
  if (!m_socket->is_open())
    return;

  NFD_LOG_INFO("[id:" << this->getId()
               << ",endpoint:" << m_socket->local_endpoint()
               << "] Close connection");

  closeSocket();
  this->onFail("Close connection");
}

template<class T, class U>
inline void
StreamFace<T, U>::processErrorCode(const boost::system::error_code& error)
{
  if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
    return;

  if (!m_socket->is_open())
    {
      this->onFail("Connection closed");
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
                   << "] Send or receive operation failed, closing socket: "
                   << error.category().message(error.value()));
    }

  closeSocket();

  if (error == boost::asio::error::eof)
    {
      this->onFail("Connection closed");
    }
  else
    {
      this->onFail("Send or receive operation failed, closing socket: " +
                   error.category().message(error.value()));
    }
}


template<class T, class U>
inline void
StreamFace<T, U>::handleSend(const boost::system::error_code& error,
                             const Block& wire)
{
  if (error)
    return processErrorCode(error);

  NFD_LOG_TRACE("[id:" << this->getId()
                << ",endpoint:" << m_socket->local_endpoint()
                << "] Successfully sent: " << wire.size() << " bytes");
}

template<class T, class U>
inline void
StreamFace<T, U>::handleSend(const boost::system::error_code& error,
                             const Block& header, const Block& payload)
{
  if (error)
    return processErrorCode(error);

  NFD_LOG_TRACE("[id:" << this->getId()
                << ",endpoint:" << m_socket->local_endpoint()
                << "] Successfully sent: " << (header.size()+payload.size()) << " bytes");
}

template<class T, class U>
inline void
StreamFace<T, U>::handleReceive(const boost::system::error_code& error,
                             std::size_t bytes_recvd)
{
  if (error)
    return processErrorCode(error);

  NFD_LOG_TRACE("[id:" << this->getId()
                << ",endpoint:" << m_socket->local_endpoint()
                << "] Received: " << bytes_recvd << " bytes");

  m_inputBufferSize += bytes_recvd;
  // do magic

  std::size_t offset = 0;

  bool isOk = true;
  Block element;
  while(m_inputBufferSize - offset > 0)
    {
      isOk = Block::fromBuffer(m_inputBuffer + offset, m_inputBufferSize - offset, element);
      if (!isOk)
        break;

      offset += element.size();

      BOOST_ASSERT(offset <= m_inputBufferSize);

      if (!this->decodeAndDispatchInput(element))
        {
          NFD_LOG_WARN("[id:" << this->getId()
                       << ",endpoint:" << m_socket->local_endpoint()
                       << "] Received unrecognized block of type ["
                       << element.type() << "]");
          // ignore unknown packet and proceed
        }
    }
  if (!isOk && m_inputBufferSize == MAX_NDN_PACKET_SIZE && offset == 0)
    {
      NFD_LOG_WARN("[id:" << this->getId()
                   << ",endpoint:" << m_socket->local_endpoint()
                   << "] Failed to parse incoming packet or it is too large to process, "
                   << "closing down the face");

      closeSocket();
      this->onFail("Failed to parse incoming packet or it is too large to process, "
                   "closing down the face");
      return;
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
                          bind(&StreamFace<T, U>::handleReceive, this, _1, _2));
}

template<class T, class U>
inline void
StreamFace<T, U>::keepFaceAliveUntilAllHandlersExecuted(const shared_ptr<Face>& face)
{
}

template<class T, class U>
inline void
StreamFace<T, U>::closeSocket()
{
  boost::asio::io_service& io = m_socket->get_io_service();

  // use the non-throwing variants and ignore errors, if any
  boost::system::error_code error;
  m_socket->shutdown(protocol::socket::shutdown_both, error);
  m_socket->close(error);
  // after this, handlers will be called with an error code

  // ensure that the Face object is alive at least until all pending
  // handlers are dispatched
  io.post(bind(&StreamFace<T, U>::keepFaceAliveUntilAllHandlersExecuted,
               this, this->shared_from_this()));
}

} // namespace nfd

#endif // NFD_FACE_STREAM_FACE_HPP

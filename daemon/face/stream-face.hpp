/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_STREAM_FACE_HPP
#define NFD_FACE_STREAM_FACE_HPP

#include "face.hpp"

namespace ndn {

template <class T>
class StreamFace : public Face
{
public:
  typedef T protocol;

  StreamFace(FaceId id,
             const shared_ptr<typename protocol::socket>& socket);

protected:
  void
  handleSend(const boost::system::error_code& error,
             const Block& wire);

  void
  handleReceive(const boost::system::error_code& error,
                std::size_t bytes_recvd);

protected:
  shared_ptr<typename protocol::socket> m_socket;

private:
  uint8_t m_inputBuffer[MAX_NDN_PACKET_SIZE];
  std::size_t m_inputBufferSize;
};

template <class T>
inline
StreamFace<T>::StreamFace(FaceId id,
                          const shared_ptr<typename StreamFace::protocol::socket>& socket)
  : Face(id)
  , m_socket(socket)
{
  m_socket->async_receive(boost::asio::buffer(m_inputBuffer, MAX_NDN_PACKET_SIZE), 0,
                          bind(&StreamFace<T>::handleReceive, this, _1, _2));
}


template <class T>
inline void
StreamFace<T>::handleSend(const boost::system::error_code& error,
                          const Block& wire)
{
  if (error) {
    if (error == boost::system::errc::operation_canceled) // when socket is closed by someone
      return;

    onFail("Send operation failed: " + error.category().message(error.value()));
    m_socket->close();
    return;
  }

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

    onFail("Receive operation failed: " + error.category().message(error.value()));
    m_socket->close();
    return;
  }

  m_inputBufferSize += bytes_recvd;
  // do magic

  std::size_t offset = 0;
  /// @todo Eliminate reliance on exceptions in this path
  try {
    Block element(m_inputBuffer + offset, m_inputBufferSize - offset);
    offset += element.size();

    /// @todo Ensure lazy field decoding process
    if (element.type() == Tlv::Interest)
      {
        shared_ptr<Interest> i = make_shared<Interest>();
        i->wireDecode(element);
        onReceiveInterest(*i);
      }
    else if (element.type() == Tlv::Data)
      {
        shared_ptr<Data> d = make_shared<Data>();
        d->wireDecode(element);
        onReceiveData(*d);
      }
    // @todo Add local header support
    // else if (element.type() == Tlv::LocalHeader)
    //   {
    //     shared_ptr<Interest> i = make_shared<Interest>();
    //     i->wireDecode(element);
    //   }
    else
      {
        /// @todo Add loggin

        // ignore unknown packet and proceed
      }
  }
  catch(const Tlv::Error&) {
    if (m_inputBufferSize == MAX_NDN_PACKET_SIZE && offset == 0)
      {
        onFail("Received input is invalid or too large to process, closing down the face");
        m_socket->close();
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


} // namespace ndn

#endif // NFD_FACE_STREAM_FACE_HPP

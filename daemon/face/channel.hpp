/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_CHANNEL_HPP
#define NFD_FACE_CHANNEL_HPP

#include "face.hpp"

namespace nfd {

class Channel : noncopyable
{
public:
  /** \brief Prototype for the callback called when face is created
   *         (as a response to incoming connection or after connection
   *         is established)
   */
  typedef function<void(const shared_ptr<Face>& newFace)> FaceCreatedCallback;

  /** \brief Prototype for the callback that is called when face is failed to
   *         get created
   */
  typedef function<void(const std::string& reason)> ConnectFailedCallback;

  virtual
  ~Channel();

  const FaceUri&
  getUri() const;

protected:
  void
  setUri(const FaceUri& uri);

private:
  FaceUri m_uri;
};

inline const FaceUri&
Channel::getUri() const
{
  return m_uri;
}

} // namespace nfd

#endif // NFD_FACE_CHANNEL_HPP

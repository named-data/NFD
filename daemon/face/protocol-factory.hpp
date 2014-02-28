/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_PROTOCOL_FACTORY_HPP
#define NFD_FACE_PROTOCOL_FACTORY_HPP

#include "common.hpp"
#include "core/face-uri.hpp"

namespace nfd {

class Face;

/**
 * \brief Prototype for the callback called when face is created
 *        (as a response to incoming connection or after connection
 *        is established)
 */
typedef function<void(const shared_ptr<Face>& newFace)> FaceCreatedCallback;

/**
 * \brief Prototype for the callback that is called when face is failed to
 *        get created
 */
typedef function<void(const std::string& reason)> FaceConnectFailedCallback;


class ProtocolFactory
{
public:
  /**
   * \brief Base class for all exceptions thrown by channel factories
   */
  struct Error : public std::runtime_error
  {
    Error(const std::string& what) : std::runtime_error(what) {}
  };

  /** \brief Try to create Face using the supplied Face URI
   *
   * This method should automatically choose channel, based on supplied Face URI
   * and create face.
   *
   * \throws Factory::Error if Factory does not support connect operation
   */
  virtual void
  createFace(const FaceUri& uri,
             const FaceCreatedCallback& onCreated,
             const FaceConnectFailedCallback& onConnectFailed) = 0;
};

} // namespace nfd

#endif // NFD_FACE_PROTOCOL_FACTORY_HPP

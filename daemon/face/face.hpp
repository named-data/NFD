/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_FACE_H
#define NFD_FACE_FACE_H

#include "common.hpp"
#include "core/event-emitter.hpp"

namespace nfd {

/** \class FaceId
 *  \brief identifies a face
 */
typedef int FaceId;

const FaceId INVALID_FACEID = -1;

const std::size_t MAX_NDN_PACKET_SIZE = 8800;

/** \class Face
 *  \brief represents a face
 */
class Face : noncopyable, public enable_shared_from_this<Face>
{
public:
  Face();
  
  virtual
  ~Face();
  
  FaceId
  getId() const;

  /// fires when an Interest is received
  EventEmitter<const Interest&> onReceiveInterest;
  
  /// fires when a Data is received
  EventEmitter<const Data&> onReceiveData;

  /// fires when face disconnects or fails to perform properly
  EventEmitter<const std::string& /*reason*/> onFail;
  
  /// send an Interest
  virtual void
  sendInterest(const Interest& interest) = 0;
  
  /// send a Data
  virtual void
  sendData(const Data& data) = 0;
  
  /** \brief Get whether underlying communicate is up
   *  In this base class this property is always true.
   */
  virtual bool
  isUp() const;
  
  /** \brief Set the description
   *  This is typically invoked by mgmt on set description command
   */
  virtual void
  setDescription(const std::string& description);
  
  /// Get the description
  virtual const std::string&
  getDescription() const;
  
  /** \brief Get whether packets sent this Face may reach multiple peers
   *  In this base class this property is always false.
   */
  virtual bool
  isMultiAccess() const;
  
  /// Get whether the face has opted in for local control header
  virtual bool
  isLocalControlHeaderEnabled() const;

protected:
  // void
  // receiveInterest();

  // void
  // receiveData();
  
private:
  FaceId
  setId(FaceId faceId);

private:
  FaceId m_id;
  std::string m_description;
  
  // allow setting FaceId
  friend class Forwarder;
};

} // namespace nfd

#endif // NFD_FACE_FACE_H

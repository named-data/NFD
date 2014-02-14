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
  /**
   * \brief Face-related error
   */
  struct Error : public std::runtime_error
  {
    Error(const std::string& what) : std::runtime_error(what) {}
  };

  Face();
  
  virtual
  ~Face();
  
  FaceId
  getId() const;

  /// fires when an Interest is received
  EventEmitter<Interest> onReceiveInterest;
  
  /// fires when a Data is received
  EventEmitter<Data> onReceiveData;

  /// fires when face disconnects or fails to perform properly
  EventEmitter<std::string/*reason*/> onFail;
  
  /// send an Interest
  virtual void
  sendInterest(const Interest& interest) = 0;
  
  /// send a Data
  virtual void
  sendData(const Data& data) = 0;

  /**
   * \brief Close the face
   *
   * This terminates all communication on the face and cause
   * onFail() method event to be invoked
   */
  virtual void
  close() = 0;
  
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
  void
  setId(FaceId faceId);

private:
  FaceId m_id;
  std::string m_description;
  
  // allow setting FaceId
  friend class Forwarder;
};

} // namespace nfd

#endif // NFD_FACE_FACE_H

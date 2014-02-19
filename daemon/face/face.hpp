/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_FACE_HPP
#define NFD_FACE_FACE_HPP

#include "common.hpp"
#include "core/event-emitter.hpp"

namespace nfd {

/** \class FaceId
 *  \brief identifies a face
 */
typedef int FaceId;

const FaceId INVALID_FACEID = -1;

const std::size_t MAX_NDN_PACKET_SIZE = 8800;


/* \brief indicates a feature in LocalControlHeader
 */
enum LocalControlHeaderFeature
{
  /// any feature
  LOCAL_CONTROL_HEADER_FEATURE_ANY,
  /// in-faceid
  LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID,
  /// out-faceid
  LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID,
  /// upper bound of enum
  LOCAL_CONTROL_HEADER_FEATURE_MAX
};


/** \brief represents a face
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

  /** \brief Get whether face is connected to a local app
   *
   *  In this base class this property is always false.
   */
  virtual bool
  isLocal() const = 0;
  
  /** \brief Get whether underlying communication is up
   *
   *  In this base class this property is always true.
   */
  virtual bool
  isUp() const;

  /** \brief Set the description
   *
   *  This is typically invoked by mgmt on set description command
   */
  virtual void
  setDescription(const std::string& description);

  /// Get the description
  virtual const std::string&
  getDescription() const;

  /** \brief Get whether packets sent this Face may reach multiple peers
   *
   *  In this base class this property is always false.
   */
  virtual bool
  isMultiAccess() const;

  /** \brief get whether a LocalControlHeader feature is enabled
   *
   *  \param feature The feature. Cannot be LOCAL_CONTROL_HEADER_FEATURE_MAX
   *  LOCAL_CONTROL_HEADER_FEATURE_ANY returns true if any feature is enabled.
   */
  bool
  isLocalControlHeaderEnabled(LocalControlHeaderFeature feature =
                              LOCAL_CONTROL_HEADER_FEATURE_ANY) const;

  /** \brief enable or disable a LocalControlHeader feature
   *
   *  \param feature The feature. Cannot be LOCAL_CONTROL_HEADER_FEATURE_ANY
   *                                     or LOCAL_CONTROL_HEADER_FEATURE_MAX
   */
  void
  setLocalControlHeaderFeature(LocalControlHeaderFeature feature, bool enabled);

private:
  void
  setId(FaceId faceId);

private:
  FaceId m_id;
  std::string m_description;
  std::vector<bool> m_localControlHeaderFeatures;

  // allow setting FaceId
  friend class Forwarder;
};

inline bool
Face::isLocalControlHeaderEnabled(LocalControlHeaderFeature feature) const
{
  BOOST_ASSERT(feature < m_localControlHeaderFeatures.size());
  return m_localControlHeaderFeatures[feature];
}

} // namespace nfd

#endif // NFD_FACE_FACE_HPP

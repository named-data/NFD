/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_PIT_FACE_RECORD_HPP
#define NFD_TABLE_PIT_FACE_RECORD_HPP

#include "face/face.hpp"
#include "core/time.hpp"

namespace nfd {
namespace pit {

/** \class FaceRecord
 *  \brief contains information about an Interest
 *         on an incoming or outgoing face
 *  \note This is an implementation detail to extract common functionality
 *        of InRecord and OutRecord
 */
class FaceRecord
{
public:
  explicit
  FaceRecord(shared_ptr<Face> face);
  
  FaceRecord(const FaceRecord& other);
  
  shared_ptr<Face>
  getFace() const;
  
  uint32_t
  getLastNonce() const;
  
  time::Point
  getLastRenewed() const;
  
  /** \brief gives the time point this record expires
   *  \return{ getLastRenewed() + InterestLifetime }
   */
  time::Point
  getExpiry() const;

  /// updates lastNonce, lastRenewed, expiry fields
  void
  update(const Interest& interest);

private:
  shared_ptr<Face> m_face;
  uint32_t m_lastNonce;
  time::Point m_lastRenewed;
  time::Point m_expiry;
};

inline shared_ptr<Face>
FaceRecord::getFace() const
{
  return m_face;
}

inline uint32_t
FaceRecord::getLastNonce() const
{
  return m_lastNonce;
}

inline time::Point
FaceRecord::getLastRenewed() const
{
  return m_lastRenewed;
}

inline time::Point
FaceRecord::getExpiry() const
{
  return m_expiry;
}

} // namespace pit
} // namespace nfd

#endif // NFD_TABLE_PIT_FACE_RECORD_HPP

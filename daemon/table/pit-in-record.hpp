/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_PIT_IN_RECORD_HPP
#define NFD_TABLE_PIT_IN_RECORD_HPP

#include "pit-face-record.hpp"

namespace nfd {
namespace pit {

/** \class InRecord
 *  \brief contains information about an Interest from an incoming face
 */
class InRecord : public FaceRecord
{
public:
  explicit
  InRecord(shared_ptr<Face> face);

  InRecord(const InRecord& other);

  void
  update(const Interest& interest);

  const Interest&
  getInterest() const;

private:
  shared_ptr<Interest> m_interest;
};

inline const Interest&
InRecord::getInterest() const
{
  BOOST_ASSERT(static_cast<bool>(m_interest));
  return *m_interest;
}

} // namespace pit
} // namespace nfd

#endif // NFD_TABLE_PIT_IN_RECORD_HPP

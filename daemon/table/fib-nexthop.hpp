/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_FIB_NEXTHOP_HPP
#define NFD_TABLE_FIB_NEXTHOP_HPP

#include "common.hpp"
#include "face/face.hpp"

namespace nfd {
namespace fib {

/** \class NextHop
 *  \brief represents a nexthop record in FIB entry
 */
class NextHop
{
public:
  explicit
  NextHop(shared_ptr<Face> face);

  NextHop(const NextHop& other);

  shared_ptr<Face>
  getFace() const;

  void
  setCost(uint32_t cost);

  uint32_t
  getCost() const;

private:
  shared_ptr<Face> m_face;
  uint32_t m_cost;
};

} // namespace fib
} // namespace nfd

#endif // NFD_TABLE_FIB_NEXTHOP_HPP

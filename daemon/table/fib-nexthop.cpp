/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fib-nexthop.hpp"

namespace nfd {
namespace fib {

NextHop::NextHop(shared_ptr<Face> face)
  : m_face(face), m_cost(0)
{
}

NextHop::NextHop(const NextHop& other)
  : m_face(other.m_face), m_cost(other.m_cost)
{
}

shared_ptr<Face>
NextHop::getFace() const
{
  return m_face;
}

void
NextHop::setCost(uint32_t cost)
{
  m_cost = cost;
}

uint32_t
NextHop::getCost() const
{
  return m_cost;
}

} // namespace fib
} // namespace nfd

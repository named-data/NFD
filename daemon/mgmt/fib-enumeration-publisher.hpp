/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_FIB_ENUMERATION_PUBLISHER_HPP
#define NFD_MGMT_FIB_ENUMERATION_PUBLISHER_HPP

#include "table/fib.hpp"
#include "mgmt/segment-publisher.hpp"

namespace nfd {

class FibEnumerationPublisher : public SegmentPublisher
{
public:
  FibEnumerationPublisher(const Fib& fib,
                          shared_ptr<AppFace> face,
                          const Name& prefix);

  virtual
  ~FibEnumerationPublisher();

protected:

  virtual size_t
  generate(ndn::EncodingBuffer& outBuffer);

private:
  const Fib& m_fib;
};

} // namespace nfd

#endif // NFD_MGMT_FIB_ENUMERATION_PUBLISHER_HPP

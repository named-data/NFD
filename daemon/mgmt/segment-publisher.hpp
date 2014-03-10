/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_SEGMENT_PUBLISHER_HPP
#define NFD_MGMT_SEGMENT_PUBLISHER_HPP

#include "common.hpp"
#include "mgmt/app-face.hpp"

#include <ndn-cpp-dev/encoding/encoding-buffer.hpp>

namespace nfd {

class AppFace;

class SegmentPublisher : noncopyable
{
public:
  SegmentPublisher(shared_ptr<AppFace> face,
                   const Name& prefix);

  virtual
  ~SegmentPublisher();

  void
  publish();

protected:

  virtual size_t
  generate(ndn::EncodingBuffer& outBuffer) =0;

private:
  void
  publishSegment(shared_ptr<Data>& data);

private:
  shared_ptr<AppFace> m_face;
  const Name m_prefix;
};

} // namespace nfd

#endif // NFD_MGMT_SEGMENT_PUBLISHER_HPP

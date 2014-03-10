/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_FACE_STATUS_PUBLISHER_HPP
#define NFD_MGMT_FACE_STATUS_PUBLISHER_HPP

#include "fw/face-table.hpp"
#include "mgmt/segment-publisher.hpp"

namespace nfd {

class FaceStatusPublisher : public SegmentPublisher
{
public:
  FaceStatusPublisher(const FaceTable& faceTable,
                      shared_ptr<AppFace> face,
                      const Name& prefix);

  virtual
  ~FaceStatusPublisher();

protected:

  virtual size_t
  generate(ndn::EncodingBuffer& outBuffer);

private:
  const FaceTable& m_faceTable;
};

} // namespace nfd

#endif // NFD_MGMT_FACE_STATUS_PUBLISHER_HPP

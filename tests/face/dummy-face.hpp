/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TEST_FACE_DUMMY_FACE_HPP
#define NFD_TEST_FACE_DUMMY_FACE_HPP

#include "face/face.hpp"
#include "face/local-face.hpp"

namespace nfd {
namespace tests {

/** \class DummyFace
 *  \brief a Face for unit testing
 */
template<class FaceBase>
class DummyFaceImpl : public FaceBase
{
public:
  DummyFaceImpl()
    : FaceBase(FaceUri("dummy://"), FaceUri("dummy://"))
  {
  }

  virtual void
  sendInterest(const Interest& interest)
  {
    this->onSendInterest(interest);
    m_sentInterests.push_back(interest);
    this->afterSend();
  }

  virtual void
  sendData(const Data& data)
  {
    this->onSendData(data);
    m_sentDatas.push_back(data);
    this->afterSend();
  }

  virtual void
  close()
  {
    this->onFail("close");
  }

  void
  receiveInterest(const Interest& interest)
  {
    this->onReceiveInterest(interest);
  }

  void
  receiveData(const Data& data)
  {
    this->onReceiveData(data);
  }

  EventEmitter<> afterSend;

public:
  std::vector<Interest> m_sentInterests;
  std::vector<Data> m_sentDatas;
};

typedef DummyFaceImpl<Face> DummyFace;
typedef DummyFaceImpl<LocalFace> DummyLocalFace;

} // namespace tests
} // namespace nfd

#endif // TEST_FACE_DUMMY_FACE_HPP

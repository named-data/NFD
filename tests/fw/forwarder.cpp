/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fw/forwarder.hpp"
#include "../face/dummy-face.hpp"

#include <boost/test/unit_test.hpp>

namespace nfd {

class ForwarderTestFace : public Face {
public:
  ForwarderTestFace(boost::asio::io_service& ioService)
    : m_ioService(ioService)
  {
  }
  
  virtual void
  sendInterest(const Interest& interest)
  {
    m_sentInterests.push_back(interest);
    m_ioService.stop();
  }
  
  virtual void
  sendData(const Data& data)
  {
    m_sentDatas.push_back(data);
    m_ioService.stop();
  }

  void
  receiveInterest(const Interest& interest)
  {
    onReceiveInterest(interest);
  }
  
  void
  receiveData(const Data& data)
  {
    onReceiveData(data);
  }

public:
  std::vector<Interest> m_sentInterests;
  std::vector<Data> m_sentDatas;

private:
  boost::asio::io_service& m_ioService;
};

BOOST_AUTO_TEST_SUITE(FwForwarder)

BOOST_AUTO_TEST_CASE(AddRemoveFace)
{
  boost::asio::io_service io;
  Forwarder forwarder(io);
  
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  
  BOOST_CHECK_EQUAL(face1->getId(), INVALID_FACEID);
  BOOST_CHECK_EQUAL(face2->getId(), INVALID_FACEID);
  
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  BOOST_CHECK_NE(face1->getId(), INVALID_FACEID);
  BOOST_CHECK_NE(face2->getId(), INVALID_FACEID);
  BOOST_CHECK_NE(face1->getId(), face2->getId());

  forwarder.removeFace(face1);
  forwarder.removeFace(face2);

  BOOST_CHECK_EQUAL(face1->getId(), INVALID_FACEID);
  BOOST_CHECK_EQUAL(face2->getId(), INVALID_FACEID);
}

BOOST_AUTO_TEST_CASE(SimpleExchange)
{
  Name nameA  ("ndn:/A");
  Name nameAB ("ndn:/A/B");
  Name nameABC("ndn:/A/B/C");
  Interest interestAB(nameAB);
  interestAB.setInterestLifetime(4000);
  Data dataABC(nameABC);
  
  boost::asio::io_service io;
  Forwarder forwarder(io);

  shared_ptr<ForwarderTestFace> face1 = make_shared<ForwarderTestFace>(boost::ref(io));
  shared_ptr<ForwarderTestFace> face2 = make_shared<ForwarderTestFace>(boost::ref(io));
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  
  Fib& fib = forwarder.getFib();
  std::pair<shared_ptr<fib::Entry>, bool> fibInsertResult =
    fib.insert(Name("ndn:/A"));
  shared_ptr<fib::Entry> fibEntry = fibInsertResult.first;
  fibEntry->addNextHop(face2, 0);
  
  face1->receiveInterest(interestAB);
  io.run();
  io.reset();
  BOOST_REQUIRE_EQUAL(face2->m_sentInterests.size(), 1);
  BOOST_CHECK(face2->m_sentInterests[0].getName().equals(nameAB));
  
  face2->receiveData(dataABC);
  io.run();
  io.reset();
  BOOST_REQUIRE_EQUAL(face1->m_sentDatas.size(), 1);
  BOOST_CHECK(face1->m_sentDatas[0].getName().equals(nameABC));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd

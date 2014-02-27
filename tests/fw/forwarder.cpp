/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fw/forwarder.hpp"
#include "../face/dummy-face.hpp"

#include <boost/test/unit_test.hpp>

namespace nfd {

BOOST_AUTO_TEST_SUITE(FwForwarder)

BOOST_AUTO_TEST_CASE(AddRemoveFace)
{
  resetGlobalIoService();
  Forwarder forwarder;

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

class ForwarderTestFace : public DummyFace {
public:
  virtual void
  afterSend()
  {
    getGlobalIoService().stop();
  }
};

BOOST_AUTO_TEST_CASE(SimpleExchange)
{
  resetGlobalIoService();
  Forwarder forwarder;

  Name nameA  ("ndn:/A");
  Name nameAB ("ndn:/A/B");
  Name nameABC("ndn:/A/B/C");
  Interest interestAB(nameAB);
  interestAB.setInterestLifetime(4000);
  Data dataABC(nameABC);

  shared_ptr<ForwarderTestFace> face1 = make_shared<ForwarderTestFace>();
  shared_ptr<ForwarderTestFace> face2 = make_shared<ForwarderTestFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  Fib& fib = forwarder.getFib();
  std::pair<shared_ptr<fib::Entry>, bool> fibInsertResult =
    fib.insert(Name("ndn:/A"));
  shared_ptr<fib::Entry> fibEntry = fibInsertResult.first;
  fibEntry->addNextHop(face2, 0);

  face1->receiveInterest(interestAB);
  getGlobalIoService().run();
  getGlobalIoService().reset();
  BOOST_REQUIRE_EQUAL(face2->m_sentInterests.size(), 1);
  BOOST_CHECK(face2->m_sentInterests[0].getName().equals(nameAB));
  BOOST_CHECK_EQUAL(face2->m_sentInterests[0].getIncomingFaceId(), face1->getId());

  face2->receiveData(dataABC);
  getGlobalIoService().run();
  getGlobalIoService().reset();
  BOOST_REQUIRE_EQUAL(face1->m_sentDatas.size(), 1);
  BOOST_CHECK(face1->m_sentDatas[0].getName().equals(nameABC));
  BOOST_CHECK_EQUAL(face1->m_sentDatas[0].getIncomingFaceId(), face2->getId());
}

class ScopeLocalhostIncomingTestForwarder : public Forwarder
{
public:
  ScopeLocalhostIncomingTestForwarder()
  {
  }

  virtual void
  onDataUnsolicited(Face& inFace, const Data& data)
  {
    ++m_onDataUnsolicited_count;
  }

protected:
  virtual void
  dispatchToStrategy(const Face& inFace,
                     const Interest& interest,
                     shared_ptr<fib::Entry> fibEntry,
                     shared_ptr<pit::Entry> pitEntry)
  {
    ++m_dispatchToStrategy_count;
  }

public:
  int m_dispatchToStrategy_count;
  int m_onDataUnsolicited_count;
};

BOOST_AUTO_TEST_CASE(ScopeLocalhostIncoming)
{
  resetGlobalIoService();
  ScopeLocalhostIncomingTestForwarder forwarder;
  shared_ptr<Face> face1 = make_shared<DummyLocalFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  // local face, /localhost: OK
  forwarder.m_dispatchToStrategy_count = 0;
  forwarder.onIncomingInterest(*face1, Interest("/localhost/A1"));
  BOOST_CHECK_EQUAL(forwarder.m_dispatchToStrategy_count, 1);

  // non-local face, /localhost: violate
  forwarder.m_dispatchToStrategy_count = 0;
  forwarder.onIncomingInterest(*face2, Interest("/localhost/A2"));
  BOOST_CHECK_EQUAL(forwarder.m_dispatchToStrategy_count, 0);

  // local face, non-/localhost: OK
  forwarder.m_dispatchToStrategy_count = 0;
  forwarder.onIncomingInterest(*face1, Interest("/A3"));
  BOOST_CHECK_EQUAL(forwarder.m_dispatchToStrategy_count, 1);

  // non-local face, non-/localhost: OK
  forwarder.m_dispatchToStrategy_count = 0;
  forwarder.onIncomingInterest(*face2, Interest("/A4"));
  BOOST_CHECK_EQUAL(forwarder.m_dispatchToStrategy_count, 1);

  // local face, /localhost: OK
  forwarder.m_onDataUnsolicited_count = 0;
  forwarder.onIncomingData(*face1, Data("/localhost/B1"));
  BOOST_CHECK_EQUAL(forwarder.m_onDataUnsolicited_count, 1);

  // non-local face, /localhost: OK
  forwarder.m_onDataUnsolicited_count = 0;
  forwarder.onIncomingData(*face2, Data("/localhost/B2"));
  BOOST_CHECK_EQUAL(forwarder.m_onDataUnsolicited_count, 0);

  // local face, non-/localhost: OK
  forwarder.m_onDataUnsolicited_count = 0;
  forwarder.onIncomingData(*face1, Data("/B3"));
  BOOST_CHECK_EQUAL(forwarder.m_onDataUnsolicited_count, 1);

  // non-local face, non-/localhost: OK
  forwarder.m_onDataUnsolicited_count = 0;
  forwarder.onIncomingData(*face2, Data("/B4"));
  BOOST_CHECK_EQUAL(forwarder.m_onDataUnsolicited_count, 1);
}

BOOST_AUTO_TEST_CASE(ScopeLocalhostOutgoing)
{
  resetGlobalIoService();
  Forwarder forwarder;
  shared_ptr<DummyLocalFace> face1 = make_shared<DummyLocalFace>();
  shared_ptr<DummyFace>      face2 = make_shared<DummyFace>();
  shared_ptr<Face>           face3 = make_shared<DummyLocalFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  forwarder.addFace(face3);
  Pit& pit = forwarder.getPit();

  // local face, /localhost: OK
  Interest interestA1 = Interest("/localhost/A1");
  shared_ptr<pit::Entry> pitA1 = pit.insert(interestA1).first;
  pitA1->insertOrUpdateInRecord(face3, interestA1);
  face1->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pitA1, *face1);
  BOOST_CHECK_EQUAL(face1->m_sentInterests.size(), 1);

  // non-local face, /localhost: violate
  Interest interestA2 = Interest("/localhost/A2");
  shared_ptr<pit::Entry> pitA2 = pit.insert(interestA2).first;
  pitA2->insertOrUpdateInRecord(face3, interestA1);
  face2->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pitA2, *face2);
  BOOST_CHECK_EQUAL(face2->m_sentInterests.size(), 0);

  // local face, non-/localhost: OK
  Interest interestA3 = Interest("/A3");
  shared_ptr<pit::Entry> pitA3 = pit.insert(interestA3).first;
  pitA3->insertOrUpdateInRecord(face3, interestA3);
  face1->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pitA3, *face1);
  BOOST_CHECK_EQUAL(face1->m_sentInterests.size(), 1);

  // non-local face, non-/localhost: OK
  Interest interestA4 = Interest("/A4");
  shared_ptr<pit::Entry> pitA4 = pit.insert(interestA4).first;
  pitA4->insertOrUpdateInRecord(face3, interestA4);
  face2->m_sentInterests.clear();
  forwarder.onOutgoingInterest(pitA4, *face2);
  BOOST_CHECK_EQUAL(face2->m_sentInterests.size(), 1);

  // local face, /localhost: OK
  face1->m_sentDatas.clear();
  forwarder.onOutgoingData(Data("/localhost/B1"), *face1);
  BOOST_CHECK_EQUAL(face1->m_sentDatas.size(), 1);

  // non-local face, /localhost: OK
  face2->m_sentDatas.clear();
  forwarder.onOutgoingData(Data("/localhost/B2"), *face2);
  BOOST_CHECK_EQUAL(face2->m_sentDatas.size(), 0);

  // local face, non-/localhost: OK
  face1->m_sentDatas.clear();
  forwarder.onOutgoingData(Data("/B3"), *face1);
  BOOST_CHECK_EQUAL(face1->m_sentDatas.size(), 1);

  // non-local face, non-/localhost: OK
  face2->m_sentDatas.clear();
  forwarder.onOutgoingData(Data("/B4"), *face2);
  BOOST_CHECK_EQUAL(face2->m_sentDatas.size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd

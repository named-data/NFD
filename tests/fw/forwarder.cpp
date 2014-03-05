/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fw/forwarder.hpp"
#include "tests/face/dummy-face.hpp"
#include <ndn-cpp-dev/security/key-chain.hpp>

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FwForwarder, BaseFixture)

BOOST_AUTO_TEST_CASE(SimpleExchange)
{
  Forwarder forwarder;

  Name nameA  ("ndn:/A");
  Name nameAB ("ndn:/A/B");
  Name nameABC("ndn:/A/B/C");
  Interest interestAB(nameAB);
  interestAB.setInterestLifetime(4000);
  shared_ptr<Data> dataABC = make_shared<Data>(nameABC);
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  dataABC->setSignature(fakeSignature);

  shared_ptr<DummyFace> face1 = make_shared<DummyFace>();
  shared_ptr<DummyFace> face2 = make_shared<DummyFace>();
  face1->afterSend += bind(&boost::asio::io_service::stop, &g_io);
  face2->afterSend += bind(&boost::asio::io_service::stop, &g_io);
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  Fib& fib = forwarder.getFib();
  std::pair<shared_ptr<fib::Entry>, bool> fibInsertResult =
    fib.insert(Name("ndn:/A"));
  shared_ptr<fib::Entry> fibEntry = fibInsertResult.first;
  fibEntry->addNextHop(face2, 0);

  face1->receiveInterest(interestAB);
  g_io.run();
  g_io.reset();
  BOOST_REQUIRE_EQUAL(face2->m_sentInterests.size(), 1);
  BOOST_CHECK(face2->m_sentInterests[0].getName().equals(nameAB));
  BOOST_CHECK_EQUAL(face2->m_sentInterests[0].getIncomingFaceId(), face1->getId());

  face2->receiveData(*dataABC);
  g_io.run();
  g_io.reset();
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
  ScopeLocalhostIncomingTestForwarder forwarder;
  shared_ptr<Face> face1 = make_shared<DummyLocalFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  shared_ptr<Data> d1, d2, d3, d4;
  shared_ptr<Interest> i1, i2, i3, i4;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));

  // local face, /localhost: OK
  forwarder.m_dispatchToStrategy_count = 0;
  i1 = make_shared<Interest>(Name("/localhost/A1"));
  forwarder.onIncomingInterest(*face1, *i1);
  BOOST_CHECK_EQUAL(forwarder.m_dispatchToStrategy_count, 1);

  // non-local face, /localhost: violate
  forwarder.m_dispatchToStrategy_count = 0;
  i2 = make_shared<Interest>(Name("/localhost/A2"));
  forwarder.onIncomingInterest(*face2, *i2);
  BOOST_CHECK_EQUAL(forwarder.m_dispatchToStrategy_count, 0);

  // local face, non-/localhost: OK
  forwarder.m_dispatchToStrategy_count = 0;
  i3 = make_shared<Interest>(Name("/A3"));
  forwarder.onIncomingInterest(*face1, *i3);
  BOOST_CHECK_EQUAL(forwarder.m_dispatchToStrategy_count, 1);

  // non-local face, non-/localhost: OK
  forwarder.m_dispatchToStrategy_count = 0;
  i4 = make_shared<Interest>(Name("/A4"));
  forwarder.onIncomingInterest(*face2, *i4);
  BOOST_CHECK_EQUAL(forwarder.m_dispatchToStrategy_count, 1);

  // local face, /localhost: OK
  forwarder.m_onDataUnsolicited_count = 0;
  d1 = make_shared<Data>(Name("/localhost/B1"));
  d1->setSignature(fakeSignature);
  forwarder.onIncomingData(*face1, *d1);
  BOOST_CHECK_EQUAL(forwarder.m_onDataUnsolicited_count, 1);

  // non-local face, /localhost: OK
  forwarder.m_onDataUnsolicited_count = 0;
  d2 = make_shared<Data>(Name("/localhost/B2"));
  d2->setSignature(fakeSignature);
  forwarder.onIncomingData(*face2, *d2);
  BOOST_CHECK_EQUAL(forwarder.m_onDataUnsolicited_count, 0);

  // local face, non-/localhost: OK
  forwarder.m_onDataUnsolicited_count = 0;
  d3 = make_shared<Data>(Name("/B3"));
  d3->setSignature(fakeSignature);
  forwarder.onIncomingData(*face1, *d3);
  BOOST_CHECK_EQUAL(forwarder.m_onDataUnsolicited_count, 1);

  // non-local face, non-/localhost: OK
  forwarder.m_onDataUnsolicited_count = 0;
  d4 = make_shared<Data>(Name("/B4"));
  d4->setSignature(fakeSignature);
  forwarder.onIncomingData(*face2, *d4);
  BOOST_CHECK_EQUAL(forwarder.m_onDataUnsolicited_count, 1);
}

BOOST_AUTO_TEST_CASE(ScopeLocalhostOutgoing)
{
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

} // namespace tests
} // namespace nfd

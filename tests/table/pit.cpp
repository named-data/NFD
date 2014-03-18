/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "table/pit.hpp"
#include "tests/face/dummy-face.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TablePit, BaseFixture)

BOOST_AUTO_TEST_CASE(EntryInOutRecords)
{
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  Name name("ndn:/KuYfjtRq");
  shared_ptr<Interest> interest  = makeInterest(name);
  shared_ptr<Interest> interest1 = makeInterest(name);
  interest1->setInterestLifetime(time::milliseconds(2528));
  interest1->setNonce(25559);
  shared_ptr<Interest> interest2 = makeInterest(name);
  interest2->setInterestLifetime(time::milliseconds(6464));
  interest2->setNonce(19004);
  shared_ptr<Interest> interest3 = makeInterest(name);
  interest3->setInterestLifetime(time::milliseconds(3585));
  interest3->setNonce(24216);
  shared_ptr<Interest> interest4 = makeInterest(name);
  interest4->setInterestLifetime(time::milliseconds(8795));
  interest4->setNonce(17365);

  pit::Entry entry(*interest);

  BOOST_CHECK_EQUAL(entry.getInterest().getName(), name);
  BOOST_CHECK_EQUAL(entry.getName(), name);

  const pit::InRecordCollection& inRecords1 = entry.getInRecords();
  BOOST_CHECK_EQUAL(inRecords1.size(), 0);
  const pit::OutRecordCollection& outRecords1 = entry.getOutRecords();
  BOOST_CHECK_EQUAL(outRecords1.size(), 0);

  // insert InRecord
  time::steady_clock::TimePoint before1 = time::steady_clock::now();
  pit::InRecordCollection::iterator in1 =
    entry.insertOrUpdateInRecord(face1, *interest1);
  time::steady_clock::TimePoint after1 = time::steady_clock::now();
  const pit::InRecordCollection& inRecords2 = entry.getInRecords();
  BOOST_CHECK_EQUAL(inRecords2.size(), 1);
  BOOST_CHECK(in1 == inRecords2.begin());
  BOOST_CHECK_EQUAL(in1->getFace(), face1);
  BOOST_CHECK_EQUAL(in1->getLastNonce(), interest1->getNonce());
  BOOST_CHECK_GE(in1->getLastRenewed(), before1);
  BOOST_CHECK_LE(in1->getLastRenewed(), after1);
  BOOST_CHECK_LE(in1->getExpiry() - in1->getLastRenewed()
                 - interest1->getInterestLifetime(),
                 (after1 - before1));

  // insert OutRecord
  time::steady_clock::TimePoint before2 = time::steady_clock::now();
  pit::OutRecordCollection::iterator out1 =
    entry.insertOrUpdateOutRecord(face1, *interest1);
  time::steady_clock::TimePoint after2 = time::steady_clock::now();
  const pit::OutRecordCollection& outRecords2 = entry.getOutRecords();
  BOOST_CHECK_EQUAL(outRecords2.size(), 1);
  BOOST_CHECK(out1 == outRecords2.begin());
  BOOST_CHECK_EQUAL(out1->getFace(), face1);
  BOOST_CHECK_EQUAL(out1->getLastNonce(), interest1->getNonce());
  BOOST_CHECK_GE(out1->getLastRenewed(), before2);
  BOOST_CHECK_LE(out1->getLastRenewed(), after2);
  BOOST_CHECK_LE(out1->getExpiry() - out1->getLastRenewed()
                 - interest1->getInterestLifetime(),
                 (after2 - before2));

  // update InRecord
  time::steady_clock::TimePoint before3 = time::steady_clock::now();
  pit::InRecordCollection::iterator in2 =
    entry.insertOrUpdateInRecord(face1, *interest2);
  time::steady_clock::TimePoint after3 = time::steady_clock::now();
  const pit::InRecordCollection& inRecords3 = entry.getInRecords();
  BOOST_CHECK_EQUAL(inRecords3.size(), 1);
  BOOST_CHECK(in2 == inRecords3.begin());
  BOOST_CHECK_EQUAL(in2->getFace(), face1);
  BOOST_CHECK_EQUAL(in2->getLastNonce(), interest2->getNonce());
  BOOST_CHECK_LE(in2->getExpiry() - in2->getLastRenewed()
                 - interest2->getInterestLifetime(),
                 (after3 - before3));

  // insert another InRecord
  pit::InRecordCollection::iterator in3 =
    entry.insertOrUpdateInRecord(face2, *interest3);
  const pit::InRecordCollection& inRecords4 = entry.getInRecords();
  BOOST_CHECK_EQUAL(inRecords4.size(), 2);
  BOOST_CHECK_EQUAL(in3->getFace(), face2);

  // delete all InRecords
  entry.deleteInRecords();
  const pit::InRecordCollection& inRecords5 = entry.getInRecords();
  BOOST_CHECK_EQUAL(inRecords5.size(), 0);

  // insert another OutRecord
  pit::OutRecordCollection::iterator out2 =
    entry.insertOrUpdateOutRecord(face2, *interest4);
  const pit::OutRecordCollection& outRecords3 = entry.getOutRecords();
  BOOST_CHECK_EQUAL(outRecords3.size(), 2);
  BOOST_CHECK_EQUAL(out2->getFace(), face2);

  // delete OutRecord
  entry.deleteOutRecord(face2);
  const pit::OutRecordCollection& outRecords4 = entry.getOutRecords();
  BOOST_REQUIRE_EQUAL(outRecords4.size(), 1);
  BOOST_CHECK_EQUAL(outRecords4.begin()->getFace(), face1);
}

BOOST_AUTO_TEST_CASE(EntryNonce)
{
  shared_ptr<Interest> interest = makeInterest("ndn:/qtCQ7I1c");

  pit::Entry entry(*interest);

  BOOST_CHECK_EQUAL(entry.addNonce(25559), true);
  BOOST_CHECK_EQUAL(entry.addNonce(25559), false);
  BOOST_CHECK_EQUAL(entry.addNonce(19004), true);
  BOOST_CHECK_EQUAL(entry.addNonce(19004), false);
}

BOOST_AUTO_TEST_CASE(EntryLifetime)
{
  shared_ptr<Interest> interest = makeInterest("ndn:/7oIEurbgy6");
  BOOST_ASSERT(interest->getInterestLifetime() < time::milliseconds::zero()); // library uses -1 to indicate unset lifetime

  shared_ptr<Face> face = make_shared<DummyFace>();
  pit::Entry entry(*interest);

  pit::InRecordCollection::iterator inIt = entry.insertOrUpdateInRecord(face, *interest);
  BOOST_CHECK_GT(inIt->getExpiry(), time::steady_clock::now());

  pit::OutRecordCollection::iterator outIt = entry.insertOrUpdateOutRecord(face, *interest);
  BOOST_CHECK_GT(outIt->getExpiry(), time::steady_clock::now());
}

BOOST_AUTO_TEST_CASE(EntryCanForwardTo)
{
  shared_ptr<Interest> interest = makeInterest("ndn:/WDsuBLIMG");
  pit::Entry entry(*interest);

  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();

  entry.insertOrUpdateInRecord(face1, *interest);
  BOOST_CHECK_EQUAL(entry.canForwardTo(*face1), false);
  BOOST_CHECK_EQUAL(entry.canForwardTo(*face2), true);

  entry.insertOrUpdateInRecord(face2, *interest);
  BOOST_CHECK_EQUAL(entry.canForwardTo(*face1), true);
  BOOST_CHECK_EQUAL(entry.canForwardTo(*face2), true);

  entry.insertOrUpdateOutRecord(face1, *interest);
  BOOST_CHECK_EQUAL(entry.canForwardTo(*face1), false);
  BOOST_CHECK_EQUAL(entry.canForwardTo(*face2), true);
}

BOOST_AUTO_TEST_CASE(Insert)
{
  Name name1("ndn:/5vzBNnMst");
  Name name2("ndn:/igSGfEIM62");
  Exclude exclude0;
  Exclude exclude1;
  exclude1.excludeOne(Name::Component("u26p47oep"));
  Exclude exclude2;
  exclude2.excludeOne(Name::Component("FG1Ni6nYcf"));

  // base
  Interest interestA(name1, -1, -1, exclude0, -1, false, -1, time::milliseconds::min(), 0);
  // A+exclude1
  Interest interestB(name1, -1, -1, exclude1, -1, false, -1, time::milliseconds::min(), 0);
  // A+exclude2
  Interest interestC(name1, -1, -1, exclude2, -1, false, -1, time::milliseconds::min(), 0);
  // A+MinSuffixComponents
  Interest interestD(name1, 2, -1, exclude0, -1, false, -1, time::milliseconds::min(), 0);
  // A+MaxSuffixComponents
  Interest interestE(name1, -1,  4, exclude0, -1, false, -1, time::milliseconds::min(), 0);
  // A+ChildSelector
  Interest interestF(name1, -1, -1, exclude0,  1, false, -1, time::milliseconds::min(), 0);
  // A+MustBeFresh
  Interest interestG(name1, -1, -1, exclude0, -1,  true, -1, time::milliseconds::min(), 0);
  // A+Scope
  Interest interestH(name1, -1, -1, exclude0, -1, false,  2, time::milliseconds::min(), 0);
  // A+InterestLifetime
  Interest interestI(name1, -1, -1, exclude0, -1, false, -1, time::milliseconds(2000), 0);
  // A+Nonce
  Interest interestJ(name1, -1, -1, exclude0, -1, false, -1, time::milliseconds::min(), 2192);
  // different Name+exclude1
  Interest interestK(name2, -1, -1, exclude1, -1, false, -1, time::milliseconds::min(), 0);

  NameTree nameTree(16);
  Pit pit(nameTree);

  std::pair<shared_ptr<pit::Entry>, bool> insertResult;

  BOOST_CHECK_EQUAL(pit.size(), 0);

  insertResult = pit.insert(interestA);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 1);

  insertResult = pit.insert(interestB);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 2);

  insertResult = pit.insert(interestC);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 3);

  insertResult = pit.insert(interestD);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 4);

  insertResult = pit.insert(interestE);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 5);

  insertResult = pit.insert(interestF);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 6);

  insertResult = pit.insert(interestG);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 7);


  insertResult = pit.insert(interestH);
  BOOST_CHECK_EQUAL(insertResult.second, false);// only guiders differ
  BOOST_CHECK_EQUAL(pit.size(), 7);

  insertResult = pit.insert(interestI);
  BOOST_CHECK_EQUAL(insertResult.second, false);// only guiders differ
  BOOST_CHECK_EQUAL(pit.size(), 7);

  insertResult = pit.insert(interestJ);
  BOOST_CHECK_EQUAL(insertResult.second, false);// only guiders differ
  BOOST_CHECK_EQUAL(pit.size(), 7);

  insertResult = pit.insert(interestK);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 8);
}

BOOST_AUTO_TEST_CASE(Erase)
{
  Interest interest(Name("ndn:/z88Admz6A2"));

  NameTree nameTree(16);
  Pit pit(nameTree);

  std::pair<shared_ptr<pit::Entry>, bool> insertResult;

  BOOST_CHECK_EQUAL(pit.size(), 0);

  insertResult = pit.insert(interest);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 1);

  insertResult = pit.insert(interest);
  BOOST_CHECK_EQUAL(insertResult.second, false);
  BOOST_CHECK_EQUAL(pit.size(), 1);

  pit.erase(insertResult.first);
  BOOST_CHECK_EQUAL(pit.size(), 0);

  insertResult = pit.insert(interest);
  BOOST_CHECK_EQUAL(insertResult.second, true);
  BOOST_CHECK_EQUAL(pit.size(), 1);

}

BOOST_AUTO_TEST_CASE(FindAllDataMatches)
{
  Name nameA   ("ndn:/A");
  Name nameAB  ("ndn:/A/B");
  Name nameABC ("ndn:/A/B/C");
  Name nameABCD("ndn:/A/B/C/D");
  Name nameD   ("ndn:/D");

  Interest interestA  (nameA  );
  Interest interestABC(nameABC);
  Interest interestD  (nameD  );

  NameTree nameTree(16);
  Pit pit(nameTree);
  int count = 0;

  BOOST_CHECK_EQUAL(pit.size(), 0);

  pit.insert(interestA  );
  pit.insert(interestABC);
  pit.insert(interestD  );

  nameTree.lookup(nameABCD); // make sure /A/B/C/D is in nameTree

  BOOST_CHECK_EQUAL(pit.size(), 3);

  Data data(nameABCD);

  shared_ptr<pit::DataMatchResult> matches = pit.findAllDataMatches(data);

  bool hasA   = false;
  bool hasAB  = false;
  bool hasABC = false;
  bool hasD   = false;

  for (pit::DataMatchResult::iterator it = matches->begin();
       it != matches->end(); ++it) {
    ++count;
    shared_ptr<pit::Entry> entry = *it;

    if (entry->getName().equals(nameA ))
      hasA   = true;

    if (entry->getName().equals(nameAB))
      hasAB  = true;

    if (entry->getName().equals(nameABC))
      hasABC = true;

    if (entry->getName().equals(nameD))
      hasD   = true;
  }
  BOOST_CHECK_EQUAL(hasA  , true);
  BOOST_CHECK_EQUAL(hasAB , false);
  BOOST_CHECK_EQUAL(hasABC, true);
  BOOST_CHECK_EQUAL(hasD  , false);

  BOOST_CHECK_EQUAL(count, 2);

}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd

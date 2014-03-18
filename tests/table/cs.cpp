/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 *
 * Author: Ilya Moiseenko <iliamo@ucla.edu>
 */

#include "table/cs.hpp"
#include "table/cs-entry.hpp"

#include "tests/test-common.hpp"

#include <ndn-cpp-dev/security/key-chain.hpp>

namespace nfd {
namespace tests {

class CsAccessor : public Cs
{
public:

  bool
  evictItem_accessor()
  {
    return evictItem();
  }
};

BOOST_FIXTURE_TEST_SUITE(TableCs, BaseFixture)

BOOST_AUTO_TEST_CASE(Insertion)
{
  Cs cs;
  
  shared_ptr<Data> data = make_shared<Data>("/insertion");
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  data->setSignature(fakeSignature);
  
  BOOST_CHECK_EQUAL(cs.insert(*data), true);
}

BOOST_AUTO_TEST_CASE(Insertion2)
{
  Cs cs;

  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  shared_ptr<Data> data = make_shared<Data>("/a");
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  shared_ptr<Data> data2 = make_shared<Data>("/b");
  data2->setSignature(fakeSignature);
  cs.insert(*data2);
 
  shared_ptr<Data> data3 = make_shared<Data>("/c");
  data3->setSignature(fakeSignature);
  cs.insert(*data3);
  
  shared_ptr<Data> data4 = make_shared<Data>("/d");
  data4->setSignature(fakeSignature);
  cs.insert(*data4);
  
  BOOST_CHECK_EQUAL(cs.size(), 4);
}


BOOST_AUTO_TEST_CASE(DuplicateInsertion)
{
  Cs cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  shared_ptr<Data> data0 = make_shared<Data>("/insert/smth");
  data0->setSignature(fakeSignature);
  BOOST_CHECK_EQUAL(cs.insert(*data0), true);
  
  shared_ptr<Data> data = make_shared<Data>("/insert/duplicate");
  data->setSignature(fakeSignature);
  BOOST_CHECK_EQUAL(cs.insert(*data), true);
  
  cs.insert(*data);
  BOOST_CHECK_EQUAL(cs.size(), 2);
}


BOOST_AUTO_TEST_CASE(DuplicateInsertion2)
{
  Cs cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  shared_ptr<Data> data = make_shared<Data>("/insert/duplicate");
  data->setSignature(fakeSignature);
  BOOST_CHECK_EQUAL(cs.insert(*data), true);
  
  cs.insert(*data);
  BOOST_CHECK_EQUAL(cs.size(), 1);
  
  shared_ptr<Data> data2 = make_shared<Data>("/insert/original");
  data2->setSignature(fakeSignature);
  BOOST_CHECK_EQUAL(cs.insert(*data2), true);
  BOOST_CHECK_EQUAL(cs.size(), 2);
}

BOOST_AUTO_TEST_CASE(InsertAndFind)
{
  Cs cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  Name name("/insert/and/find");
  
  shared_ptr<Data> data = make_shared<Data>(name);
  data->setSignature(fakeSignature);
  BOOST_CHECK_EQUAL(cs.insert(*data), true);
  
  shared_ptr<Interest> interest = make_shared<Interest>(name);
  
  const Data* found = cs.find(*interest);
  
  BOOST_REQUIRE(found != 0);
  BOOST_CHECK_EQUAL(data->getName(), found->getName());
}

BOOST_AUTO_TEST_CASE(InsertAndNotFind)
{
  Cs cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  Name name("/insert/and/find");
  shared_ptr<Data> data = make_shared<Data>(name);
  data->setSignature(fakeSignature);
  BOOST_CHECK_EQUAL(cs.insert(*data), true);
  
  Name name2("/not/find");
  shared_ptr<Interest> interest = make_shared<Interest>(name2);
  
  const Data* found = cs.find(*interest);
  
  BOOST_CHECK_EQUAL(found, static_cast<const Data*>(0));
}


BOOST_AUTO_TEST_CASE(InsertAndErase)
{
  CsAccessor cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  shared_ptr<Data> data = make_shared<Data>("/insertandelete");
  data->setSignature(fakeSignature);
  cs.insert(*data);
  BOOST_CHECK_EQUAL(cs.size(), 1);
  
  cs.evictItem_accessor();
  BOOST_CHECK_EQUAL(cs.size(), 0);
}

BOOST_AUTO_TEST_CASE(StalenessQueue)
{
  CsAccessor cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  Name name2("/insert/fresh");
  shared_ptr<Data> data2 = make_shared<Data>(name2);
  data2->setFreshnessPeriod(time::milliseconds(5000));
  data2->setSignature(fakeSignature);
  cs.insert(*data2);
  
  Name name("/insert/expire");
  shared_ptr<Data> data = make_shared<Data>(name);
  data->setFreshnessPeriod(time::milliseconds(500));
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  BOOST_CHECK_EQUAL(cs.size(), 2);
  
  sleep(1);
  
  cs.evictItem_accessor();
  BOOST_CHECK_EQUAL(cs.size(), 1);
  
  shared_ptr<Interest> interest = make_shared<Interest>(name2);
  const Data* found = cs.find(*interest);
  BOOST_REQUIRE(found != 0);
  BOOST_CHECK_EQUAL(found->getName(), name2);
}

//even max size
BOOST_AUTO_TEST_CASE(ReplacementEvenSize)
{
  Cs cs(4);

  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));

  shared_ptr<Data> data = make_shared<Data>("/a");
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  shared_ptr<Data> data2 = make_shared<Data>("/b");
  data2->setSignature(fakeSignature);
  cs.insert(*data2);
 
  shared_ptr<Data> data3 = make_shared<Data>("/c");
  data3->setSignature(fakeSignature);
  cs.insert(*data3);
  
  shared_ptr<Data> data4 = make_shared<Data>("/d");
  data4->setSignature(fakeSignature);
  cs.insert(*data4);
 
  shared_ptr<Data> data5 = make_shared<Data>("/e");
  data5->setSignature(fakeSignature);
  cs.insert(*data5);
  
  BOOST_CHECK_EQUAL(cs.size(), 4);
}

//odd max size
BOOST_AUTO_TEST_CASE(Replacement2)
{
  Cs cs(3);
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));

  shared_ptr<Data> data = make_shared<Data>("/a");
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  shared_ptr<Data> data2 = make_shared<Data>("/b");
  data2->setSignature(fakeSignature);
  cs.insert(*data2);
 
  shared_ptr<Data> data3 = make_shared<Data>("/c");
  data3->setSignature(fakeSignature);
  cs.insert(*data3);
  
  shared_ptr<Data> data4 = make_shared<Data>("/d");
  data4->setSignature(fakeSignature);
  cs.insert(*data4);
  
  BOOST_CHECK_EQUAL(cs.size(), 3);
}

BOOST_AUTO_TEST_CASE(InsertAndEraseByName)
{
  CsAccessor cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));

  Name name("/insertandremovebyname");
  shared_ptr<Data> data = make_shared<Data>(name);
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  uint8_t digest1[32];
  ndn::ndn_digestSha256(data->wireEncode().wire(), data->wireEncode().size(), digest1);
  
  shared_ptr<Data> data2 = make_shared<Data>("/a");
  data2->setSignature(fakeSignature);
  cs.insert(*data2);

  shared_ptr<Data> data3 = make_shared<Data>("/z");
  data3->setSignature(fakeSignature);
  cs.insert(*data3);

  BOOST_CHECK_EQUAL(cs.size(), 3);
  
  name.append(digest1, 32);
  cs.erase(name);
  BOOST_CHECK_EQUAL(cs.size(), 2);
}

BOOST_AUTO_TEST_CASE(DigestCalculation)
{
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));

  shared_ptr<Data> data = make_shared<Data>("/digest/compute");
  data->setSignature(fakeSignature);
  
  uint8_t digest1[32];
  ndn::ndn_digestSha256(data->wireEncode().wire(), data->wireEncode().size(), digest1);
  
  cs::Entry* entry = new cs::Entry(*data, false);
  
  BOOST_CHECK_EQUAL_COLLECTIONS(digest1, digest1+32,
                                entry->getDigest()->buf(), entry->getDigest()->buf()+32);
}

BOOST_AUTO_TEST_CASE(InsertCanonical)
{
  Cs cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  shared_ptr<Data> data = make_shared<Data>("/a");
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  shared_ptr<Data> data2 = make_shared<Data>("/b");
  data2->setSignature(fakeSignature);
  cs.insert(*data2);
  
  shared_ptr<Data> data3 = make_shared<Data>("/c");
  data3->setSignature(fakeSignature);
  cs.insert(*data3);
  
  shared_ptr<Data> data4 = make_shared<Data>("/d");
  data4->setSignature(fakeSignature);
  cs.insert(*data4);
  
  shared_ptr<Data> data5 = make_shared<Data>("/c/c/1/2/3/4/5/6");
  data5->setSignature(fakeSignature);
  cs.insert(*data5);
  
  shared_ptr<Data> data6 = make_shared<Data>("/c/c/1/2/3");
  data6->setSignature(fakeSignature);
  cs.insert(*data6);
  
  shared_ptr<Data> data7 = make_shared<Data>("/c/c/1");
  data7->setSignature(fakeSignature);
  cs.insert(*data7);
}

BOOST_AUTO_TEST_CASE(EraseCanonical)
{
  Cs cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  shared_ptr<Data> data = make_shared<Data>("/a");
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  shared_ptr<Data> data2 = make_shared<Data>("/b");
  data2->setSignature(fakeSignature);
  cs.insert(*data2);
  
  shared_ptr<Data> data3 = make_shared<Data>("/c");
  data3->setSignature(fakeSignature);
  cs.insert(*data3);
  
  shared_ptr<Data> data4 = make_shared<Data>("/d");
  data4->setSignature(fakeSignature);
  cs.insert(*data4);
  
  uint8_t digest1[32];
  ndn::ndn_digestSha256(data->wireEncode().wire(), data->wireEncode().size(), digest1);

  shared_ptr<Data> data5 = make_shared<Data>("/c/c/1/2/3/4/5/6");
  data5->setSignature(fakeSignature);
  cs.insert(*data5);
  
  shared_ptr<Data> data6 = make_shared<Data>("/c/c/1/2/3");
  data6->setSignature(fakeSignature);
  cs.insert(*data6);
  
  shared_ptr<Data> data7 = make_shared<Data>("/c/c/1");
  data7->setSignature(fakeSignature);
  cs.insert(*data7);

  Name name("/a");
  name.append(digest1,32);
  cs.erase(name);
  BOOST_CHECK_EQUAL(cs.size(), 6);
}

BOOST_AUTO_TEST_CASE(ImplicitDigestSelector)
{
  CsAccessor cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  Name name("/digest/works");
  shared_ptr<Data> data = make_shared<Data>(name);
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  shared_ptr<Data> data2 = make_shared<Data>("/a");
  data2->setSignature(fakeSignature);
  cs.insert(*data2);

  shared_ptr<Data> data3 = make_shared<Data>("/z/z/z");
  data3->setSignature(fakeSignature);
  cs.insert(*data3);
  
  uint8_t digest1[32];
  ndn::ndn_digestSha256(data->wireEncode().wire(), data->wireEncode().size(), digest1);
  
  uint8_t digest2[32] = {0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1};
  
  shared_ptr<Interest> interest = make_shared<Interest>(boost::cref(Name(name).append(digest1, 32)));
  interest->setMinSuffixComponents(0);
  interest->setMaxSuffixComponents(0);
  
  const Data* found = cs.find(*interest);
  BOOST_CHECK_NE(found, static_cast<const Data*>(0));
  BOOST_CHECK_EQUAL(found->getName(), name);
  
  shared_ptr<Interest> interest2 = make_shared<Interest>(boost::cref(Name(name).append(digest2, 32)));
  interest2->setMinSuffixComponents(0);
  interest2->setMaxSuffixComponents(0);
  
  const Data* notfound = cs.find(*interest2);
  BOOST_CHECK_EQUAL(notfound, static_cast<const Data*>(0));
}

BOOST_AUTO_TEST_CASE(ChildSelector)
{
  Cs cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  shared_ptr<Data> data = make_shared<Data>("/a");
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  shared_ptr<Data> data2 = make_shared<Data>("/b");
  data2->setSignature(fakeSignature);
  cs.insert(*data2);
  
  shared_ptr<Data> data4 = make_shared<Data>("/d");
  data4->setSignature(fakeSignature);
  cs.insert(*data4);
  
  shared_ptr<Data> data5 = make_shared<Data>("/c/c");
  data5->setSignature(fakeSignature);
  cs.insert(*data5);
  
  shared_ptr<Data> data6 = make_shared<Data>("/c/f");
  data6->setSignature(fakeSignature);
  cs.insert(*data6);
  
  shared_ptr<Data> data7 = make_shared<Data>("/c/n");
  data7->setSignature(fakeSignature);
  cs.insert(*data7);
  
  shared_ptr<Interest> interest = make_shared<Interest>("/c");
  interest->setChildSelector(1);
  
  const Data* found = cs.find(*interest);
  BOOST_CHECK_EQUAL(found->getName(), "/c/n");

  shared_ptr<Interest> interest2 = make_shared<Interest>("/c");
  interest2->setChildSelector(0);
  
  const Data* found2 = cs.find(*interest2);
  BOOST_CHECK_EQUAL(found2->getName(), "/c/c");
}

BOOST_AUTO_TEST_CASE(ChildSelector2)
{
  Cs cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));

  shared_ptr<Data> data = make_shared<Data>("/a/b/1");
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  shared_ptr<Data> data2 = make_shared<Data>("/a/b/2");
  data2->setSignature(fakeSignature);
  cs.insert(*data2);
  
  shared_ptr<Data> data3 = make_shared<Data>("/a/z/1");
  data3->setSignature(fakeSignature);
  cs.insert(*data3);
  
  shared_ptr<Data> data4 = make_shared<Data>("/a/z/2");
  data4->setSignature(fakeSignature);
  cs.insert(*data4);
  
  shared_ptr<Interest> interest = make_shared<Interest>("/a");
  interest->setChildSelector(1);
  
  const Data* found = cs.find(*interest);
  BOOST_CHECK_EQUAL(found->getName(), "/a/z/1");
}

BOOST_AUTO_TEST_CASE(MustBeFreshSelector)
{
  Cs cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  Name name("/insert/nonfresh");
  shared_ptr<Data> data = make_shared<Data>(name);
  data->setFreshnessPeriod(time::milliseconds(500));
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  sleep(1);
  
  shared_ptr<Interest> interest = make_shared<Interest>(name);
  interest->setMustBeFresh(true);
  
  const Data* found = cs.find(*interest);
  BOOST_CHECK_EQUAL(found, static_cast<const Data*>(0));
}

BOOST_AUTO_TEST_CASE(MinMaxComponentsSelector)
{
  Cs cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  shared_ptr<Data> data = make_shared<Data>("/a");
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  shared_ptr<Data> data2 = make_shared<Data>("/b");
  data2->setSignature(fakeSignature);
  cs.insert(*data2);
  
  shared_ptr<Data> data4 = make_shared<Data>("/d");
  data4->setSignature(fakeSignature);
  cs.insert(*data4);
  
  shared_ptr<Data> data5 = make_shared<Data>("/c/c/1/2/3/4/5/6");
  data5->setSignature(fakeSignature);
  cs.insert(*data5);
  
  shared_ptr<Data> data6 = make_shared<Data>("/c/c/6/7/8/9");
  data6->setSignature(fakeSignature);
  cs.insert(*data6);
  
  shared_ptr<Data> data7 = make_shared<Data>("/c/c/1/2/3");
  data7->setSignature(fakeSignature);
  cs.insert(*data7);
  
  shared_ptr<Data> data8 = make_shared<Data>("/c/c/1");
  data8->setSignature(fakeSignature);
  cs.insert(*data8);
  
  shared_ptr<Interest> interest = make_shared<Interest>("/c/c");
  interest->setMinSuffixComponents(3);
  interest->setChildSelector(0);
  
  const Data* found = cs.find(*interest);
  BOOST_CHECK_EQUAL(found->getName(), "/c/c/1/2/3/4/5/6");

  shared_ptr<Interest> interest2 = make_shared<Interest>("/c/c");
  interest2->setMinSuffixComponents(4);
  interest2->setChildSelector(1);
  
  const Data* found2 = cs.find(*interest2);
  BOOST_CHECK_EQUAL(found2->getName(), "/c/c/6/7/8/9");
  
  shared_ptr<Interest> interest3 = make_shared<Interest>("/c/c");
  interest3->setMaxSuffixComponents(2);
  interest3->setChildSelector(1);
  
  const Data* found3 = cs.find(*interest3);
  BOOST_CHECK_EQUAL(found3->getName(), "/c/c/1");
}

BOOST_AUTO_TEST_CASE(ExcludeSelector)
{
  Cs cs;
  
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  
  shared_ptr<Data> data = make_shared<Data>("/a");
  data->setSignature(fakeSignature);
  cs.insert(*data);
  
  shared_ptr<Data> data2 = make_shared<Data>("/b");
  data2->setSignature(fakeSignature);
  cs.insert(*data2);
  
  shared_ptr<Data> data3 = make_shared<Data>("/c/a");
  data3->setSignature(fakeSignature);
  cs.insert(*data3);
  
  shared_ptr<Data> data4 = make_shared<Data>("/d");
  data4->setSignature(fakeSignature);
  cs.insert(*data4);

  shared_ptr<Data> data5 = make_shared<Data>("/c/c");
  data5->setSignature(fakeSignature);
  cs.insert(*data5);
  
  shared_ptr<Data> data6 = make_shared<Data>("/c/f");
  data6->setSignature(fakeSignature);
  cs.insert(*data6);
  
  shared_ptr<Data> data7 = make_shared<Data>("/c/n");
  data7->setSignature(fakeSignature);
  cs.insert(*data7);
  
  shared_ptr<Interest> interest = make_shared<Interest>("/c");
  interest->setChildSelector(1);
  Exclude e;
  e.excludeOne (Name::Component("n"));
  interest->setExclude(e);
  
  const Data* found = cs.find(*interest);
  BOOST_CHECK_EQUAL(found->getName(), "/c/f");

  shared_ptr<Interest> interest2 = make_shared<Interest>("/c");
  interest2->setChildSelector(0);
  
  Exclude e2;
  e2.excludeOne (Name::Component("a"));
  interest2->setExclude(e2);

  const Data* found2 = cs.find(*interest2);
  BOOST_CHECK_EQUAL(found2->getName(), "/c/c");
  
  shared_ptr<Interest> interest3 = make_shared<Interest>("/c");
  interest3->setChildSelector(0);
  
  Exclude e3;
  e3.excludeOne (Name::Component("c"));
  interest3->setExclude(e3);

  const Data* found3 = cs.find(*interest3);
  BOOST_CHECK_EQUAL(found3->getName(), "/c/a");
}

//

class FindFixture : protected BaseFixture
{
protected:
  void
  insert(uint32_t id, const Name& name)
  {
    shared_ptr<Data> data = make_shared<Data>(name);
    data->setFreshnessPeriod(time::milliseconds(99999));
    data->setContent(reinterpret_cast<const uint8_t*>(&id), sizeof(id));
    
    ndn::SignatureSha256WithRsa fakeSignature;
    fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
    data->setSignature(fakeSignature);
    
    m_cs.insert(*data);
  }
  
  Interest&
  startInterest(const Name& name)
  {
    m_interest = make_shared<Interest>(name);
    return *m_interest;
  }
  
  uint32_t
  find()
  {
    const Data* found = m_cs.find(*m_interest);
    if (found == 0) {
      return 0;
    }
    const Block& content = found->getContent();
    if (content.value_size() != sizeof(uint32_t)) {
      return 0;
    }
    return *reinterpret_cast<const uint32_t*>(content.value());
  }
  
protected:
  Cs m_cs;
  shared_ptr<Interest> m_interest;
};

BOOST_FIXTURE_TEST_SUITE(Find, FindFixture)

BOOST_AUTO_TEST_CASE(EmptyDataName)
{
  insert(1, "ndn:/");
  
  startInterest("ndn:/");
  BOOST_CHECK_EQUAL(find(), 1);
}

BOOST_AUTO_TEST_CASE(EmptyInterestName)
{
  insert(1, "ndn:/A");
  
  startInterest("ndn:/");
  BOOST_CHECK_EQUAL(find(), 1);
}

BOOST_AUTO_TEST_CASE(Leftmost)
{
  insert(1, "ndn:/A");
  insert(2, "ndn:/B/p/1");
  insert(3, "ndn:/B/p/2");
  insert(4, "ndn:/B/q/1");
  insert(5, "ndn:/B/q/2");
  insert(6, "ndn:/C");
  
  startInterest("ndn:/B");
  BOOST_CHECK_EQUAL(find(), 2);
}

BOOST_AUTO_TEST_CASE(Rightmost)
{
  insert(1, "ndn:/A");
  insert(2, "ndn:/B/p/1");
  insert(3, "ndn:/B/p/2");
  insert(4, "ndn:/B/q/1");
  insert(5, "ndn:/B/q/2");
  insert(6, "ndn:/C");
  
  startInterest("ndn:/B")
    .setChildSelector(1);
  BOOST_CHECK_EQUAL(find(), 4);
}

BOOST_AUTO_TEST_CASE(Leftmost_ExactName1)
{
  insert(1, "ndn:/");
  insert(2, "ndn:/A/B");
  insert(3, "ndn:/A/C");
  insert(4, "ndn:/A");
  insert(5, "ndn:/D");
  
  // Intuitively you would think Data 4 should be between Data 1 and 2,
  // but Data 4 has full Name ndn:/A/<32-octet hash>.
  startInterest("ndn:/A");
  BOOST_CHECK_EQUAL(find(), 2);
}

BOOST_AUTO_TEST_CASE(Leftmost_ExactName33)
{
  insert(1, "ndn:/");
  insert(2, "ndn:/A");
  insert(3, "ndn:/A/BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"); // 33 'B's
  insert(4, "ndn:/A/CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"); // 33 'C's
  insert(5, "ndn:/D");
  
  // Data 2 is returned, because <32-octet hash> is less than Data 3.
  startInterest("ndn:/A");
  BOOST_CHECK_EQUAL(find(), 2);
}

BOOST_AUTO_TEST_CASE(MinSuffixComponents)
{
  insert(1, "ndn:/A/1/2/3/4");
  insert(2, "ndn:/B/1/2/3");
  insert(3, "ndn:/C/1/2");
  insert(4, "ndn:/D/1");
  insert(5, "ndn:/E");
  insert(6, "ndn:/");
  
  startInterest("ndn:/")
    .setChildSelector(1)
    .setMinSuffixComponents(0);
  BOOST_CHECK_EQUAL(find(), 6);
  
  startInterest("ndn:/")
    .setChildSelector(1)
    .setMinSuffixComponents(1);
  BOOST_CHECK_EQUAL(find(), 6);
  
  startInterest("ndn:/")
    .setChildSelector(1)
    .setMinSuffixComponents(2);
  BOOST_CHECK_EQUAL(find(), 5);
  
  startInterest("ndn:/")
    .setChildSelector(1)
    .setMinSuffixComponents(3);
  BOOST_CHECK_EQUAL(find(), 4);
  
  startInterest("ndn:/")
    .setChildSelector(1)
    .setMinSuffixComponents(4);
  BOOST_CHECK_EQUAL(find(), 3);
  
  startInterest("ndn:/")
    .setChildSelector(1)
    .setMinSuffixComponents(5);
  BOOST_CHECK_EQUAL(find(), 2);
  
  startInterest("ndn:/")
    .setChildSelector(1)
    .setMinSuffixComponents(6);
  BOOST_CHECK_EQUAL(find(), 1);
  
  startInterest("ndn:/")
    .setChildSelector(1)
    .setMinSuffixComponents(7);
  BOOST_CHECK_EQUAL(find(), 0);
}

BOOST_AUTO_TEST_CASE(MaxSuffixComponents)
{
  insert(1, "ndn:/");
  insert(2, "ndn:/A");
  insert(3, "ndn:/A/B");
  insert(4, "ndn:/A/B/C");
  insert(5, "ndn:/A/B/C/D");
  insert(6, "ndn:/A/B/C/D/E");
  // Order is 6,5,4,3,2,1, because <32-octet hash> is greater than a 1-octet component.
  
  startInterest("ndn:/")
    .setMaxSuffixComponents(0);
  BOOST_CHECK_EQUAL(find(), 0);
  
  startInterest("ndn:/")
    .setMaxSuffixComponents(1);
  BOOST_CHECK_EQUAL(find(), 1);
  
  startInterest("ndn:/")
    .setMaxSuffixComponents(2);
  BOOST_CHECK_EQUAL(find(), 2);
  
  startInterest("ndn:/")
    .setMaxSuffixComponents(3);
  BOOST_CHECK_EQUAL(find(), 3);
  
  startInterest("ndn:/")
    .setMaxSuffixComponents(4);
  BOOST_CHECK_EQUAL(find(), 4);
  
  startInterest("ndn:/")
    .setMaxSuffixComponents(5);
  BOOST_CHECK_EQUAL(find(), 5);
  
  startInterest("ndn:/")
    .setMaxSuffixComponents(6);
  BOOST_CHECK_EQUAL(find(), 6);
}

BOOST_AUTO_TEST_CASE(DigestOrder)
{
  insert(1, "ndn:/A");
  insert(2, "ndn:/A");
  // We don't know which comes first, but there must be some order
  
  startInterest("ndn:/A")
    .setChildSelector(0);
  uint32_t leftmost = find();
  
  startInterest("ndn:/A")
    .setChildSelector(1);
  uint32_t rightmost = find();
  
  BOOST_CHECK_NE(leftmost, rightmost);
}

BOOST_AUTO_TEST_CASE(DigestExclude)
{
  insert(1, "ndn:/A/B");
  insert(2, "ndn:/A");
  insert(3, "ndn:/A/CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"); // 33 'C's
  
  startInterest("ndn:/A")
    .setExclude(Exclude().excludeBefore(ndn::name::Component(reinterpret_cast<const uint8_t*>(
        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"), 31))); // 31 0xFF's
  BOOST_CHECK_EQUAL(find(), 2);
  
  startInterest("ndn:/A")
    .setChildSelector(1)
    .setExclude(Exclude().excludeAfter(ndn::name::Component(reinterpret_cast<const uint8_t*>(
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00"), 33))); // 33 0x00's
  BOOST_CHECK_EQUAL(find(), 2);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd

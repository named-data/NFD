/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \author Ilya Moiseenko <iliamo@ucla.edu>
 **/

#include "table/cs.hpp"
#include "table/cs-entry.hpp"

#include "tests/test-common.hpp"

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

BOOST_AUTO_TEST_CASE(SetLimit)
{
  Cs cs(1);

  BOOST_CHECK_EQUAL(cs.insert(*makeData("/1")), true);
  BOOST_CHECK_EQUAL(cs.insert(*makeData("/2")), true);
  BOOST_CHECK_EQUAL(cs.size(), 1);

  cs.setLimit(3);
  BOOST_CHECK_EQUAL(cs.insert(*makeData("/3")), true);
  BOOST_CHECK_EQUAL(cs.insert(*makeData("/4")), true);
  BOOST_CHECK_EQUAL(cs.insert(*makeData("/5")), true);
  BOOST_CHECK_EQUAL(cs.size(), 3);

  cs.setLimit(2);
  BOOST_CHECK_EQUAL(cs.size(), 2);
}

BOOST_AUTO_TEST_CASE(Insertion)
{
  Cs cs;

  BOOST_CHECK_EQUAL(cs.insert(*makeData("/insertion")), true);
}

BOOST_AUTO_TEST_CASE(Insertion2)
{
  Cs cs;

  cs.insert(*makeData("/a"));
  cs.insert(*makeData("/b"));
  cs.insert(*makeData("/c"));
  cs.insert(*makeData("/d"));

  BOOST_CHECK_EQUAL(cs.size(), 4);
}

BOOST_AUTO_TEST_CASE(DuplicateInsertion)
{
  Cs cs;

  shared_ptr<Data> data0 = makeData("/insert/smth");
  BOOST_CHECK_EQUAL(cs.insert(*data0), true);

  shared_ptr<Data> data = makeData("/insert/duplicate");
  BOOST_CHECK_EQUAL(cs.insert(*data), true);

  cs.insert(*data);
  BOOST_CHECK_EQUAL(cs.size(), 2);
}


BOOST_AUTO_TEST_CASE(DuplicateInsertion2)
{
  Cs cs;

  shared_ptr<Data> data = makeData("/insert/duplicate");
  BOOST_CHECK_EQUAL(cs.insert(*data), true);

  cs.insert(*data);
  BOOST_CHECK_EQUAL(cs.size(), 1);

  shared_ptr<Data> data2 = makeData("/insert/original");
  BOOST_CHECK_EQUAL(cs.insert(*data2), true);
  BOOST_CHECK_EQUAL(cs.size(), 2);
}

BOOST_AUTO_TEST_CASE(InsertAndFind)
{
  Cs cs;

  Name name("/insert/and/find");

  shared_ptr<Data> data = makeData(name);
  BOOST_CHECK_EQUAL(cs.insert(*data), true);

  shared_ptr<Interest> interest = make_shared<Interest>(name);

  const Data* found = cs.find(*interest);

  BOOST_REQUIRE(found != 0);
  BOOST_CHECK_EQUAL(data->getName(), found->getName());
}

BOOST_AUTO_TEST_CASE(InsertAndNotFind)
{
  Cs cs;

  Name name("/insert/and/find");
  shared_ptr<Data> data = makeData(name);
  BOOST_CHECK_EQUAL(cs.insert(*data), true);

  Name name2("/not/find");
  shared_ptr<Interest> interest = make_shared<Interest>(name2);

  const Data* found = cs.find(*interest);

  BOOST_CHECK_EQUAL(found, static_cast<const Data*>(0));
}


BOOST_AUTO_TEST_CASE(InsertAndErase)
{
  CsAccessor cs;

  shared_ptr<Data> data = makeData("/insertandelete");
  cs.insert(*data);
  BOOST_CHECK_EQUAL(cs.size(), 1);

  cs.evictItem_accessor();
  BOOST_CHECK_EQUAL(cs.size(), 0);
}

BOOST_AUTO_TEST_CASE(StalenessQueue)
{
  CsAccessor cs;

  Name name2("/insert/fresh");
  shared_ptr<Data> data2 = makeData(name2);
  data2->setFreshnessPeriod(time::milliseconds(5000));
  signData(data2);
  cs.insert(*data2);

  Name name("/insert/expire");
  shared_ptr<Data> data = makeData(name);
  data->setFreshnessPeriod(time::milliseconds(500));
  signData(data);
  cs.insert(*data);

  BOOST_CHECK_EQUAL(cs.size(), 2);

  sleep(3);

  cs.evictItem_accessor();
  BOOST_CHECK_EQUAL(cs.size(), 1);

  shared_ptr<Interest> interest = make_shared<Interest>(name2);
  const Data* found = cs.find(*interest);
  BOOST_REQUIRE(found != 0);
  BOOST_CHECK_EQUAL(found->getName(), name2);
}

BOOST_AUTO_TEST_CASE(Bug2043) // eviction order of unsolicited packets
{
  Cs cs(3);

  cs.insert(*makeData("/a"), true);
  cs.insert(*makeData("/b"), true);
  cs.insert(*makeData("/c"), true);
  BOOST_CHECK_EQUAL(cs.size(), 3);

  cs.insert(*makeData("/d"), true);
  BOOST_CHECK_EQUAL(cs.size(), 3);

  const Data* oldestData = cs.find(Interest("/a"));
  const Data* newestData = cs.find(Interest("/c"));
  BOOST_CHECK(oldestData == 0);
  BOOST_CHECK(newestData != 0);
}

BOOST_AUTO_TEST_CASE(UnsolicitedWithSolicitedEvictionOrder)
{
  Cs cs(3);

  cs.insert(*makeData("/a"), true);
  cs.insert(*makeData("/b"), false);
  cs.insert(*makeData("/c"), true);
  BOOST_CHECK_EQUAL(cs.size(), 3);

  cs.insert(*makeData("/d"), true);
  BOOST_CHECK_EQUAL(cs.size(), 3);

  const Data* oldestData = cs.find(Interest("/a"));
  const Data* newestData = cs.find(Interest("/c"));
  BOOST_CHECK(oldestData == 0);
  BOOST_CHECK(newestData != 0);
}

//even max size
BOOST_AUTO_TEST_CASE(ReplacementEvenSize)
{
  Cs cs(4);

  shared_ptr<Data> data = makeData("/a");
  cs.insert(*data);

  shared_ptr<Data> data2 = makeData("/b");
  cs.insert(*data2);

  shared_ptr<Data> data3 = makeData("/c");
  cs.insert(*data3);

  shared_ptr<Data> data4 = makeData("/d");
  cs.insert(*data4);

  shared_ptr<Data> data5 = makeData("/e");
  cs.insert(*data5);

  BOOST_CHECK_EQUAL(cs.size(), 4);
}

//odd max size
BOOST_AUTO_TEST_CASE(Replacement2)
{
  Cs cs(3);

  shared_ptr<Data> data = makeData("/a");
  cs.insert(*data);

  shared_ptr<Data> data2 = makeData("/b");
  cs.insert(*data2);

  shared_ptr<Data> data3 = makeData("/c");
  cs.insert(*data3);

  shared_ptr<Data> data4 = makeData("/d");
  cs.insert(*data4);

  BOOST_CHECK_EQUAL(cs.size(), 3);
}

BOOST_AUTO_TEST_CASE(InsertAndEraseByName)
{
  CsAccessor cs;

  Name name("/insertandremovebyname");
  shared_ptr<Data> data = makeData(name);
  cs.insert(*data);

  ndn::ConstBufferPtr digest1 = ndn::crypto::sha256(data->wireEncode().wire(),
                                                    data->wireEncode().size());

  shared_ptr<Data> data2 = makeData("/a");
  cs.insert(*data2);

  shared_ptr<Data> data3 = makeData("/z");
  cs.insert(*data3);

  BOOST_CHECK_EQUAL(cs.size(), 3);

  name.appendImplicitSha256Digest(digest1);
  cs.erase(name);
  BOOST_CHECK_EQUAL(cs.size(), 2);
}

BOOST_AUTO_TEST_CASE(DigestCalculation)
{
  shared_ptr<Data> data = makeData("/digest/compute");

  ndn::ConstBufferPtr digest1 = ndn::crypto::sha256(data->wireEncode().wire(),
                                                    data->wireEncode().size());
  BOOST_CHECK_EQUAL(digest1->size(), 32);

  cs::Entry* entry = new cs::Entry();
  entry->setData(*data, false);

  BOOST_CHECK_EQUAL_COLLECTIONS(digest1->begin(), digest1->end(),
                                entry->getFullName()[-1].value_begin(),
                                entry->getFullName()[-1].value_end());
}

BOOST_AUTO_TEST_CASE(InsertCanonical)
{
  Cs cs;

  shared_ptr<Data> data = makeData("/a");
  cs.insert(*data);

  shared_ptr<Data> data2 = makeData("/b");
  cs.insert(*data2);

  shared_ptr<Data> data3 = makeData("/c");
  cs.insert(*data3);

  shared_ptr<Data> data4 = makeData("/d");
  cs.insert(*data4);

  shared_ptr<Data> data5 = makeData("/c/c/1/2/3/4/5/6");
  cs.insert(*data5);

  shared_ptr<Data> data6 = makeData("/c/c/1/2/3");
  cs.insert(*data6);

  shared_ptr<Data> data7 = makeData("/c/c/1");
  cs.insert(*data7);
}

BOOST_AUTO_TEST_CASE(EraseCanonical)
{
  Cs cs;

  shared_ptr<Data> data = makeData("/a");
  cs.insert(*data);

  shared_ptr<Data> data2 = makeData("/b");
  cs.insert(*data2);

  shared_ptr<Data> data3 = makeData("/c");
  cs.insert(*data3);

  shared_ptr<Data> data4 = makeData("/d");
  cs.insert(*data4);

  shared_ptr<Data> data5 = makeData("/c/c/1/2/3/4/5/6");
  cs.insert(*data5);

  shared_ptr<Data> data6 = makeData("/c/c/1/2/3");
  cs.insert(*data6);

  shared_ptr<Data> data7 = makeData("/c/c/1");
  cs.insert(*data7);

  ndn::ConstBufferPtr digest1 = ndn::crypto::sha256(data->wireEncode().wire(),
                                                    data->wireEncode().size());

  Name name("/a");
  name.appendImplicitSha256Digest(digest1->buf(), digest1->size());
  cs.erase(name);
  BOOST_CHECK_EQUAL(cs.size(), 6);
}

BOOST_AUTO_TEST_CASE(ImplicitDigestSelector)
{
  CsAccessor cs;

  Name name("/digest/works");
  shared_ptr<Data> data = makeData(name);
  cs.insert(*data);

  shared_ptr<Data> data2 = makeData("/a");
  cs.insert(*data2);

  shared_ptr<Data> data3 = makeData("/z/z/z");
  cs.insert(*data3);

  ndn::ConstBufferPtr digest1 = ndn::crypto::sha256(data->wireEncode().wire(),
                                                    data->wireEncode().size());
  uint8_t digest2[32] = {0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1};

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setName(Name(name).append(digest1->buf(), digest1->size()));
  interest->setMinSuffixComponents(0);
  interest->setMaxSuffixComponents(0);

  const Data* found = cs.find(*interest);
  BOOST_CHECK_NE(found, static_cast<const Data*>(0));
  BOOST_CHECK_EQUAL(found->getName(), name);

  shared_ptr<Interest> interest2 = make_shared<Interest>();
  interest2->setName(Name(name).append(digest2, 32));
  interest2->setMinSuffixComponents(0);
  interest2->setMaxSuffixComponents(0);

  const Data* notfound = cs.find(*interest2);
  BOOST_CHECK_EQUAL(notfound, static_cast<const Data*>(0));
}

BOOST_AUTO_TEST_CASE(ChildSelector)
{
  Cs cs;

  shared_ptr<Data> data = makeData("/a");
  cs.insert(*data);

  shared_ptr<Data> data2 = makeData("/b");
  cs.insert(*data2);

  shared_ptr<Data> data4 = makeData("/d");
  cs.insert(*data4);

  shared_ptr<Data> data5 = makeData("/c/c");
  cs.insert(*data5);

  shared_ptr<Data> data6 = makeData("/c/f");
  cs.insert(*data6);

  shared_ptr<Data> data7 = makeData("/c/n");
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

  shared_ptr<Data> data = makeData("/a/b/1");
  cs.insert(*data);

  shared_ptr<Data> data2 = makeData("/a/b/2");
  cs.insert(*data2);

  shared_ptr<Data> data3 = makeData("/a/z/1");
  cs.insert(*data3);

  shared_ptr<Data> data4 = makeData("/a/z/2");
  cs.insert(*data4);

  shared_ptr<Interest> interest = make_shared<Interest>("/a");
  interest->setChildSelector(1);

  const Data* found = cs.find(*interest);
  BOOST_CHECK_EQUAL(found->getName(), "/a/z/1");
}

BOOST_AUTO_TEST_CASE(MustBeFreshSelector)
{
  Cs cs;

  Name name("/insert/nonfresh");
  shared_ptr<Data> data = makeData(name);
  data->setFreshnessPeriod(time::milliseconds(500));
  signData(data);
  cs.insert(*data);

  sleep(1);

  shared_ptr<Interest> interest = make_shared<Interest>(name);
  interest->setMustBeFresh(true);

  const Data* found = cs.find(*interest);
  BOOST_CHECK_EQUAL(found, static_cast<const Data*>(0));
}

BOOST_AUTO_TEST_CASE(PublisherKeySelector)
{
  Cs cs;

  Name name("/insert/withkey");
  shared_ptr<Data> data = makeData(name);
  cs.insert(*data);

  shared_ptr<Interest> interest = make_shared<Interest>(name);
  Name keyName("/somewhere/key");

  ndn::KeyLocator locator(keyName);
  interest->setPublisherPublicKeyLocator(locator);

  const Data* found = cs.find(*interest);
  BOOST_CHECK_EQUAL(found, static_cast<const Data*>(0));
}

BOOST_AUTO_TEST_CASE(PublisherKeySelector2)
{
  Cs cs;
  Name name("/insert/withkey");
  shared_ptr<Data> data = makeData(name);
  cs.insert(*data);

  Name name2("/insert/withkey2");
  shared_ptr<Data> data2 = make_shared<Data>(name2);

  Name keyName("/somewhere/key");
  const ndn::KeyLocator locator(keyName);

  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue,
                                        reinterpret_cast<const uint8_t*>(0), 0));

  fakeSignature.setKeyLocator(locator);
  data2->setSignature(fakeSignature);
  data2->wireEncode();

  cs.insert(*data2);

  shared_ptr<Interest> interest = make_shared<Interest>(name2);
  interest->setPublisherPublicKeyLocator(locator);

  const Data* found = cs.find(*interest);
  BOOST_CHECK_NE(found, static_cast<const Data*>(0));
  BOOST_CHECK_EQUAL(found->getName(), data2->getName());
}


/// @todo Expected failures, needs to be fixed as part of Issue #2118
BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(MinMaxComponentsSelector, 1)
BOOST_AUTO_TEST_CASE(MinMaxComponentsSelector)
{
  Cs cs;

  shared_ptr<Data> data = makeData("/a");
  cs.insert(*data);

  shared_ptr<Data> data2 = makeData("/b");
  cs.insert(*data2);

  shared_ptr<Data> data4 = makeData("/d");
  cs.insert(*data4);

  shared_ptr<Data> data5 = makeData("/c/c/1/2/3/4/5/6");
  cs.insert(*data5);

  shared_ptr<Data> data6 = makeData("/c/c/6/7/8/9");
  cs.insert(*data6);

  shared_ptr<Data> data7 = makeData("/c/c/1/2/3");
  cs.insert(*data7);

  shared_ptr<Data> data8 = makeData("/c/c/1");
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

  shared_ptr<Data> data = makeData("/a");
  cs.insert(*data);

  shared_ptr<Data> data2 = makeData("/b");
  cs.insert(*data2);

  shared_ptr<Data> data3 = makeData("/c/a");
  cs.insert(*data3);

  shared_ptr<Data> data4 = makeData("/d");
  cs.insert(*data4);

  shared_ptr<Data> data5 = makeData("/c/c");
  cs.insert(*data5);

  shared_ptr<Data> data6 = makeData("/c/f");
  cs.insert(*data6);

  shared_ptr<Data> data7 = makeData("/c/n");
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
    shared_ptr<Data> data = makeData(name);
    data->setFreshnessPeriod(time::milliseconds(99999));
    data->setContent(reinterpret_cast<const uint8_t*>(&id), sizeof(id));
    signData(data);

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

/// @todo Expected failures, needs to be fixed as part of Issue #2118
BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(Leftmost_ExactName1, 1)
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

/// @todo Expected failures, needs to be fixed as part of Issue #2118
BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(MinSuffixComponents, 2)
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

/// @todo Expected failures, needs to be fixed as part of Issue #2118
BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(MaxSuffixComponents, 5)
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

/// @todo Expected failures, needs to be fixed as part of Issue #2118
BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(DigestExclude, 1)
BOOST_AUTO_TEST_CASE(DigestExclude)
{
  insert(1, "ndn:/A/B");
  insert(2, "ndn:/A");
  insert(3, "ndn:/A/CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"); // 33 'C's

  startInterest("ndn:/A")
    .setExclude(Exclude().excludeBefore(name::Component(reinterpret_cast<const uint8_t*>(
        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"), 31))); // 31 0xFF's
  BOOST_CHECK_EQUAL(find(), 2);

  startInterest("ndn:/A")
    .setChildSelector(1)
    .setExclude(Exclude().excludeAfter(name::Component(reinterpret_cast<const uint8_t*>(
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00"), 33))); // 33 0x00's
  BOOST_CHECK_EQUAL(find(), 2);
}

BOOST_AUTO_TEST_CASE(ExactName32)
{
  insert(1, "ndn:/A/BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"); // 32 'B's
  insert(2, "ndn:/A/CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"); // 32 'C's

  startInterest("ndn:/A/BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
  BOOST_CHECK_EQUAL(find(), 1);
}

/// @todo Expected failures, needs to be fixed as part of Issue #2118
BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(MinSuffixComponents32, 2)
BOOST_AUTO_TEST_CASE(MinSuffixComponents32)
{
  insert(1, "ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/A/1/2/3/4"); // 32 'x's
  insert(2, "ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/B/1/2/3");
  insert(3, "ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/C/1/2");
  insert(4, "ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/D/1");
  insert(5, "ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/E");
  insert(6, "ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setChildSelector(1)
    .setMinSuffixComponents(0);
  BOOST_CHECK_EQUAL(find(), 6);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setChildSelector(1)
    .setMinSuffixComponents(1);
  BOOST_CHECK_EQUAL(find(), 6);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setChildSelector(1)
    .setMinSuffixComponents(2);
  BOOST_CHECK_EQUAL(find(), 5);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setChildSelector(1)
    .setMinSuffixComponents(3);
  BOOST_CHECK_EQUAL(find(), 4);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setChildSelector(1)
    .setMinSuffixComponents(4);
  BOOST_CHECK_EQUAL(find(), 3);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setChildSelector(1)
    .setMinSuffixComponents(5);
  BOOST_CHECK_EQUAL(find(), 2);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setChildSelector(1)
    .setMinSuffixComponents(6);
  BOOST_CHECK_EQUAL(find(), 1);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setChildSelector(1)
    .setMinSuffixComponents(7);
  BOOST_CHECK_EQUAL(find(), 0);
}

/// @todo Expected failures, needs to be fixed as part of Issue #2118
BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(MaxSuffixComponents32, 5)
BOOST_AUTO_TEST_CASE(MaxSuffixComponents32)
{
  insert(1, "ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/"); // 32 'x's
  insert(2, "ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/A");
  insert(3, "ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/A/B");
  insert(4, "ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/A/B/C");
  insert(5, "ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/A/B/C/D");
  insert(6, "ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/A/B/C/D/E");
  // Order is 6,5,4,3,2,1, because <32-octet hash> is greater than a 1-octet component.

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setMaxSuffixComponents(0);
  BOOST_CHECK_EQUAL(find(), 0);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setMaxSuffixComponents(1);
  BOOST_CHECK_EQUAL(find(), 1);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setMaxSuffixComponents(2);
  BOOST_CHECK_EQUAL(find(), 2);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setMaxSuffixComponents(3);
  BOOST_CHECK_EQUAL(find(), 3);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setMaxSuffixComponents(4);
  BOOST_CHECK_EQUAL(find(), 4);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setMaxSuffixComponents(5);
  BOOST_CHECK_EQUAL(find(), 5);

  startInterest("ndn:/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
    .setMaxSuffixComponents(6);
  BOOST_CHECK_EQUAL(find(), 6);
}

BOOST_AUTO_TEST_SUITE_END() // Find

BOOST_AUTO_TEST_SUITE_END() // TableCs

} // namespace tests
} // namespace nfd

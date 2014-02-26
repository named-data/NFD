/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "table/name-tree.hpp"
#include <boost/test/unit_test.hpp>

namespace nfd {

using name_tree::Entry;

BOOST_AUTO_TEST_SUITE(TableNameTree)

BOOST_AUTO_TEST_CASE (Entry)
{
  Name prefix("ndn:/named-data/research/abc/def/ghi");

  name_tree::Entry npe = name_tree::Entry(prefix);
  BOOST_CHECK_EQUAL(npe.getPrefix(), prefix);

  // examine all the get methods

  uint32_t hash = npe.getHash();
  BOOST_CHECK_EQUAL(hash, 0);

  shared_ptr<name_tree::Entry> parent = npe.getParent();
  BOOST_CHECK(!static_cast<bool>(parent));

  std::vector<shared_ptr<name_tree::Entry> >& childList = npe.getChildren();
  BOOST_CHECK_EQUAL(childList.size(), 0);

  shared_ptr<fib::Entry> fib = npe.getFibEntry();
  BOOST_CHECK(!static_cast<bool>(fib));

  std::vector< shared_ptr<pit::Entry> >& pitList = npe.getPitEntries();
  BOOST_CHECK_EQUAL(pitList.size(), 0);

  // examine all the set method 

  npe.setHash(12345);
  BOOST_CHECK_EQUAL(npe.getHash(), 12345);

  Name parentName("ndn:/named-data/research/abc/def");
  parent = make_shared<name_tree::Entry>(parentName);
  npe.setParent(parent);
  BOOST_CHECK_EQUAL(npe.getParent(), parent);

  // Insert FIB 

  shared_ptr<fib::Entry> fibEntry(new fib::Entry(prefix));
  shared_ptr<fib::Entry> fibEntryParent(new fib::Entry(parentName));
  
  npe.setFibEntry(fibEntry);
  BOOST_CHECK_EQUAL(npe.getFibEntry(), fibEntry);

  // Erase a FIB that does not exist 
  BOOST_CHECK_EQUAL(npe.
  eraseFibEntry(fibEntryParent), false);
  BOOST_CHECK_EQUAL(npe.getFibEntry(), fibEntry);

  // Erase the FIB that exists
  BOOST_CHECK_EQUAL(npe.
  eraseFibEntry(fibEntry), true);
  BOOST_CHECK(!static_cast<bool>(npe.getFibEntry()));

  // Insert a PIT

  shared_ptr<pit::Entry> PitEntry(make_shared<pit::Entry>(prefix));
  shared_ptr<pit::Entry> PitEntry2(make_shared<pit::Entry>(parentName)); 

  Name prefix3("ndn:/named-data/research/abc/def");
  shared_ptr<pit::Entry> PitEntry3(make_shared<pit::Entry>(prefix3));

  npe.insertPitEntry(PitEntry);
  BOOST_CHECK_EQUAL(npe.getPitEntries().size(), 1);

  npe.insertPitEntry(PitEntry2);
  BOOST_CHECK_EQUAL(npe.getPitEntries().size(), 2);

  BOOST_CHECK_EQUAL(npe.
  erasePitEntry(PitEntry), true);
  BOOST_CHECK_EQUAL(npe.getPitEntries().size(), 1);

  // erase a PIT Entry that does not exist

  BOOST_CHECK_EQUAL(npe.
  erasePitEntry(PitEntry3), false);
  BOOST_CHECK_EQUAL(npe.getPitEntries().size(), 1);

  BOOST_CHECK_EQUAL(npe.
  erasePitEntry(PitEntry2), true);
  BOOST_CHECK_EQUAL(npe.getPitEntries().size(), 0);

  // erase a PIT Entry that does not exist any more

  BOOST_CHECK_EQUAL(npe.
  erasePitEntry(PitEntry2), false);
}

BOOST_AUTO_TEST_CASE (NameTreeBasic)
{
  size_t nBuckets = 16;
  NameTree nt(nBuckets);

  BOOST_CHECK_EQUAL(nt.size(), 0);
  BOOST_CHECK_EQUAL(nt.getNBuckets(), nBuckets); 

  Name nameABC = ("ndn:/a/b/c");
  shared_ptr<name_tree::Entry> npeABC = nt.lookup(nameABC);
  BOOST_CHECK_EQUAL(nt.size(), 4);
  
  Name nameABD = ("/a/b/d");
  shared_ptr<name_tree::Entry> npeABD = nt.lookup(nameABD);
  BOOST_CHECK_EQUAL(nt.size(), 5);

  Name nameAE = ("/a/e/");
  shared_ptr<name_tree::Entry> npeAE = nt.lookup(nameAE);
  BOOST_CHECK_EQUAL(nt.size(), 6);

  Name nameF = ("/f");
  shared_ptr<name_tree::Entry> npeF = nt.lookup(nameF);
  BOOST_CHECK_EQUAL(nt.size(), 7);

  // validate lookup() and findExactMatch() 

  Name nameAB ("/a/b");
  BOOST_CHECK_EQUAL(npeABC->getParent(), nt.findExactMatch(nameAB));
  BOOST_CHECK_EQUAL(npeABD->getParent(), nt.findExactMatch(nameAB));

  Name nameA ("/a");
  BOOST_CHECK_EQUAL(npeAE->getParent(), nt.findExactMatch(nameA));

  Name nameRoot ("/");
  BOOST_CHECK_EQUAL(npeF->getParent(), nt.findExactMatch(nameRoot));
  BOOST_CHECK_EQUAL(nt.size(), 7);

  Name name0 = ("/does/not/exist");
  shared_ptr<name_tree::Entry> npe0 = nt.findExactMatch(name0);
  BOOST_CHECK(!static_cast<bool>(npe0));


  // Longest Prefix Matching

  shared_ptr<name_tree::Entry> temp;
  Name nameABCLPM("/a/b/c/def/asdf/nlf");
  temp = nt.findLongestPrefixMatch(nameABCLPM);
  BOOST_CHECK_EQUAL(temp, nt.findExactMatch(nameABC));

  Name nameABDLPM("/a/b/d/def/asdf/nlf");
  temp = nt.findLongestPrefixMatch(nameABDLPM);
  BOOST_CHECK_EQUAL(temp, nt.findExactMatch(nameABD));

  Name nameABLPM("/a/b/hello/world");
  temp = nt.findLongestPrefixMatch(nameABLPM);
  BOOST_CHECK_EQUAL(temp, nt.findExactMatch(nameAB));

  Name nameAELPM("/a/e/hello/world");
  temp = nt.findLongestPrefixMatch(nameAELPM);
  BOOST_CHECK_EQUAL(temp, nt.findExactMatch(nameAE));

  Name nameALPM("/a/hello/world");
  temp = nt.findLongestPrefixMatch(nameALPM);
  BOOST_CHECK_EQUAL(temp, nt.findExactMatch(nameA));

  Name nameFLPM("/f/hello/world");
  temp = nt.findLongestPrefixMatch(nameFLPM);
  BOOST_CHECK_EQUAL(temp, nt.findExactMatch(nameF));

  Name nameRootLPM("/does_not_exist");
  temp = nt.findLongestPrefixMatch(nameRootLPM);
  BOOST_CHECK_EQUAL(temp, nt.findExactMatch(nameRoot));

  // nt.dump(std::cout);

  bool eraseResult = false;
  temp = nt.findExactMatch(nameABC);
  if (static_cast<bool>(temp))
    eraseResult = nt.
  eraseEntryIfEmpty(temp);
  BOOST_CHECK_EQUAL(nt.size(), 6);
  BOOST_CHECK(!static_cast<bool>(nt.findExactMatch(nameABC)));
  BOOST_CHECK_EQUAL(eraseResult, true);

  eraseResult = false;
  temp = nt.findExactMatch(nameABCLPM);
  if (static_cast<bool>(temp)) 
    eraseResult = nt.
  eraseEntryIfEmpty(temp);
  BOOST_CHECK(!static_cast<bool>(temp));
  BOOST_CHECK_EQUAL(nt.size(), 6);
  BOOST_CHECK_EQUAL(eraseResult, false);

  // nt.dump(std::cout);

  nt.lookup(nameABC);
  BOOST_CHECK_EQUAL(nt.size(), 7);

  eraseResult = false;
  temp = nt.findExactMatch(nameABC);
  if (static_cast<bool>(temp)) 
    eraseResult = nt.
  eraseEntryIfEmpty(temp);
  BOOST_CHECK_EQUAL(nt.size(), 6);
  BOOST_CHECK_EQUAL(eraseResult, true);
  BOOST_CHECK(!static_cast<bool>(nt.findExactMatch(nameABC)));

  // nt.dump(std::cout);

  BOOST_CHECK_EQUAL(nt.getNBuckets(), 16);

  // should resize now 
  Name nameABCD("a/b/c/d");
  nt.lookup(nameABCD);
  Name nameABCDE("a/b/c/d/e");
  nt.lookup(nameABCDE);
  BOOST_CHECK_EQUAL(nt.size(), 9);
  BOOST_CHECK_EQUAL(nt.getNBuckets(), 32);

  // nt.dump(std::cout);

  // try to erase /a/b/c, should return false 
  temp = nt.findExactMatch(nameABC);
  BOOST_CHECK_EQUAL(temp->getPrefix(), nameABC);
  eraseResult = nt.
  eraseEntryIfEmpty(temp);
  BOOST_CHECK_EQUAL(eraseResult, false);
  temp = nt.findExactMatch(nameABC);
  BOOST_CHECK_EQUAL(temp->getPrefix(), nameABC);

  temp = nt.findExactMatch(nameABD);
  if (static_cast<bool>(temp)) 
    nt.
  eraseEntryIfEmpty(temp);
  BOOST_CHECK_EQUAL(nt.size(), 8);
  
  // nt.dump(std::cout);

  shared_ptr<std::vector<shared_ptr<name_tree::Entry> > > fullList;
  fullList = nt.fullEnumerate();
  BOOST_CHECK_EQUAL(fullList->size(), 8);
  // for (size_t j = 0; j < (*fullList).size(); j++)
  // {
  //  temp = (*fullList)[j];
  //  std::cout << temp->getPrefix().toUri() << std::endl;
  // }

  shared_ptr<std::vector<shared_ptr<name_tree::Entry> > > partialList;
  partialList = nt.partialEnumerate(nameA);
  BOOST_CHECK_EQUAL(partialList->size(), 6);
  // for (size_t j = 0; j < (*partialList).size(); j++)
  // {
  //  temp = (*partialList)[j];
  //  std::cout << temp->getPrefix().toUri() << std::endl;
  // }

  partialList = nt.partialEnumerate(nameRoot);
  BOOST_CHECK_EQUAL(partialList->size(), 8);
  // for (size_t j = 0; j < (*partialList).size(); j++)
  // {
  //  temp = (*partialList)[j];
  //  std::cout << temp->getPrefix().toUri() << std::endl;
  // }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd



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
 **/

#include "table/name-tree.hpp"
#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

using name_tree::Entry;

BOOST_FIXTURE_TEST_SUITE(TableNameTree, BaseFixture)

BOOST_AUTO_TEST_CASE(Hash)
{
  Name root("/");
  root.wireEncode();
  size_t hashValue = name_tree::computeHash(root);
  BOOST_CHECK_EQUAL(hashValue, static_cast<size_t>(0));

  Name prefix("/nohello/world/ndn/research");
  prefix.wireEncode();
  std::vector<size_t> hashSet = name_tree::computeHashSet(prefix);
  BOOST_CHECK_EQUAL(hashSet.size(), prefix.size() + 1);
}

BOOST_AUTO_TEST_CASE(Entry)
{
  Name prefix("ndn:/named-data/research/abc/def/ghi");

  shared_ptr<name_tree::Entry> npe = make_shared<name_tree::Entry>(prefix);
  BOOST_CHECK_EQUAL(npe->getPrefix(), prefix);

  // examine all the get methods

  size_t hash = npe->getHash();
  BOOST_CHECK_EQUAL(hash, static_cast<size_t>(0));

  shared_ptr<name_tree::Entry> parent = npe->getParent();
  BOOST_CHECK(!static_cast<bool>(parent));

  std::vector<shared_ptr<name_tree::Entry> >& childList = npe->getChildren();
  BOOST_CHECK_EQUAL(childList.size(), static_cast<size_t>(0));

  shared_ptr<fib::Entry> fib = npe->getFibEntry();
  BOOST_CHECK(!static_cast<bool>(fib));

  const std::vector< shared_ptr<pit::Entry> >& pitList = npe->getPitEntries();
  BOOST_CHECK_EQUAL(pitList.size(), static_cast<size_t>(0));

  // examine all the set method

  npe->setHash(static_cast<size_t>(12345));
  BOOST_CHECK_EQUAL(npe->getHash(), static_cast<size_t>(12345));

  Name parentName("ndn:/named-data/research/abc/def");
  parent = make_shared<name_tree::Entry>(parentName);
  npe->setParent(parent);
  BOOST_CHECK_EQUAL(npe->getParent(), parent);

  // Insert FIB

  shared_ptr<fib::Entry> fibEntry(new fib::Entry(prefix));
  shared_ptr<fib::Entry> fibEntryParent(new fib::Entry(parentName));

  npe->setFibEntry(fibEntry);
  BOOST_CHECK_EQUAL(npe->getFibEntry(), fibEntry);

  npe->setFibEntry(shared_ptr<fib::Entry>());
  BOOST_CHECK(!static_cast<bool>(npe->getFibEntry()));

  // Insert a PIT

  shared_ptr<pit::Entry> PitEntry(make_shared<pit::Entry>(prefix));
  shared_ptr<pit::Entry> PitEntry2(make_shared<pit::Entry>(parentName));

  npe->insertPitEntry(PitEntry);
  BOOST_CHECK_EQUAL(npe->getPitEntries().size(), 1);

  npe->insertPitEntry(PitEntry2);
  BOOST_CHECK_EQUAL(npe->getPitEntries().size(), 2);

  npe->erasePitEntry(PitEntry);
  BOOST_CHECK_EQUAL(npe->getPitEntries().size(), 1);

  npe->erasePitEntry(PitEntry2);
  BOOST_CHECK_EQUAL(npe->getPitEntries().size(), 0);
}

BOOST_AUTO_TEST_CASE(NameTreeBasic)
{
  size_t nBuckets = 16;
  NameTree nt(nBuckets);

  BOOST_CHECK_EQUAL(nt.size(), 0);
  BOOST_CHECK_EQUAL(nt.getNBuckets(), nBuckets);

  Name nameABC("ndn:/a/b/c");
  shared_ptr<name_tree::Entry> npeABC = nt.lookup(nameABC);
  BOOST_CHECK_EQUAL(nt.size(), 4);

  Name nameABD("/a/b/d");
  shared_ptr<name_tree::Entry> npeABD = nt.lookup(nameABD);
  BOOST_CHECK_EQUAL(nt.size(), 5);

  Name nameAE("/a/e/");
  shared_ptr<name_tree::Entry> npeAE = nt.lookup(nameAE);
  BOOST_CHECK_EQUAL(nt.size(), 6);

  Name nameF("/f");
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

  Name name0("/does/not/exist");
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

  BOOST_CHECK_EQUAL(nt.getNBuckets(), 16);

  // should resize now
  Name nameABCD("a/b/c/d");
  nt.lookup(nameABCD);
  Name nameABCDE("a/b/c/d/e");
  nt.lookup(nameABCDE);
  BOOST_CHECK_EQUAL(nt.size(), 9);
  BOOST_CHECK_EQUAL(nt.getNBuckets(), 32);

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
}

BOOST_AUTO_TEST_CASE(NameTreeIteratorFullEnumerate)
{
  size_t nBuckets = 1024;
  NameTree nt(nBuckets);

  BOOST_CHECK_EQUAL(nt.size(), 0);
  BOOST_CHECK_EQUAL(nt.getNBuckets(), nBuckets);

  Name nameABC("ndn:/a/b/c");
  shared_ptr<name_tree::Entry> npeABC = nt.lookup(nameABC);
  BOOST_CHECK_EQUAL(nt.size(), 4);

  Name nameABD("/a/b/d");
  shared_ptr<name_tree::Entry> npeABD = nt.lookup(nameABD);
  BOOST_CHECK_EQUAL(nt.size(), 5);

  Name nameAE("/a/e/");
  shared_ptr<name_tree::Entry> npeAE = nt.lookup(nameAE);
  BOOST_CHECK_EQUAL(nt.size(), 6);

  Name nameF("/f");
  shared_ptr<name_tree::Entry> npeF = nt.lookup(nameF);
  BOOST_CHECK_EQUAL(nt.size(), 7);

  Name nameRoot("/");
  shared_ptr<name_tree::Entry> npeRoot = nt.lookup(nameRoot);
  BOOST_CHECK_EQUAL(nt.size(), 7);

  Name nameA("/a");
  Name nameAB("/a/b");

  bool hasRoot  = false;
  bool hasA     = false;
  bool hasAB    = false;
  bool hasABC   = false;
  bool hasABD   = false;
  bool hasAE    = false;
  bool hasF     = false;

  int counter = 0;
  NameTree::const_iterator it = nt.fullEnumerate();

  for(; it != nt.end(); it++)
  {
    counter++;

    if (it->getPrefix() == nameRoot)
      hasRoot = true;
    if (it->getPrefix() == nameA)
      hasA    = true;
    if (it->getPrefix() == nameAB)
      hasAB   = true;
    if (it->getPrefix() == nameABC)
      hasABC  = true;
    if (it->getPrefix() == nameABD)
      hasABD  = true;
    if (it->getPrefix() == nameAE)
      hasAE   = true;
    if (it->getPrefix() == nameF)
      hasF    = true;
  }

  BOOST_CHECK_EQUAL(hasRoot , true);
  BOOST_CHECK_EQUAL(hasA    , true);
  BOOST_CHECK_EQUAL(hasAB   , true);
  BOOST_CHECK_EQUAL(hasABC  , true);
  BOOST_CHECK_EQUAL(hasABD  , true);
  BOOST_CHECK_EQUAL(hasAE   , true);
  BOOST_CHECK_EQUAL(hasF    , true);

  BOOST_CHECK_EQUAL(counter , 7);
}

// Predicates for testing the partial enumerate function

static inline std::pair<bool, bool>
predicate_NameTreeEntry_NameA_Only(const name_tree::Entry& entry)
{
  Name nameA("/a");
  bool first = nameA.equals(entry.getPrefix());
  return std::make_pair(first, true);
}

static inline std::pair<bool, bool>
predicate_NameTreeEntry_Except_NameA(const name_tree::Entry& entry)
{
  Name nameA("/a");
  bool first = !(nameA.equals(entry.getPrefix()));
  return std::make_pair(first, true);
}

static inline std::pair<bool, bool>
predicate_NameTreeEntry_NoSubNameA(const name_tree::Entry& entry)
{
  Name nameA("/a");
  bool second = !(nameA.equals(entry.getPrefix()));
  return std::make_pair(true, second);
}

static inline std::pair<bool, bool>
predicate_NameTreeEntry_NoNameA_NoSubNameAB(const name_tree::Entry& entry)
{
  Name nameA("/a");
  Name nameAB("/a/b");
  bool first = !(nameA.equals(entry.getPrefix()));
  bool second = !(nameAB.equals(entry.getPrefix()));
  return std::make_pair(first, second);
}

static inline std::pair<bool, bool>
predicate_NameTreeEntry_NoNameA_NoSubNameAC(const name_tree::Entry& entry)
{
  Name nameA("/a");
  Name nameAC("/a/c");
  bool first = !(nameA.equals(entry.getPrefix()));
  bool second = !(nameAC.equals(entry.getPrefix()));
  return std::make_pair(first, second);
}

static inline std::pair<bool, bool>
predicate_NameTreeEntry_Example(const name_tree::Entry& entry)
{
  Name nameRoot("/");
  Name nameA("/a");
  Name nameAB("/a/b");
  Name nameABC("/a/b/c");
  Name nameAD("/a/d");
  Name nameE("/e");
  Name nameF("/f");

  bool first = false;
  bool second = false;

  Name name = entry.getPrefix();

  if (name == nameRoot || name == nameAB || name == nameABC || name == nameAD)
  {
    first = true;
  }

  if(name == nameRoot || name == nameA || name == nameF)
  {
    second = true;
  }

  return std::make_pair(first, second);
}

BOOST_AUTO_TEST_CASE(NameTreeIteratorPartialEnumerate)
{
  typedef NameTree::const_iterator const_iterator;

  size_t nBuckets = 16;
  NameTree nameTree(nBuckets);
  int counter = 0;

  // empty nameTree, should return end();
  Name nameA("/a");
  bool hasA = false;
  const_iterator itA = nameTree.partialEnumerate(nameA);
  BOOST_CHECK(itA == nameTree.end());

  // We have "/", "/a", "/a/b", "a/c" now.
  Name nameAB("/a/b");
  bool hasAB = false;
  nameTree.lookup(nameAB);

  Name nameAC("/a/c");
  bool hasAC = false;
  nameTree.lookup(nameAC);
  BOOST_CHECK_EQUAL(nameTree.size(), 4);

  // Enumerate on some name that is not in nameTree
  Name name0="/0";
  const_iterator it0 = nameTree.partialEnumerate(name0);
  BOOST_CHECK(it0 == nameTree.end());

  // Accept "root" nameA only
  const_iterator itAOnly = nameTree.partialEnumerate(nameA, &predicate_NameTreeEntry_NameA_Only);
  BOOST_CHECK_EQUAL(itAOnly->getPrefix(), nameA);
  BOOST_CHECK(++itAOnly == nameTree.end());

  // Accept anything except "root" nameA
  const_iterator itExceptA = nameTree.partialEnumerate(nameA, &predicate_NameTreeEntry_Except_NameA);
  hasA = false;
  hasAB = false;
  hasAC = false;

  counter = 0;
  for (; itExceptA != nameTree.end(); itExceptA++)
  {
    counter++;

    if (itExceptA->getPrefix() == nameA)
      hasA  = true;

    if (itExceptA->getPrefix() == nameAB)
      hasAB = true;

    if (itExceptA->getPrefix() == nameAC)
      hasAC = true;
  }
  BOOST_CHECK_EQUAL(hasA,  false);
  BOOST_CHECK_EQUAL(hasAB, true);
  BOOST_CHECK_EQUAL(hasAC, true);

  BOOST_CHECK_EQUAL(counter, 2);

  Name nameAB1("a/b/1");
  bool hasAB1 = false;
  nameTree.lookup(nameAB1);

  Name nameAB2("a/b/2");
  bool hasAB2 = false;
  nameTree.lookup(nameAB2);

  Name nameAC1("a/c/1");
  bool hasAC1 = false;
  nameTree.lookup(nameAC1);

  Name nameAC2("a/c/2");
  bool hasAC2 = false;
  nameTree.lookup(nameAC2);

  BOOST_CHECK_EQUAL(nameTree.size(), 8);

  // No NameA
  // No SubTree from NameAB
  const_iterator itNoNameANoSubNameAB = nameTree.partialEnumerate(nameA, &predicate_NameTreeEntry_NoNameA_NoSubNameAB);
  hasA   = false;
  hasAB  = false;
  hasAB1 = false;
  hasAB2 = false;
  hasAC  = false;
  hasAC1 = false;
  hasAC2 = false;

  counter = 0;

  for (; itNoNameANoSubNameAB != nameTree.end(); itNoNameANoSubNameAB++)
  {
    counter++;

    if (itNoNameANoSubNameAB->getPrefix() == nameA)
      hasA   = true;

    if (itNoNameANoSubNameAB->getPrefix() == nameAB)
      hasAB  = true;

    if (itNoNameANoSubNameAB->getPrefix() == nameAB1)
      hasAB1 = true;

    if (itNoNameANoSubNameAB->getPrefix() == nameAB2)
      hasAB2 = true;

    if (itNoNameANoSubNameAB->getPrefix() == nameAC)
      hasAC  = true;

    if (itNoNameANoSubNameAB->getPrefix() == nameAC1)
      hasAC1 = true;

    if (itNoNameANoSubNameAB->getPrefix() == nameAC2)
      hasAC2 = true;
  }

  BOOST_CHECK_EQUAL(hasA,   false);
  BOOST_CHECK_EQUAL(hasAB,  true);
  BOOST_CHECK_EQUAL(hasAB1, false);
  BOOST_CHECK_EQUAL(hasAB2, false);
  BOOST_CHECK_EQUAL(hasAC,  true);
  BOOST_CHECK_EQUAL(hasAC1, true);
  BOOST_CHECK_EQUAL(hasAC2, true);

  BOOST_CHECK_EQUAL(counter, 4);

  // No NameA
  // No SubTree from NameAC
  const_iterator itNoNameANoSubNameAC = nameTree.partialEnumerate(nameA, &predicate_NameTreeEntry_NoNameA_NoSubNameAC);
  hasA   = false;
  hasAB  = false;
  hasAB1 = false;
  hasAB2 = false;
  hasAC  = false;
  hasAC1 = false;
  hasAC2 = false;

  counter = 0;

  for (; itNoNameANoSubNameAC != nameTree.end(); itNoNameANoSubNameAC++)
  {
    counter++;

    if (itNoNameANoSubNameAC->getPrefix() == nameA)
      hasA   = true;

    if (itNoNameANoSubNameAC->getPrefix() == nameAB)
      hasAB  = true;

    if (itNoNameANoSubNameAC->getPrefix() == nameAB1)
      hasAB1 = true;

    if (itNoNameANoSubNameAC->getPrefix() == nameAB2)
      hasAB2 = true;

    if (itNoNameANoSubNameAC->getPrefix() == nameAC)
      hasAC  = true;

    if (itNoNameANoSubNameAC->getPrefix() == nameAC1)
      hasAC1 = true;

    if (itNoNameANoSubNameAC->getPrefix() == nameAC2)
      hasAC2 = true;
  }

  BOOST_CHECK_EQUAL(hasA,   false);
  BOOST_CHECK_EQUAL(hasAB,  true);
  BOOST_CHECK_EQUAL(hasAB1, true);
  BOOST_CHECK_EQUAL(hasAB2, true);
  BOOST_CHECK_EQUAL(hasAC,  true);
  BOOST_CHECK_EQUAL(hasAC1, false);
  BOOST_CHECK_EQUAL(hasAC2, false);

  BOOST_CHECK_EQUAL(counter, 4);

  // No Subtree from NameA
  const_iterator itNoANoSubNameA = nameTree.partialEnumerate(nameA, &predicate_NameTreeEntry_NoSubNameA);

  hasA   = false;
  hasAB  = false;
  hasAB1 = false;
  hasAB2 = false;
  hasAC  = false;
  hasAC1 = false;
  hasAC2 = false;

  counter = 0;

  for (; itNoANoSubNameA != nameTree.end(); itNoANoSubNameA++)
  {
    counter++;

    if (itNoANoSubNameA->getPrefix() == nameA)
      hasA   = true;

    if (itNoANoSubNameA->getPrefix() == nameAB)
      hasAB  = true;

    if (itNoANoSubNameA->getPrefix() == nameAB1)
      hasAB1 = true;

    if (itNoANoSubNameA->getPrefix() == nameAB2)
      hasAB2 = true;

    if (itNoANoSubNameA->getPrefix() == nameAC)
      hasAC  = true;

    if (itNoANoSubNameA->getPrefix() == nameAC1)
      hasAC1 = true;

    if (itNoANoSubNameA->getPrefix() == nameAC2)
      hasAC2 = true;
  }

  BOOST_CHECK_EQUAL(hasA,   true);
  BOOST_CHECK_EQUAL(hasAB,  false);
  BOOST_CHECK_EQUAL(hasAB1, false);
  BOOST_CHECK_EQUAL(hasAB2, false);
  BOOST_CHECK_EQUAL(hasAC,  false);
  BOOST_CHECK_EQUAL(hasAC1, false);
  BOOST_CHECK_EQUAL(hasAC2, false);

  BOOST_CHECK_EQUAL(counter, 1);

  // Example
  // /
  // /A
  // /A/B x
  // /A/B/C
  // /A/D x
  // /E
  // /F

  NameTree nameTreeExample(nBuckets);

  Name nameRoot("/");
  bool hasRoot = false;

  nameTreeExample.lookup(nameA);
  hasA = false;
  nameTreeExample.lookup(nameAB);
  hasAB = false;

  Name nameABC("a/b/c");
  bool hasABC = false;
  nameTreeExample.lookup(nameABC);

  Name nameAD("/a/d");
  nameTreeExample.lookup(nameAD);
  bool hasAD = false;

  Name nameE("/e");
  nameTreeExample.lookup(nameE);
  bool hasE = false;

  Name nameF("/f");
  nameTreeExample.lookup(nameF);
  bool hasF = false;

  counter = 0;
  const_iterator itExample = nameTreeExample.partialEnumerate(nameA, &predicate_NameTreeEntry_Example);

  for (; itExample != nameTreeExample.end(); itExample++)
  {
    counter++;

    if (itExample->getPrefix() == nameRoot)
      hasRoot = true;

    if (itExample->getPrefix() == nameA)
     hasA     = true;

    if (itExample->getPrefix() == nameAB)
     hasAB    = true;

    if (itExample->getPrefix() == nameABC)
     hasABC   = true;

    if (itExample->getPrefix() == nameAD)
     hasAD    = true;

    if (itExample->getPrefix() == nameE)
     hasE     = true;

    if (itExample->getPrefix() == nameF)
      hasF    = true;
  }

  BOOST_CHECK_EQUAL(hasRoot, false);
  BOOST_CHECK_EQUAL(hasA,    false);
  BOOST_CHECK_EQUAL(hasAB,   true);
  BOOST_CHECK_EQUAL(hasABC,  false);
  BOOST_CHECK_EQUAL(hasAD,   true);
  BOOST_CHECK_EQUAL(hasE,    false);
  BOOST_CHECK_EQUAL(hasF,    false);

  BOOST_CHECK_EQUAL(counter, 2);
}


BOOST_AUTO_TEST_CASE(NameTreeIteratorFindAllMatches)
{
  size_t nBuckets = 16;
  NameTree nt(nBuckets);
  int counter = 0;

  Name nameABCDEF("a/b/c/d/e/f");
  shared_ptr<name_tree::Entry> npeABCDEF = nt.lookup(nameABCDEF);

  Name nameAAC("a/a/c");
  shared_ptr<name_tree::Entry> npeAAC = nt.lookup(nameAAC);

  Name nameAAD1("a/a/d/1");
  shared_ptr<name_tree::Entry> npeAAD1 = nt.lookup(nameAAD1);

  Name nameAAD2("a/a/d/2");
  shared_ptr<name_tree::Entry> npeAAD2 = nt.lookup(nameAAD2);


  BOOST_CHECK_EQUAL(nt.size(), 12);

  Name nameRoot ("/");
  Name nameA    ("/a");
  Name nameAB   ("/a/b");
  Name nameABC  ("/a/b/c");
  Name nameABCD ("/a/b/c/d");
  Name nameABCDE("/a/b/c/d/e");
  Name nameAA   ("/a/a");
  Name nameAAD  ("/a/a/d");

  bool hasRoot    = false;
  bool hasA       = false;
  bool hasAB      = false;
  bool hasABC     = false;
  bool hasABCD    = false;
  bool hasABCDE   = false;
  bool hasABCDEF  = false;
  bool hasAA      = false;
  bool hasAAC     = false;
  bool hasAAD     = false;
  bool hasAAD1    = false;
  bool hasAAD2    = false;

  counter = 0;

  for (NameTree::const_iterator it = nt.findAllMatches(nameABCDEF);
      it != nt.end(); it++)
  {
    counter++;

    if (it->getPrefix() == nameRoot)
      hasRoot   = true;
    if (it->getPrefix() == nameA)
      hasA      = true;
    if (it->getPrefix() == nameAB)
      hasAB     = true;
    if (it->getPrefix() == nameABC)
      hasABC    = true;
    if (it->getPrefix() == nameABCD)
      hasABCD   = true;
    if (it->getPrefix() == nameABCDE)
      hasABCDE  = true;
    if (it->getPrefix() == nameABCDEF)
      hasABCDEF = true;
    if (it->getPrefix() == nameAA)
      hasAA     = true;
    if (it->getPrefix() == nameAAC)
      hasAAC    = true;
    if (it->getPrefix() == nameAAD)
      hasAAD    = true;
    if (it->getPrefix() == nameAAD1)
      hasAAD1   = true;
    if (it->getPrefix() == nameAAD2)
      hasAAD2   = true;
  }

  BOOST_CHECK_EQUAL(hasRoot   , true);
  BOOST_CHECK_EQUAL(hasA      , true);
  BOOST_CHECK_EQUAL(hasAB     , true);
  BOOST_CHECK_EQUAL(hasABC    , true);
  BOOST_CHECK_EQUAL(hasABCD   , true);
  BOOST_CHECK_EQUAL(hasABCDE  , true);
  BOOST_CHECK_EQUAL(hasABCDEF , true);
  BOOST_CHECK_EQUAL(hasAA     , false);
  BOOST_CHECK_EQUAL(hasAAC    , false);
  BOOST_CHECK_EQUAL(hasAAD    , false);
  BOOST_CHECK_EQUAL(hasAAD1   , false);
  BOOST_CHECK_EQUAL(hasAAD2   , false);

  BOOST_CHECK_EQUAL(counter, 7);
}

BOOST_AUTO_TEST_CASE(NameTreeHashTableResizeShrink)
{
  size_t nBuckets = 16;
  NameTree nameTree(nBuckets);

  Name prefix("/a/b/c/d/e/f/g/h"); // requires 9 buckets

  shared_ptr<name_tree::Entry> entry = nameTree.lookup(prefix);
  BOOST_CHECK_EQUAL(nameTree.size(), 9);
  BOOST_CHECK_EQUAL(nameTree.getNBuckets(), 32);

  nameTree.eraseEntryIfEmpty(entry);
  BOOST_CHECK_EQUAL(nameTree.size(), 0);
  BOOST_CHECK_EQUAL(nameTree.getNBuckets(), 16);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd

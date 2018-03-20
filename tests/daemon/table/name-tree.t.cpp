/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
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
 */

#include "table/name-tree.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace name_tree {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestNameTree, BaseFixture)

BOOST_AUTO_TEST_CASE(ComputeHash)
{
  Name root("/");
  root.wireEncode();
  HashValue hashValue = computeHash(root);
  BOOST_CHECK_EQUAL(hashValue, 0);

  Name prefix("/nohello/world/ndn/research");
  prefix.wireEncode();
  HashSequence hashes = computeHashes(prefix);
  BOOST_CHECK_EQUAL(hashes.size(), prefix.size() + 1);

  hashes = computeHashes(prefix, 2);
  BOOST_CHECK_EQUAL(hashes.size(), 3);
}

BOOST_AUTO_TEST_SUITE(Hashtable)
using name_tree::Hashtable;

BOOST_AUTO_TEST_CASE(Modifiers)
{
  Hashtable ht(HashtableOptions(16));

  Name name("/A/B/C/D");
  HashSequence hashes = computeHashes(name);

  BOOST_CHECK_EQUAL(ht.size(), 0);
  BOOST_CHECK(ht.find(name, 2) == nullptr);

  const Node* node = nullptr;
  bool isNew = false;
  std::tie(node, isNew) = ht.insert(name, 2, hashes);
  BOOST_CHECK_EQUAL(isNew, true);
  BOOST_CHECK(node != nullptr);
  BOOST_CHECK_EQUAL(ht.size(), 1);
  BOOST_CHECK_EQUAL(ht.find(name, 2), node);
  BOOST_CHECK_EQUAL(ht.find(name, 2, hashes), node);

  BOOST_CHECK(ht.find(name, 0) == nullptr);
  BOOST_CHECK(ht.find(name, 1) == nullptr);
  BOOST_CHECK(ht.find(name, 3) == nullptr);
  BOOST_CHECK(ht.find(name, 4) == nullptr);

  const Node* node2 = nullptr;
  std::tie(node2, isNew) = ht.insert(name, 2, hashes);
  BOOST_CHECK_EQUAL(isNew, false);
  BOOST_CHECK_EQUAL(node2, node);
  BOOST_CHECK_EQUAL(ht.size(), 1);

  std::tie(node2, isNew) = ht.insert(name, 4, hashes);
  BOOST_CHECK_EQUAL(isNew, true);
  BOOST_CHECK(node2 != nullptr);
  BOOST_CHECK_NE(node2, node);
  BOOST_CHECK_EQUAL(ht.size(), 2);

  ht.erase(const_cast<Node*>(node2));
  BOOST_CHECK_EQUAL(ht.size(), 1);
  BOOST_CHECK(ht.find(name, 4) == nullptr);
  BOOST_CHECK_EQUAL(ht.find(name, 2), node);

  ht.erase(const_cast<Node*>(node));
  BOOST_CHECK_EQUAL(ht.size(), 0);
  BOOST_CHECK(ht.find(name, 2) == nullptr);
  BOOST_CHECK(ht.find(name, 4) == nullptr);
}

BOOST_AUTO_TEST_CASE(Resize)
{
  HashtableOptions options(9);
  BOOST_CHECK_EQUAL(options.initialSize, 9);
  BOOST_CHECK_EQUAL(options.minSize, 9);
  options.minSize = 6;
  options.expandLoadFactor = 0.80;
  options.expandFactor = 5.0;
  options.shrinkLoadFactor = 0.12;
  options.shrinkFactor = 0.3;

  Hashtable ht(options);

  auto addNodes = [&ht] (int min, int max) {
    for (int i = min; i <= max; ++i) {
      Name name;
      name.appendNumber(i);
      HashSequence hashes = computeHashes(name);
      ht.insert(name, name.size(), hashes);
    }
  };

  auto removeNodes = [&ht] (int min, int max) {
    for (int i = min; i <= max; ++i) {
      Name name;
      name.appendNumber(i);
      const Node* node = ht.find(name, name.size());
      BOOST_REQUIRE(node != nullptr);
      ht.erase(const_cast<Node*>(node));
    }
  };

  BOOST_CHECK_EQUAL(ht.size(), 0);
  BOOST_CHECK_EQUAL(ht.getNBuckets(), 9);

  addNodes(1, 1);
  BOOST_CHECK_EQUAL(ht.size(), 1);
  BOOST_CHECK_EQUAL(ht.getNBuckets(), 9);

  removeNodes(1, 1);
  BOOST_CHECK_EQUAL(ht.size(), 0);
  BOOST_CHECK_EQUAL(ht.getNBuckets(), 6);

  addNodes(1, 4);
  BOOST_CHECK_EQUAL(ht.size(), 4);
  BOOST_CHECK_EQUAL(ht.getNBuckets(), 6);

  addNodes(5, 5);
  BOOST_CHECK_EQUAL(ht.size(), 5);
  BOOST_CHECK_EQUAL(ht.getNBuckets(), 30);

  addNodes(6, 23);
  BOOST_CHECK_EQUAL(ht.size(), 23);
  BOOST_CHECK_EQUAL(ht.getNBuckets(), 30);

  addNodes(24, 25);
  BOOST_CHECK_EQUAL(ht.size(), 25);
  BOOST_CHECK_EQUAL(ht.getNBuckets(), 150);

  removeNodes(19, 25);
  BOOST_CHECK_EQUAL(ht.size(), 18);
  BOOST_CHECK_EQUAL(ht.getNBuckets(), 150);

  removeNodes(17, 18);
  BOOST_CHECK_EQUAL(ht.size(), 16);
  BOOST_CHECK_EQUAL(ht.getNBuckets(), 45);

  removeNodes(7, 16);
  BOOST_CHECK_EQUAL(ht.size(), 6);
  BOOST_CHECK_EQUAL(ht.getNBuckets(), 45);

  removeNodes(5, 6);
  BOOST_CHECK_EQUAL(ht.size(), 4);
  BOOST_CHECK_EQUAL(ht.getNBuckets(), 13);

  removeNodes(1, 4);
  BOOST_CHECK_EQUAL(ht.size(), 0);
  BOOST_CHECK_EQUAL(ht.getNBuckets(), 6);
}

BOOST_AUTO_TEST_SUITE_END() // Hashtable

BOOST_AUTO_TEST_SUITE(TestEntry)

BOOST_AUTO_TEST_CASE(TreeRelation)
{
  Name name("ndn:/named-data/research/abc/def/ghi");
  auto node = make_unique<Node>(0, name);
  Entry& npe = node->entry;
  BOOST_CHECK(npe.getParent() == nullptr);

  Name parentName = name.getPrefix(-1);
  auto parentNode = make_unique<Node>(1, parentName);
  Entry& parent = parentNode->entry;
  BOOST_CHECK_EQUAL(parent.hasChildren(), false);
  BOOST_CHECK_EQUAL(parent.isEmpty(), true);

  npe.setParent(parentNode->entry);
  BOOST_CHECK_EQUAL(npe.getParent(), &parent);
  BOOST_CHECK_EQUAL(parent.hasChildren(), true);
  BOOST_CHECK_EQUAL(parent.isEmpty(), false);
  BOOST_REQUIRE_EQUAL(parent.getChildren().size(), 1);
  BOOST_CHECK_EQUAL(parent.getChildren().front(), &npe);

  npe.unsetParent();
  BOOST_CHECK(npe.getParent() == nullptr);
  BOOST_CHECK_EQUAL(parent.hasChildren(), false);
  BOOST_CHECK_EQUAL(parent.isEmpty(), true);
}

BOOST_AUTO_TEST_CASE(TableEntries)
{
  Name name("ndn:/named-data/research/abc/def/ghi");
  Node node(0, name);
  Entry& npe = node.entry;
  BOOST_CHECK_EQUAL(npe.getName(), name);

  BOOST_CHECK_EQUAL(npe.hasTableEntries(), false);
  BOOST_CHECK_EQUAL(npe.isEmpty(), true);
  BOOST_CHECK(npe.getFibEntry() == nullptr);
  BOOST_CHECK_EQUAL(npe.hasPitEntries(), false);
  BOOST_CHECK_EQUAL(npe.getPitEntries().empty(), true);
  BOOST_CHECK(npe.getMeasurementsEntry() == nullptr);
  BOOST_CHECK(npe.getStrategyChoiceEntry() == nullptr);

  npe.setFibEntry(make_unique<fib::Entry>(name));
  BOOST_REQUIRE(npe.getFibEntry() != nullptr);
  BOOST_CHECK_EQUAL(npe.getFibEntry()->getPrefix(), name);
  BOOST_CHECK_EQUAL(npe.hasTableEntries(), true);
  BOOST_CHECK_EQUAL(npe.isEmpty(), false);

  npe.setFibEntry(nullptr);
  BOOST_CHECK(npe.getFibEntry() == nullptr);
  BOOST_CHECK_EQUAL(npe.hasTableEntries(), false);
  BOOST_CHECK_EQUAL(npe.isEmpty(), true);

  auto pit1 = make_shared<pit::Entry>(*makeInterest(name));
  shared_ptr<Interest> interest2 = makeInterest(name);
  interest2->setMinSuffixComponents(2);
  auto pit2 = make_shared<pit::Entry>(*interest2);

  npe.insertPitEntry(pit1);
  BOOST_CHECK_EQUAL(npe.hasPitEntries(), true);
  BOOST_CHECK_EQUAL(npe.getPitEntries().size(), 1);
  BOOST_CHECK_EQUAL(npe.hasTableEntries(), true);
  BOOST_CHECK_EQUAL(npe.isEmpty(), false);

  npe.insertPitEntry(pit2);
  BOOST_CHECK_EQUAL(npe.getPitEntries().size(), 2);

  pit::Entry* pit1ptr = pit1.get();
  weak_ptr<pit::Entry> pit1weak(pit1);
  pit1.reset();
  BOOST_CHECK_EQUAL(pit1weak.use_count(), 1); // npe is the sole owner of pit1
  npe.erasePitEntry(pit1ptr);
  BOOST_REQUIRE_EQUAL(npe.getPitEntries().size(), 1);
  BOOST_CHECK_EQUAL(npe.getPitEntries().front()->getInterest(), *interest2);

  npe.erasePitEntry(pit2.get());
  BOOST_CHECK_EQUAL(npe.hasPitEntries(), false);
  BOOST_CHECK_EQUAL(npe.getPitEntries().size(), 0);
  BOOST_CHECK_EQUAL(npe.hasTableEntries(), false);
  BOOST_CHECK_EQUAL(npe.isEmpty(), true);

  npe.setMeasurementsEntry(make_unique<measurements::Entry>(name));
  BOOST_REQUIRE(npe.getMeasurementsEntry() != nullptr);
  BOOST_CHECK_EQUAL(npe.getMeasurementsEntry()->getName(), name);
  BOOST_CHECK_EQUAL(npe.hasTableEntries(), true);
  BOOST_CHECK_EQUAL(npe.isEmpty(), false);

  npe.setMeasurementsEntry(nullptr);
  BOOST_CHECK(npe.getMeasurementsEntry() == nullptr);
  BOOST_CHECK_EQUAL(npe.hasTableEntries(), false);
  BOOST_CHECK_EQUAL(npe.isEmpty(), true);

  npe.setStrategyChoiceEntry(make_unique<strategy_choice::Entry>(name));
  BOOST_REQUIRE(npe.getStrategyChoiceEntry() != nullptr);
  BOOST_CHECK_EQUAL(npe.getStrategyChoiceEntry()->getPrefix(), name);
  BOOST_CHECK_EQUAL(npe.hasTableEntries(), true);
  BOOST_CHECK_EQUAL(npe.isEmpty(), false);

  npe.setStrategyChoiceEntry(nullptr);
  BOOST_CHECK(npe.getStrategyChoiceEntry() == nullptr);
  BOOST_CHECK_EQUAL(npe.hasTableEntries(), false);
  BOOST_CHECK_EQUAL(npe.isEmpty(), true);
}

BOOST_AUTO_TEST_SUITE_END() // TestEntry

BOOST_AUTO_TEST_CASE(Basic)
{
  size_t nBuckets = 16;
  NameTree nt(nBuckets);

  BOOST_CHECK_EQUAL(nt.size(), 0);
  BOOST_CHECK_EQUAL(nt.getNBuckets(), nBuckets);

  // lookup

  Name nameABC("/a/b/c");
  Entry& npeABC = nt.lookup(nameABC);
  BOOST_CHECK_EQUAL(nt.size(), 4);

  Name nameABD("/a/b/d");
  Entry& npeABD = nt.lookup(nameABD);
  BOOST_CHECK_EQUAL(nt.size(), 5);

  Name nameAE("/a/e/");
  Entry& npeAE = nt.lookup(nameAE);
  BOOST_CHECK_EQUAL(nt.size(), 6);

  Name nameF("/f");
  Entry& npeF = nt.lookup(nameF);
  BOOST_CHECK_EQUAL(nt.size(), 7);

  // getParent and findExactMatch

  Name nameAB("/a/b");
  BOOST_CHECK_EQUAL(npeABC.getParent(), nt.findExactMatch(nameAB));
  BOOST_CHECK_EQUAL(npeABD.getParent(), nt.findExactMatch(nameAB));

  Name nameA("/a");
  BOOST_CHECK_EQUAL(npeAE.getParent(), nt.findExactMatch(nameA));

  Name nameRoot("/");
  BOOST_CHECK_EQUAL(npeF.getParent(), nt.findExactMatch(nameRoot));
  BOOST_CHECK_EQUAL(nt.size(), 7);

  Name name0("/does/not/exist");
  Entry* npe0 = nt.findExactMatch(name0);
  BOOST_CHECK(npe0 == nullptr);


  // findLongestPrefixMatch

  Entry* temp = nullptr;
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
  if (temp != nullptr)
    eraseResult = nt.eraseIfEmpty(temp);
  BOOST_CHECK_EQUAL(nt.size(), 6);
  BOOST_CHECK(nt.findExactMatch(nameABC) == nullptr);
  BOOST_CHECK_EQUAL(eraseResult, true);

  eraseResult = false;
  temp = nt.findExactMatch(nameABCLPM);
  if (temp != nullptr)
    eraseResult = nt.eraseIfEmpty(temp);
  BOOST_CHECK(temp == nullptr);
  BOOST_CHECK_EQUAL(nt.size(), 6);
  BOOST_CHECK_EQUAL(eraseResult, false);

  nt.lookup(nameABC);
  BOOST_CHECK_EQUAL(nt.size(), 7);

  eraseResult = false;
  temp = nt.findExactMatch(nameABC);
  if (temp != nullptr)
    eraseResult = nt.eraseIfEmpty(temp);
  BOOST_CHECK_EQUAL(nt.size(), 6);
  BOOST_CHECK_EQUAL(eraseResult, true);
  BOOST_CHECK(nt.findExactMatch(nameABC) == nullptr);

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
  BOOST_CHECK_EQUAL(temp->getName(), nameABC);
  eraseResult = nt.eraseIfEmpty(temp);
  BOOST_CHECK_EQUAL(eraseResult, false);
  temp = nt.findExactMatch(nameABC);
  BOOST_CHECK_EQUAL(temp->getName(), nameABC);

  temp = nt.findExactMatch(nameABD);
  if (temp != nullptr)
    nt.eraseIfEmpty(temp);
  BOOST_CHECK_EQUAL(nt.size(), 8);
}

/** \brief verify a NameTree enumeration contains expected entries
 *
 *  Example:
 *  \code
 *  auto& enumerable = ...;
 *  EnumerationVerifier(enumerable)
 *    .expect("/A")
 *    .expect("/B")
 *    .end();
 *  // enumerable must have /A /B and nothing else to pass the test.
 *  \endcode
 */
class EnumerationVerifier : noncopyable
{
public:
  template<typename Enumerable>
  EnumerationVerifier(Enumerable&& enumerable)
  {
    for (const Entry& entry : enumerable) {
      const Name& name = entry.getName();
      BOOST_CHECK_MESSAGE(m_names.insert(name).second, "duplicate Name " << name);
    }
  }

  EnumerationVerifier&
  expect(const Name& name)
  {
    BOOST_CHECK_MESSAGE(m_names.erase(name) == 1, "missing Name " << name);
    return *this;
  }

  void
  end()
  {
    BOOST_CHECK(m_names.empty());
  }

private:
  std::unordered_set<Name> m_names;
};

class EnumerationFixture : public BaseFixture
{
protected:
  EnumerationFixture()
    : nt(N_BUCKETS)
  {
    BOOST_CHECK_EQUAL(nt.size(), 0);
    BOOST_CHECK_EQUAL(nt.getNBuckets(), N_BUCKETS);
  }

  void
  insertAbAc()
  {
    nt.lookup("/a/b");
    nt.lookup("/a/c");
    BOOST_CHECK_EQUAL(nt.size(), 4);
    // /, /a, /a/b, /a/c
  }

  void
  insertAb1Ab2Ac1Ac2()
  {
    nt.lookup("/a/b/1");
    nt.lookup("/a/b/2");
    nt.lookup("/a/c/1");
    nt.lookup("/a/c/2");
    BOOST_CHECK_EQUAL(nt.size(), 8);
    // /, /a, /a/b, /a/b/1, /a/b/2, /a/c, /a/c/1, /a/c/2
  }

protected:
  static const size_t N_BUCKETS = 16;
  NameTree nt;
};

const size_t EnumerationFixture::N_BUCKETS;

BOOST_FIXTURE_TEST_CASE(IteratorFullEnumerate, EnumerationFixture)
{
  nt.lookup("/a/b/c");
  BOOST_CHECK_EQUAL(nt.size(), 4);

  nt.lookup("/a/b/d");
  BOOST_CHECK_EQUAL(nt.size(), 5);

  nt.lookup("/a/e");
  BOOST_CHECK_EQUAL(nt.size(), 6);

  nt.lookup("/f");
  BOOST_CHECK_EQUAL(nt.size(), 7);

  nt.lookup("/");
  BOOST_CHECK_EQUAL(nt.size(), 7);

  auto&& enumerable = nt.fullEnumerate();
  EnumerationVerifier(enumerable)
    .expect("/")
    .expect("/a")
    .expect("/a/b")
    .expect("/a/b/c")
    .expect("/a/b/d")
    .expect("/a/e")
    .expect("/f")
    .end();
}

BOOST_FIXTURE_TEST_SUITE(IteratorPartialEnumerate, EnumerationFixture)

BOOST_AUTO_TEST_CASE(Empty)
{
  auto&& enumerable = nt.partialEnumerate("/a");

  EnumerationVerifier(enumerable)
    .end();
}

BOOST_AUTO_TEST_CASE(NotIn)
{
  this->insertAbAc();

  // Enumerate on some name that is not in nameTree
  Name name0("/0");
  auto&& enumerable = nt.partialEnumerate("/0");

  EnumerationVerifier(enumerable)
    .end();
}

BOOST_AUTO_TEST_CASE(OnlyA)
{
  this->insertAbAc();

  // Accept "root" nameA only
  auto&& enumerable = nt.partialEnumerate("/a", [] (const Entry& entry) {
    return std::make_pair(entry.getName() == "/a", true);
  });

  EnumerationVerifier(enumerable)
    .expect("/a")
    .end();
}

BOOST_AUTO_TEST_CASE(ExceptA)
{
  this->insertAbAc();

  // Accept anything except "root" nameA
  auto&& enumerable = nt.partialEnumerate("/a", [] (const Entry& entry) {
    return std::make_pair(entry.getName() != "/a", true);
  });

  EnumerationVerifier(enumerable)
    .expect("/a/b")
    .expect("/a/c")
    .end();
}

BOOST_AUTO_TEST_CASE(NoNameANoSubTreeAB)
{
  this->insertAb1Ab2Ac1Ac2();

  // No NameA
  // No SubTree from NameAB
  auto&& enumerable = nt.partialEnumerate("/a", [] (const Entry& entry) {
      return std::make_pair(entry.getName() != "/a", entry.getName() != "/a/b");
    });

  EnumerationVerifier(enumerable)
    .expect("/a/b")
    .expect("/a/c")
    .expect("/a/c/1")
    .expect("/a/c/2")
    .end();
}

BOOST_AUTO_TEST_CASE(NoNameANoSubTreeAC)
{
  this->insertAb1Ab2Ac1Ac2();

  // No NameA
  // No SubTree from NameAC
  auto&& enumerable = nt.partialEnumerate("/a", [] (const Entry& entry) {
      return std::make_pair(entry.getName() != "/a", entry.getName() != "/a/c");
    });

  EnumerationVerifier(enumerable)
    .expect("/a/b")
    .expect("/a/b/1")
    .expect("/a/b/2")
    .expect("/a/c")
    .end();
}

BOOST_AUTO_TEST_CASE(NoSubTreeA)
{
  this->insertAb1Ab2Ac1Ac2();

  // No Subtree from NameA
  auto&& enumerable = nt.partialEnumerate("/a", [] (const Entry& entry) {
      return std::make_pair(true, entry.getName() != "/a");
    });

  EnumerationVerifier(enumerable)
    .expect("/a")
    .end();
}

BOOST_AUTO_TEST_CASE(Example)
{
  // Example
  // /
  // /A
  // /A/B x
  // /A/B/C
  // /A/D x
  // /E
  // /F

  nt.lookup("/A");
  nt.lookup("/A/B");
  nt.lookup("/A/B/C");
  nt.lookup("/A/D");
  nt.lookup("/E");
  nt.lookup("/F");

  auto&& enumerable = nt.partialEnumerate("/A",
    [] (const Entry& entry) {
      bool visitEntry = false;
      bool visitChildren = false;

      Name name = entry.getName();

      if (name == "/" || name == "/A/B" || name == "/A/B/C" || name == "/A/D") {
        visitEntry = true;
      }

      if (name == "/" || name == "/A" || name == "/F") {
        visitChildren = true;
      }

      return std::make_pair(visitEntry, visitChildren);
    });

  EnumerationVerifier(enumerable)
    .expect("/A/B")
    .expect("/A/D")
    .end();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_CASE(IteratorFindAllMatches, EnumerationFixture)
{
  nt.lookup("/a/b/c/d/e/f");
  nt.lookup("/a/a/c");
  nt.lookup("/a/a/d/1");
  nt.lookup("/a/a/d/2");
  BOOST_CHECK_EQUAL(nt.size(), 12);

  auto&& allMatches = nt.findAllMatches("/a/b/c/d/e");

  EnumerationVerifier(allMatches)
    .expect("/")
    .expect("/a")
    .expect("/a/b")
    .expect("/a/b/c")
    .expect("/a/b/c/d")
    .expect("/a/b/c/d/e")
    .end();
}

BOOST_AUTO_TEST_CASE(HashTableResizeShrink)
{
  size_t nBuckets = 16;
  NameTree nameTree(nBuckets);

  Name prefix("/a/b/c/d/e/f/g/h"); // requires 9 buckets

  Entry& entry = nameTree.lookup(prefix);
  BOOST_CHECK_EQUAL(nameTree.size(), 9);
  BOOST_CHECK_EQUAL(nameTree.getNBuckets(), 32);

  nameTree.eraseIfEmpty(&entry);
  BOOST_CHECK_EQUAL(nameTree.size(), 0);
  BOOST_CHECK_EQUAL(nameTree.getNBuckets(), 16);
}

// .lookup should not invalidate iterator
BOOST_AUTO_TEST_CASE(SurvivedIteratorAfterLookup)
{
  NameTree nt;
  nt.lookup("/A/B/C");
  nt.lookup("/E");

  Name nameB("/A/B");
  std::set<Name> seenNames;
  for (NameTree::const_iterator it = nt.begin(); it != nt.end(); ++it) {
    BOOST_CHECK(seenNames.insert(it->getName()).second);
    if (it->getName() == nameB) {
      nt.lookup("/D");
    }
  }

  BOOST_CHECK_EQUAL(seenNames.count("/"), 1);
  BOOST_CHECK_EQUAL(seenNames.count("/A"), 1);
  BOOST_CHECK_EQUAL(seenNames.count("/A/B"), 1);
  BOOST_CHECK_EQUAL(seenNames.count("/A/B/C"), 1);
  BOOST_CHECK_EQUAL(seenNames.count("/E"), 1);

  seenNames.erase("/D"); // /D may or may not appear
  BOOST_CHECK(seenNames.size() == 5);
}

// .eraseIfEmpty should not invalidate iterator
BOOST_AUTO_TEST_CASE(SurvivedIteratorAfterErase)
{
  NameTree nt;
  nt.lookup("/A/B/C");
  nt.lookup("/A/D/E");
  nt.lookup("/A/F/G");
  nt.lookup("/H");

  Name nameD("/A/D");
  std::set<Name> seenNames;
  for (NameTree::const_iterator it = nt.begin(); it != nt.end(); ++it) {
    BOOST_CHECK(seenNames.insert(it->getName()).second);
    if (it->getName() == nameD) {
      nt.eraseIfEmpty(nt.findExactMatch("/A/F/G")); // /A/F/G and /A/F are erased
    }
  }

  BOOST_CHECK_EQUAL(seenNames.count("/"), 1);
  BOOST_CHECK_EQUAL(seenNames.count("/A"), 1);
  BOOST_CHECK_EQUAL(seenNames.count("/A/B"), 1);
  BOOST_CHECK_EQUAL(seenNames.count("/A/B/C"), 1);
  BOOST_CHECK_EQUAL(seenNames.count("/A/D"), 1);
  BOOST_CHECK_EQUAL(seenNames.count("/A/D/E"), 1);
  BOOST_CHECK_EQUAL(seenNames.count("/H"), 1);

  seenNames.erase("/A/F"); // /A/F may or may not appear
  seenNames.erase("/A/F/G"); // /A/F/G may or may not appear
  BOOST_CHECK(seenNames.size() == 7);
}

BOOST_AUTO_TEST_SUITE_END() // TestNameTree
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace name_tree
} // namespace nfd

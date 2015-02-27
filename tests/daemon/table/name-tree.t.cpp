/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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
#include <unordered_set>

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

  shared_ptr<pit::Entry> pitEntry(make_shared<pit::Entry>(*makeInterest(prefix)));
  shared_ptr<pit::Entry> pitEntry2(make_shared<pit::Entry>(*makeInterest(parentName)));

  npe->insertPitEntry(pitEntry);
  BOOST_CHECK_EQUAL(npe->getPitEntries().size(), 1);

  npe->insertPitEntry(pitEntry2);
  BOOST_CHECK_EQUAL(npe->getPitEntries().size(), 2);

  npe->erasePitEntry(pitEntry);
  BOOST_CHECK_EQUAL(npe->getPitEntries().size(), 1);

  npe->erasePitEntry(pitEntry2);
  BOOST_CHECK_EQUAL(npe->getPitEntries().size(), 0);
}

BOOST_AUTO_TEST_CASE(Basic)
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

/** \brief verify a NameTree enumeration contains expected entries
 *
 *  Example:
 *  \code{.cpp}
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
    for (const name_tree::Entry& entry : enumerable) {
      const Name& name = entry.getPrefix();
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
  auto&& enumerable = nt.partialEnumerate("/a", [] (const name_tree::Entry& entry) {
    return std::make_pair(entry.getPrefix() == "/a", true);
  });

  EnumerationVerifier(enumerable)
    .expect("/a")
    .end();
}

BOOST_AUTO_TEST_CASE(ExceptA)
{
  this->insertAbAc();

  // Accept anything except "root" nameA
  auto&& enumerable = nt.partialEnumerate("/a", [] (const name_tree::Entry& entry) {
    return std::make_pair(entry.getPrefix() != "/a", true);
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
  auto&& enumerable = nt.partialEnumerate("/a", [] (const name_tree::Entry& entry) {
      return std::make_pair(entry.getPrefix() != "/a", entry.getPrefix() != "/a/b");
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
  auto&& enumerable = nt.partialEnumerate("/a", [] (const name_tree::Entry& entry) {
      return std::make_pair(entry.getPrefix() != "/a", entry.getPrefix() != "/a/c");
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
  auto&& enumerable = nt.partialEnumerate("/a", [] (const name_tree::Entry& entry) {
      return std::make_pair(true, entry.getPrefix() != "/a");
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
    [] (const name_tree::Entry& entry) -> std::pair<bool, bool> {
      bool visitEntry = false;
      bool visitChildren = false;

      Name name = entry.getPrefix();

      if (name == "/" || name == "/A/B" || name == "/A/B/C" || name == "/A/D") {
        visitEntry = true;
      }

      if (name == "/" || name == "/A" || name == "/F") {
        visitChildren = true;
      }

      return {visitEntry, visitChildren};
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

  shared_ptr<name_tree::Entry> entry = nameTree.lookup(prefix);
  BOOST_CHECK_EQUAL(nameTree.size(), 9);
  BOOST_CHECK_EQUAL(nameTree.getNBuckets(), 32);

  nameTree.eraseEntryIfEmpty(entry);
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
    BOOST_CHECK(seenNames.insert(it->getPrefix()).second);
    if (it->getPrefix() == nameB) {
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

// .eraseEntryIfEmpty should not invalidate iterator
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
    BOOST_CHECK(seenNames.insert(it->getPrefix()).second);
    if (it->getPrefix() == nameD) {
      nt.eraseEntryIfEmpty(nt.findExactMatch("/A/F/G")); // /A/F/G and /A/F are erased
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

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd

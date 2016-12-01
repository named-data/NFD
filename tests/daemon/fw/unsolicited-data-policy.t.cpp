/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "fw/unsolicited-data-policy.hpp"
#include "fw/forwarder.hpp"
#include "tests/daemon/face/dummy-face.hpp"

#include "tests/test-common.hpp"
#include <boost/logic/tribool.hpp>
#include <boost/mpl/vector.hpp>

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

class UnsolicitedDataPolicyFixture : public UnitTestTimeFixture
{
protected:
  UnsolicitedDataPolicyFixture()
    : cs(forwarder.getCs())
  {
  }

  /** \tparam Policy policy type, or void to keep default policy
   */
  template<typename Policy>
  void
  setPolicy()
  {
    forwarder.setUnsolicitedDataPolicy(make_unique<Policy>());
  }

  bool
  isInCs(const Data& data)
  {
    using namespace boost::logic;

    tribool isFound = indeterminate;
    cs.find(Interest(data.getFullName()),
      bind([&] { isFound = true; }),
      bind([&] { isFound = false; }));

    this->advanceClocks(time::milliseconds(1));
    BOOST_REQUIRE(!indeterminate(isFound));
    return isFound;
  }

protected:
  Forwarder forwarder;
  Cs& cs;
};

template<>
void
UnsolicitedDataPolicyFixture::setPolicy<void>()
{
  // void keeps the default policy
}

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestUnsolicitedDataPolicy, UnsolicitedDataPolicyFixture)

BOOST_AUTO_TEST_CASE(GetPolicyNames)
{
  std::set<std::string> policyNames = UnsolicitedDataPolicy::getPolicyNames();
  BOOST_CHECK_EQUAL(policyNames.count("drop-all"), 1);
  BOOST_CHECK_EQUAL(policyNames.count("admit-local"), 1);
  BOOST_CHECK_EQUAL(policyNames.count("admit-network"), 1);
  BOOST_CHECK_EQUAL(policyNames.count("admit-all"), 1);
}

template<typename Policy, bool shouldAdmitLocal, bool shouldAdmitNonLocal>
struct FaceScopePolicyTest
{
  typedef Policy PolicyType;
  typedef std::integral_constant<bool, shouldAdmitLocal> ShouldAdmitLocal;
  typedef std::integral_constant<bool, shouldAdmitNonLocal> ShouldAdmitNonLocal;
};

typedef boost::mpl::vector<
  FaceScopePolicyTest<void, false, false>, // default policy
  FaceScopePolicyTest<DropAllUnsolicitedDataPolicy, false, false>,
  FaceScopePolicyTest<AdmitLocalUnsolicitedDataPolicy, true, false>,
  FaceScopePolicyTest<AdmitNetworkUnsolicitedDataPolicy, false, true>,
  FaceScopePolicyTest<AdmitAllUnsolicitedDataPolicy, true, true>
> FaceScopePolicyTests;

BOOST_AUTO_TEST_CASE_TEMPLATE(FaceScopePolicy, T, FaceScopePolicyTests)
{
  setPolicy<typename T::PolicyType>();

  auto face1 = make_shared<DummyFace>("dummy://", "dummy://",
                                      ndn::nfd::FACE_SCOPE_LOCAL);
  forwarder.addFace(face1);

  shared_ptr<Data> data1 = makeData("/unsolicited-from-local");
  forwarder.onIncomingData(*face1, *data1);
  BOOST_CHECK_EQUAL(isInCs(*data1), T::ShouldAdmitLocal::value);

  auto face2 = make_shared<DummyFace>("dummy://", "dummy://",
                                      ndn::nfd::FACE_SCOPE_NON_LOCAL);
  forwarder.addFace(face2);

  shared_ptr<Data> data2 = makeData("/unsolicited-from-non-local");
  forwarder.onIncomingData(*face2, *data2);
  BOOST_CHECK_EQUAL(isInCs(*data2), T::ShouldAdmitNonLocal::value);
}

BOOST_AUTO_TEST_SUITE_END() // TestUnsolicitedDataPolicy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd

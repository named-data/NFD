/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#include "face/transport.hpp"
#include "face/face.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/face/dummy-link-service.hpp"
#include "tests/daemon/face/dummy-transport.hpp"

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/bind.hpp>
#include <boost/mp11/map.hpp>
#include <boost/mp11/set.hpp>

namespace nfd::tests {

using namespace boost::mp11;
using namespace nfd::face;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_AUTO_TEST_SUITE(TestTransport)

BOOST_AUTO_TEST_CASE(PersistencyChange)
{
  auto transport = make_unique<DummyTransport>();
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(transport->persistencyHistory.size(), 0);

  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_NONE), false);
  BOOST_REQUIRE_EQUAL(transport->canChangePersistencyTo(transport->getPersistency()), true);
  BOOST_REQUIRE_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERMANENT), true);

  transport->setPersistency(transport->getPersistency());
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(transport->persistencyHistory.size(), 0);

  transport->setPersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  BOOST_REQUIRE_EQUAL(transport->persistencyHistory.size(), 1);
  BOOST_CHECK_EQUAL(transport->persistencyHistory.back(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
}

// Map from every TransportState to a valid state transition sequence
// for entering this state from UP.
using StateInitSequence = mp_list<
  mp_list_c<TransportState, TransportState::UP        /* nothing to do, state is already UP */>,
  mp_list_c<TransportState, TransportState::DOWN,     TransportState::DOWN>,
  mp_list_c<TransportState, TransportState::CLOSING,  TransportState::CLOSING>,
  mp_list_c<TransportState, TransportState::FAILED,   TransportState::FAILED>,
  mp_list_c<TransportState, TransportState::CLOSED,   TransportState::CLOSING, TransportState::CLOSED>
>;
static_assert(mp_is_map<StateInitSequence>());

using TransportStates = mp_map_keys<StateInitSequence>;

// The set of all state transitions (cartesian product of TransportStates)
using AllStateTransitions = mp_product<mp_list, TransportStates, TransportStates>;

// The set of *valid* state transitions
using ValidStateTransitions = mp_list<
  mp_list_c<TransportState, TransportState::UP, TransportState::DOWN>,
  mp_list_c<TransportState, TransportState::UP, TransportState::CLOSING>,
  mp_list_c<TransportState, TransportState::UP, TransportState::FAILED>,
  mp_list_c<TransportState, TransportState::DOWN, TransportState::UP>,
  mp_list_c<TransportState, TransportState::DOWN, TransportState::CLOSING>,
  mp_list_c<TransportState, TransportState::DOWN, TransportState::FAILED>,
  mp_list_c<TransportState, TransportState::CLOSING, TransportState::CLOSED>,
  mp_list_c<TransportState, TransportState::FAILED, TransportState::CLOSED>
>;
// Sanity check that there are no duplicates
static_assert(mp_is_set<ValidStateTransitions>());
// Sanity check that ValidStateTransitions is a proper subset of AllStateTransitions
static_assert(mp_all_of_q<ValidStateTransitions, mp_bind_front<mp_set_contains, AllStateTransitions>>());
static_assert(mp_size<ValidStateTransitions>() < mp_size<AllStateTransitions>());

BOOST_AUTO_TEST_CASE_TEMPLATE(SetState, T, AllStateTransitions)
{
  constexpr TransportState from = mp_first<T>::value;
  constexpr TransportState to = mp_second<T>::value;
  BOOST_TEST_INFO_SCOPE(from << " -> " << to);

  auto transport = make_unique<DummyTransport>();
  // initialize transport to the 'from' state
  using Steps = mp_rest<mp_map_find<StateInitSequence, mp_first<T>>>;
  mp_for_each<Steps>([&transport] (auto state) { transport->setState(state); });
  BOOST_REQUIRE_EQUAL(transport->getState(), from);

  bool hasSignal = false;
  transport->afterStateChange.connect([&] (TransportState oldState, TransportState newState) {
    hasSignal = true;
    BOOST_CHECK_EQUAL(oldState, from);
    BOOST_CHECK_EQUAL(newState, to);
  });

  // do transition
  constexpr bool isValid = (from == to) || mp_set_contains<ValidStateTransitions, T>();
  if constexpr (isValid) {
    BOOST_REQUIRE_NO_THROW(transport->setState(to));
    BOOST_CHECK_EQUAL(hasSignal, from != to);
  }
  else {
    BOOST_CHECK_THROW(transport->setState(to), std::runtime_error);
  }
}

class DummyTransportFixture : public GlobalIoFixture
{
protected:
  void
  initialize(unique_ptr<DummyTransport> t = make_unique<DummyTransport>())
  {
    this->face = make_unique<nfd::Face>(make_unique<DummyLinkService>(), std::move(t));
    this->transport = static_cast<DummyTransport*>(face->getTransport());
    this->sentPackets = &this->transport->sentPackets;
    this->receivedPackets = &static_cast<DummyLinkService*>(face->getLinkService())->receivedPackets;
  }

protected:
  unique_ptr<nfd::Face> face;
  DummyTransport* transport = nullptr;
  const std::vector<Block>* sentPackets = nullptr;
  const std::vector<RxPacket>* receivedPackets = nullptr;
};

BOOST_FIXTURE_TEST_CASE(Send, DummyTransportFixture)
{
  this->initialize();

  Block pkt1 = ndn::encoding::makeStringBlock(300, "Lorem ipsum dolor sit amet,");
  transport->send(pkt1);

  Block pkt2 = ndn::encoding::makeStringBlock(301, "consectetur adipiscing elit,");
  transport->send(pkt2);

  transport->setState(TransportState::DOWN);
  Block pkt3 = ndn::encoding::makeStringBlock(302, "sed do eiusmod tempor incididunt ");
  transport->send(pkt3);

  transport->setState(TransportState::CLOSING);
  Block pkt4 = ndn::encoding::makeStringBlock(303, "ut labore et dolore magna aliqua.");
  transport->send(pkt4);

  BOOST_CHECK_EQUAL(transport->getCounters().nOutPackets, 2);
  BOOST_CHECK_EQUAL(transport->getCounters().nOutBytes, pkt1.size() + pkt2.size());
  BOOST_REQUIRE_EQUAL(sentPackets->size(), 3);
  BOOST_CHECK(sentPackets->at(0) == pkt1);
  BOOST_CHECK(sentPackets->at(1) == pkt2);
  BOOST_CHECK(sentPackets->at(2) == pkt3);
}

BOOST_FIXTURE_TEST_CASE(Receive, DummyTransportFixture)
{
  this->initialize();

  Block pkt1 = ndn::encoding::makeStringBlock(300, "Lorem ipsum dolor sit amet,");
  transport->receivePacket(pkt1);

  Block pkt2 = ndn::encoding::makeStringBlock(301, "consectetur adipiscing elit,");
  transport->receivePacket(pkt2);

  transport->setState(TransportState::DOWN);
  Block pkt3 = ndn::encoding::makeStringBlock(302, "sed do eiusmod tempor incididunt ");
  transport->receivePacket(pkt3);

  BOOST_CHECK_EQUAL(transport->getCounters().nInPackets, 3);
  BOOST_CHECK_EQUAL(transport->getCounters().nInBytes, pkt1.size() + pkt2.size() + pkt3.size());
  BOOST_REQUIRE_EQUAL(receivedPackets->size(), 3);
  BOOST_CHECK(receivedPackets->at(0).packet == pkt1);
  BOOST_CHECK(receivedPackets->at(1).packet == pkt2);
  BOOST_CHECK(receivedPackets->at(2).packet == pkt3);
}

BOOST_AUTO_TEST_SUITE_END() // TestTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace nfd::tests

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

#include "core/event-emitter.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(UtilEventEmitter, BaseFixture)

class EventEmitterTester : noncopyable
{
public:
  EventEmitterTester();

  int m_hit;
  int m_a1;
  int m_a2;
  int m_a3;
  int m_a4;

  void
  clear();

  void
  f0();

  void
  f1(int a1);

  void
  f2(int a1, int a2);

  void
  f3(int a1, int a2, int a3);

  void
  f4(int a1, int a2, int a3, int a4);
};

EventEmitterTester::EventEmitterTester()
{
  this->clear();
}

void
EventEmitterTester::clear()
{
  m_hit = 0;
  m_a1 = 0;
  m_a2 = 0;
  m_a3 = 0;
  m_a4 = 0;
}

void
EventEmitterTester::f0()
{
  ++m_hit;
}

void
EventEmitterTester::f1(int a1)
{
  ++m_hit;
  m_a1 = a1;
}

void
EventEmitterTester::f2(int a1, int a2)
{
  ++m_hit;
  m_a1 = a1;
  m_a2 = a2;
}

void
EventEmitterTester::f3(int a1, int a2, int a3)
{
  ++m_hit;
  m_a1 = a1;
  m_a2 = a2;
  m_a3 = a3;
}

void
EventEmitterTester::f4(int a1, int a2, int a3, int a4)
{
  ++m_hit;
  m_a1 = a1;
  m_a2 = a2;
  m_a3 = a3;
  m_a4 = a4;
}

static int g_EventEmitterTest_RefObject_copyCount;

class EventEmitterTest_RefObject
{
public:
  EventEmitterTest_RefObject() {}

  EventEmitterTest_RefObject(const EventEmitterTest_RefObject& other);
};

EventEmitterTest_RefObject::EventEmitterTest_RefObject(const EventEmitterTest_RefObject& other)
{
  ++g_EventEmitterTest_RefObject_copyCount;
}

void
EventEmitterTest_RefObject_byVal(EventEmitterTest_RefObject a1) {}

void
EventEmitterTest_RefObject_byRef(const EventEmitterTest_RefObject& a1) {}


BOOST_AUTO_TEST_CASE(ZeroListener)
{
  EventEmitter<> ee;
  BOOST_CHECK_NO_THROW(ee());
}

BOOST_AUTO_TEST_CASE(TwoListeners)
{
  EventEmitterTester eet1;
  EventEmitterTester eet2;
  EventEmitter<> ee;
  ee += bind(&EventEmitterTester::f0, &eet1);
  ee += bind(&EventEmitterTester::f0, &eet2);
  ee();

  BOOST_CHECK_EQUAL(eet1.m_hit, 1);
  BOOST_CHECK_EQUAL(eet2.m_hit, 1);
}

BOOST_AUTO_TEST_CASE(ZeroArgument)
{
  EventEmitterTester eet;
  EventEmitter<> ee;
  ee += bind(&EventEmitterTester::f0, &eet);
  ee();

  BOOST_CHECK_EQUAL(eet.m_hit, 1);
}

BOOST_AUTO_TEST_CASE(OneArgument)
{
  EventEmitterTester eet;
  EventEmitter<int> ee;
  ee += bind(&EventEmitterTester::f1, &eet, _1);
  ee(11);

  BOOST_CHECK_EQUAL(eet.m_hit, 1);
  BOOST_CHECK_EQUAL(eet.m_a1, 11);
}

BOOST_AUTO_TEST_CASE(TwoArguments)
{
  EventEmitterTester eet;
  EventEmitter<int,int> ee;
  ee += bind(&EventEmitterTester::f2, &eet, _1, _2);
  ee(21, 22);

  BOOST_CHECK_EQUAL(eet.m_hit, 1);
  BOOST_CHECK_EQUAL(eet.m_a1, 21);
  BOOST_CHECK_EQUAL(eet.m_a2, 22);
}

BOOST_AUTO_TEST_CASE(ThreeArguments)
{
  EventEmitterTester eet;
  EventEmitter<int,int,int> ee;
  ee += bind(&EventEmitterTester::f3, &eet, _1, _2, _3);
  ee(31, 32, 33);

  BOOST_CHECK_EQUAL(eet.m_hit, 1);
  BOOST_CHECK_EQUAL(eet.m_a1, 31);
  BOOST_CHECK_EQUAL(eet.m_a2, 32);
  BOOST_CHECK_EQUAL(eet.m_a3, 33);
}

BOOST_AUTO_TEST_CASE(FourArguments)
{
  EventEmitterTester eet;
  EventEmitter<int,int,int,int> ee;
  ee += bind(&EventEmitterTester::f4, &eet, _1, _2, _3, _4);
  ee(41, 42, 43, 44);

  BOOST_CHECK_EQUAL(eet.m_hit, 1);
  BOOST_CHECK_EQUAL(eet.m_a1, 41);
  BOOST_CHECK_EQUAL(eet.m_a2, 42);
  BOOST_CHECK_EQUAL(eet.m_a3, 43);
  BOOST_CHECK_EQUAL(eet.m_a4, 44);
}

// EventEmitter passes arguments by reference,
// but it also allows a handler that accept arguments by value
BOOST_AUTO_TEST_CASE(HandlerByVal)
{
  EventEmitterTest_RefObject refObject;
  g_EventEmitterTest_RefObject_copyCount = 0;

  EventEmitter<EventEmitterTest_RefObject> ee;
  ee += &EventEmitterTest_RefObject_byVal;
  ee(refObject);

  BOOST_CHECK_EQUAL(g_EventEmitterTest_RefObject_copyCount, 1);
}

// EventEmitter passes arguments by reference, and no copying
// is necessary when handler accepts arguments by reference
BOOST_AUTO_TEST_CASE(HandlerByRef)
{
  EventEmitterTest_RefObject refObject;
  g_EventEmitterTest_RefObject_copyCount = 0;

  EventEmitter<EventEmitterTest_RefObject> ee;
  ee += &EventEmitterTest_RefObject_byRef;
  ee(refObject);

  BOOST_CHECK_EQUAL(g_EventEmitterTest_RefObject_copyCount, 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd

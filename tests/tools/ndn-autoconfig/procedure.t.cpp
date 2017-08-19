/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "ndn-autoconfig/procedure.hpp"

#include "../mock-nfd-mgmt-fixture.hpp"
#include <boost/logic/tribool.hpp>

namespace ndn {
namespace tools {
namespace autoconfig {
namespace tests {

using namespace ::nfd::tests;
using nfd::ControlParameters;

template<typename ProcedureClass>
class ProcedureFixture : public ::nfd::tools::tests::MockNfdMgmtFixture
{
public:
  void
  initialize(const Options& options)
  {
    procedure = make_unique<ProcedureClass>(face, m_keyChain);
    procedure->initialize(options);
  }

  bool
  runOnce()
  {
    BOOST_ASSERT(procedure != nullptr);
    boost::logic::tribool result;
    procedure->onComplete.connectSingleShot([&] (bool result1) { result = result1; });
    procedure->runOnce();
    face.processEvents();
    BOOST_CHECK_MESSAGE(!boost::logic::indeterminate(result), "onComplete is not invoked");
    return result;
  }

public:
  unique_ptr<ProcedureClass> procedure;
};

class DummyStage : public Stage
{
public:
  /** \param stageName stage name
   *  \param nCalls pointer to a variable which is incremented each time doStart is invoked
   *  \param result expected result, nullopt to cause a failued
   *  \param io io_service to asynchronously post the result
   */
  DummyStage(const std::string& stageName, int* nCalls, const optional<FaceUri>& result, boost::asio::io_service& io)
    : m_stageName(stageName)
    , m_nCalls(nCalls)
    , m_result(result)
    , m_io(io)
  {
  }

  const std::string&
  getName() const override
  {
    return m_stageName;
  }

private:
  void
  doStart() override
  {
    if (m_nCalls != nullptr) {
      ++(*m_nCalls);
    }
    m_io.post([this] {
      if (m_result) {
        this->succeed(*m_result);
      }
      else {
        this->fail("DUMMY-STAGE-FAIL");
      }
    });
  }

private:
  std::string m_stageName;
  int* m_nCalls;
  optional<FaceUri> m_result;
  boost::asio::io_service& m_io;
};

/** \brief two-stage Procedure where the first stage succeeds and the second stage fails
 *
 *  But the second stage shouldn't be invoked after the first stage succeeds.
 */
class ProcedureSuccessFailure : public Procedure
{
public:
  ProcedureSuccessFailure(Face& face, KeyChain& keyChain)
    : Procedure(face, keyChain)
    , m_io(face.getIoService())
  {
  }

private:
  void
  makeStages(const Options& options) override
  {
    m_stages.push_back(make_unique<DummyStage>("first", &nCalls1, FaceUri("udp://188.7.60.95"), m_io));
    m_stages.push_back(make_unique<DummyStage>("second", &nCalls2, nullopt, m_io));
  }

public:
  int nCalls1 = 0;
  int nCalls2 = 0;

private:
  boost::asio::io_service& m_io;
};

/** \brief two-stage Procedure where the first stage fails and the second stage succeeds
 */
class ProcedureFailureSuccess : public Procedure
{
public:
  ProcedureFailureSuccess(Face& face, KeyChain& keyChain)
    : Procedure(face, keyChain)
    , m_io(face.getIoService())
  {
  }

private:
  void
  makeStages(const Options& options) override
  {
    m_stages.push_back(make_unique<DummyStage>("first", &nCalls1, nullopt, m_io));
    m_stages.push_back(make_unique<DummyStage>("second", &nCalls2, FaceUri("tcp://40.23.174.71"), m_io));
  }

public:
  int nCalls1 = 0;
  int nCalls2 = 0;

private:
  boost::asio::io_service& m_io;
};

BOOST_AUTO_TEST_SUITE(NdnAutoconfig)
BOOST_AUTO_TEST_SUITE(TestProcedure)

BOOST_FIXTURE_TEST_CASE(Normal, ProcedureFixture<ProcedureSuccessFailure>)
{
  this->initialize(Options{});

  int nRegisterNdn = 0, nRegisterLocalhopNfd = 0;
  this->processInterest = [&] (const Interest& interest) {
    optional<ControlParameters> req = parseCommand(interest, "/localhost/nfd/faces/create");
    if (req) {
      BOOST_REQUIRE(req->hasUri());
      BOOST_CHECK_EQUAL(req->getUri(), "udp4://188.7.60.95:6363");

      ControlParameters resp;
      resp.setFaceId(1178)
          .setUri("udp4://188.7.60.95:6363")
          .setLocalUri("udp4://110.69.164.68:23197")
          .setFacePersistency(nfd::FacePersistency::FACE_PERSISTENCY_PERSISTENT)
          .setFlags(0);
      this->succeedCommand(interest, resp);
      return;
    }

    req = parseCommand(interest, "/localhost/nfd/rib/register");
    if (req) {
      BOOST_REQUIRE(req->hasFaceId());
      BOOST_CHECK_EQUAL(req->getFaceId(), 1178);
      BOOST_REQUIRE(req->hasOrigin());
      BOOST_CHECK_EQUAL(req->getOrigin(), nfd::ROUTE_ORIGIN_AUTOCONF);
      BOOST_REQUIRE(req->hasName());
      if (req->getName() == "/") {
        ++nRegisterNdn;
      }
      else if (req->getName() == "/localhop/nfd") {
        ++nRegisterLocalhopNfd;
      }
      else {
        BOOST_ERROR("unexpected prefix registration " << req->getName());
      }

      ControlParameters resp;
      resp.setName(req->getName())
          .setFaceId(1178)
          .setOrigin(nfd::ROUTE_ORIGIN_AUTOCONF)
          .setCost(1)
          .setFlags(0);
      this->succeedCommand(interest, resp);
      return;
    }

    BOOST_FAIL("unrecognized command Interest " << interest);
  };

  BOOST_CHECK_EQUAL(this->runOnce(), true);
  BOOST_CHECK_EQUAL(procedure->nCalls1, 1);
  BOOST_CHECK_EQUAL(procedure->nCalls2, 0);
  BOOST_CHECK_EQUAL(nRegisterNdn, 1);
  BOOST_CHECK_EQUAL(nRegisterLocalhopNfd, 1);
}

BOOST_FIXTURE_TEST_CASE(ExistingFace, ProcedureFixture<ProcedureFailureSuccess>)
{
  this->initialize(Options{});

  int nRegisterDefault = 0, nRegisterLocalhopNfd = 0;
  this->processInterest = [&] (const Interest& interest) {
    optional<ControlParameters> req = parseCommand(interest, "/localhost/nfd/faces/create");
    if (req) {
      ControlParameters resp;
      resp.setFaceId(3146)
          .setUri("tcp4://40.23.174.71:6363")
          .setLocalUri("tcp4://34.213.69.67:14445")
          .setFacePersistency(nfd::FacePersistency::FACE_PERSISTENCY_PERSISTENT)
          .setFlags(0);
      this->failCommand(interest, 409, "conflict-409", resp);
      return;
    }

    req = parseCommand(interest, "/localhost/nfd/rib/register");
    if (req) {
      BOOST_REQUIRE(req->hasName());
      if (req->getName() == "/") {
        ++nRegisterDefault;
      }
      else if (req->getName() == "/localhop/nfd") {
        ++nRegisterLocalhopNfd;
      }
      else {
        BOOST_ERROR("unexpected prefix registration " << req->getName());
      }

      ControlParameters resp;
      resp.setName(req->getName())
          .setFaceId(3146)
          .setOrigin(nfd::ROUTE_ORIGIN_AUTOCONF)
          .setCost(1)
          .setFlags(0);
      this->succeedCommand(interest, resp);
      return;
    }

    BOOST_FAIL("unrecognized command Interest " << interest);
  };

  BOOST_CHECK_EQUAL(this->runOnce(), true);
  BOOST_CHECK_EQUAL(procedure->nCalls1, 1);
  BOOST_CHECK_EQUAL(procedure->nCalls2, 1);
  BOOST_CHECK_EQUAL(nRegisterDefault, 1);
  BOOST_CHECK_EQUAL(nRegisterLocalhopNfd, 1);
}

BOOST_AUTO_TEST_SUITE_END() // TestProcedure
BOOST_AUTO_TEST_SUITE_END() // NdnAutoconfig

} // namespace tests
} // namespace autoconfig
} // namespace tools
} // namespace ndn

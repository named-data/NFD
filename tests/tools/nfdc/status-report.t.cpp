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

#include "nfdc/status-report.hpp"
#include "core/scheduler.hpp"

#include "status-fixture.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

const std::string STATUS_XML = stripXmlSpaces(R"XML(
  <?xml version="1.0"?>
  <nfdStatus xmlns="ndn:/localhost/nfd/status/1">
    <module1/>
    <module2/>
  </nfdStatus>
)XML");

const std::string STATUS_TEXT = std::string(R"TEXT(
module1
module2
)TEXT").substr(1);

class DummyModule : public Module
{
public:
  explicit
  DummyModule(const std::string& moduleName)
    : m_moduleName(moduleName)
    , m_res(0)
    , m_delay(time::milliseconds(1))
  {
  }

  /** \brief cause fetchStatus to succeed or fail
   *  \param res zero to succeed, non-zero to fail with specific code
   *  \param delay duration from fetchStatus invocation to succeed or fail; must be positive
   */
  void
  setResult(uint32_t res, time::nanoseconds delay)
  {
    BOOST_ASSERT(delay > time::nanoseconds::zero());
    m_res = res;
    m_delay = delay;
  }

  void
  fetchStatus(Controller& controller,
              const std::function<void()>& onSuccess,
              const Controller::DatasetFailCallback& onFailure,
              const CommandOptions& options) final
  {
    ++nFetchStatusCalls;
    scheduler::schedule(m_delay, [=] {
      if (m_res == 0) {
        onSuccess();
      }
      else {
        onFailure(m_res, m_moduleName + " fails with code " + to_string(m_res));
      }
    });
  }

  void
  formatStatusXml(std::ostream& os) const final
  {
    os << '<' << m_moduleName << "/>";
  }

  void
  formatStatusText(std::ostream& os) const final
  {
    os << m_moduleName << '\n';
  }

public:
  int nFetchStatusCalls = 0;

private:
  std::string m_moduleName;
  uint32_t m_res;
  time::nanoseconds m_delay;
};

class StatusReportTester : public StatusReport
{
private:
  void
  processEvents(Face&) override
  {
    processEventsFunc();
  }

public:
  std::function<void()> processEventsFunc;
};

class StatusReportModulesFixture : public IdentityManagementTimeFixture
{
protected:
  StatusReportModulesFixture()
    : face(g_io, m_keyChain)
    , controller(face, m_keyChain, validator)
    , res(0)
  {
  }

  DummyModule&
  addModule(const std::string& moduleName)
  {
    report.sections.push_back(make_unique<DummyModule>(moduleName));
    return static_cast<DummyModule&>(*report.sections.back());
  }

  void
  collect(time::nanoseconds tick, size_t nTicks)
  {
    report.processEventsFunc = [=] {
      this->advanceClocks(tick, nTicks);
    };
    res = report.collect(face, m_keyChain, validator, CommandOptions());

    if (res == 0) {
      statusXml.str("");
      report.formatXml(statusXml);
      statusText.str("");
      report.formatText(statusText);
    }
  }

protected:
  ndn::util::DummyClientFace face;
  ValidatorNull validator;
  Controller controller;
  StatusReportTester report;

  uint32_t res;
  output_test_stream statusXml;
  output_test_stream statusText;
};


BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_FIXTURE_TEST_SUITE(TestStatusReport, StatusReportModulesFixture)

BOOST_AUTO_TEST_CASE(Normal)
{
  DummyModule& m1 = addModule("module1");
  m1.setResult(0, time::milliseconds(10));
  DummyModule& m2 = addModule("module2");
  m2.setResult(0, time::milliseconds(20));

  this->collect(time::milliseconds(5), 6);

  BOOST_CHECK_EQUAL(m1.nFetchStatusCalls, 1);
  BOOST_CHECK_EQUAL(m2.nFetchStatusCalls, 1);

  BOOST_CHECK_EQUAL(res, 0);
  BOOST_CHECK(statusXml.is_equal(STATUS_XML));
  BOOST_CHECK(statusText.is_equal(STATUS_TEXT));
}

BOOST_AUTO_TEST_CASE(Reorder)
{
  DummyModule& m1 = addModule("module1");
  m1.setResult(0, time::milliseconds(20));
  DummyModule& m2 = addModule("module2");
  m2.setResult(0, time::milliseconds(10)); // module2 completes earlier than module1

  this->collect(time::milliseconds(5), 6);

  BOOST_CHECK_EQUAL(res, 0);
  BOOST_CHECK(statusXml.is_equal(STATUS_XML)); // output is still in order
  BOOST_CHECK(statusText.is_equal(STATUS_TEXT));
}

BOOST_AUTO_TEST_CASE(Error)
{
  DummyModule& m1 = addModule("module1");
  m1.setResult(0, time::milliseconds(20));
  DummyModule& m2 = addModule("module2");
  m2.setResult(500, time::milliseconds(10));

  this->collect(time::milliseconds(5), 6);

  BOOST_CHECK_EQUAL(res, 1000500);
}

BOOST_AUTO_TEST_SUITE_END() // TestStatusReport
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd

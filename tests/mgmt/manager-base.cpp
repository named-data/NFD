/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/manager-base.hpp"
#include "mgmt/internal-face.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

NFD_LOG_INIT("ManagerBaseTest");

class ManagerBaseTest : public ManagerBase, protected BaseFixture
{

public:

  ManagerBaseTest()
    : ManagerBase(make_shared<InternalFace>(), "TEST-PRIVILEGE"),
      m_callbackFired(false)
  {

  }

  void
  testSetResponse(ControlResponse& response,
                  uint32_t code,
                  const std::string& text)
  {
    setResponse(response, code, text);
  }

  void
  testSendResponse(const Name& name,
                   uint32_t code,
                   const std::string& text,
                   const Block& body)
  {
    sendResponse(name, code, text, body);
  }

  void
  testSendResponse(const Name& name,
                   uint32_t code,
                   const std::string& text)
  {
    sendResponse(name, code, text);
  }

  void
  testSendResponse(const Name& name,
                   const ControlResponse& response)
  {
    sendResponse(name, response);
  }

  shared_ptr<InternalFace>
  getInternalFace()
  {
    return ndn::ptr_lib::static_pointer_cast<InternalFace>(m_face);
  }

  void
  validateControlResponse(const Data& response,
                          const Name& expectedName,
                          uint32_t expectedCode,
                          const std::string& expectedText)
  {
    m_callbackFired = true;
    Block controlRaw = response.getContent().blockFromValue();

    ControlResponse control;
    control.wireDecode(controlRaw);

    NFD_LOG_DEBUG("received control response"
                  << " name: " << response.getName()
                  << " code: " << control.getCode()
                  << " text: " << control.getText());

    BOOST_REQUIRE(response.getName() == expectedName);
    BOOST_REQUIRE(control.getCode() == expectedCode);
    BOOST_REQUIRE(control.getText() == expectedText);
  }

  void
  validateControlResponse(const Data& response,
                          const Name& expectedName,
                          uint32_t expectedCode,
                          const std::string& expectedText,
                          const Block& expectedBody)
  {
    m_callbackFired = true;
    Block controlRaw = response.getContent().blockFromValue();

    ControlResponse control;
    control.wireDecode(controlRaw);

    NFD_LOG_DEBUG("received control response"
                  << " name: " << response.getName()
                  << " code: " << control.getCode()
                  << " text: " << control.getText());

    BOOST_REQUIRE(response.getName() == expectedName);
    BOOST_REQUIRE(control.getCode() == expectedCode);
    BOOST_REQUIRE(control.getText() == expectedText);

    BOOST_REQUIRE(control.getBody().value_size() == expectedBody.value_size());

    BOOST_CHECK(memcmp(control.getBody().value(), expectedBody.value(),
                       expectedBody.value_size()) == 0);
  }

  bool
  didCallbackFire()
  {
    return m_callbackFired;
  }

private:

  bool m_callbackFired;

};

BOOST_FIXTURE_TEST_SUITE(MgmtManagerBase, ManagerBaseTest)

BOOST_AUTO_TEST_CASE(SetResponse)
{
  ControlResponse response(200, "OK");

  BOOST_CHECK_EQUAL(response.getCode(), 200);
  BOOST_CHECK_EQUAL(response.getText(), "OK");

  testSetResponse(response, 100, "test");

  BOOST_CHECK_EQUAL(response.getCode(), 100);
  BOOST_CHECK_EQUAL(response.getText(), "test");
}

BOOST_AUTO_TEST_CASE(SendResponse4Arg)
{
  ndn::nfd::ControlParameters parameters;
  parameters.setName("/test/body");

  getInternalFace()->onReceiveData +=
    bind(&ManagerBaseTest::validateControlResponse, this, _1,
         "/response", 100, "test", parameters.wireEncode());

  testSendResponse("/response", 100, "test", parameters.wireEncode());
  BOOST_REQUIRE(didCallbackFire());
}


BOOST_AUTO_TEST_CASE(SendResponse3Arg)
{
  getInternalFace()->onReceiveData +=
    bind(&ManagerBaseTest::validateControlResponse, this, _1,
         "/response", 100, "test");

  testSendResponse("/response", 100, "test");
  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(SendResponse2Arg)
{
  getInternalFace()->onReceiveData +=
    bind(&ManagerBaseTest::validateControlResponse, this, _1,
         "/response", 100, "test");

  ControlResponse response(100, "test");

  testSendResponse("/response", 100, "test");
  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/local-control-header-manager.hpp"
#include "face/face.hpp"
#include "mgmt/internal-face.hpp"
#include "../face/dummy-face.hpp"

#include <algorithm>

#include <boost/test/unit_test.hpp>

namespace nfd {

NFD_LOG_INIT("LocalControlHeaderManagerTest");

class LocalControlHeaderManagerFixture
{
public:

  LocalControlHeaderManagerFixture()
    : m_callbackFired(false)
  {

  }

  shared_ptr<Face>
  getFace(FaceId id)
  {
    if (id > 0 && id <= m_faces.size())
      {
        return m_faces[id-1];
      }
    NFD_LOG_DEBUG("No face found returning NULL");
    return shared_ptr<DummyFace>();
  }

  void
  addFace(shared_ptr<Face> face)
  {
    m_faces.push_back(face);
  }

  void
  validateControlResponse(const Data& response,
                          const Name& expectedName,
                          uint32_t expectedCode,
                          const std::string& expectedText)
  {
    m_callbackFired = true;

    ControlResponse control;
    Block controlRaw = response.getContent().blockFromValue();

    control.wireDecode(controlRaw);

    NFD_LOG_DEBUG("received control response"
                  << " Name: " << response.getName()
                  << " code: " << control.getCode()
                  << " text: " << control.getText());

    BOOST_CHECK_EQUAL(response.getName(), expectedName);
    BOOST_CHECK_EQUAL(control.getCode(), expectedCode);
    BOOST_CHECK_EQUAL(control.getText(), expectedText);

    if (!control.getBody().empty())
      {
        BOOST_FAIL("found unexpected control response body");
      }
  }

  bool
  didCallbackFire()
  {
    return m_callbackFired;
  }

  void
  resetCallbackFired()
  {
    m_callbackFired = false;
  }

private:
  std::vector<shared_ptr<Face> > m_faces;
  bool m_callbackFired;
};


BOOST_AUTO_TEST_SUITE(MgmtLocalControlHeaderManager)

BOOST_FIXTURE_TEST_CASE(InFaceId, LocalControlHeaderManagerFixture)
{
  shared_ptr<Face> dummy = make_shared<DummyFace>();
  addFace(dummy);

  shared_ptr<InternalFace> face(make_shared<InternalFace>());

  LocalControlHeaderManager manager(bind(&LocalControlHeaderManagerFixture::getFace, this, _1),
                                        face);

  Name enable("/localhost/nfd/control-header/in-faceid/enable");

  face->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         enable, 200, "Success");

  Interest enableCommand(enable);
  enableCommand.setIncomingFaceId(1);
  manager.onLocalControlHeaderRequest(enableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));

  face->onReceiveData.clear();
  resetCallbackFired();

  Name disable("/localhost/nfd/control-header/in-faceid/disable");

  face->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         disable, 200, "Success");

  Interest disableCommand(disable);
  disableCommand.setIncomingFaceId(1);
  manager.onLocalControlHeaderRequest(disableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
}

BOOST_FIXTURE_TEST_CASE(NextHopFaceId, LocalControlHeaderManagerFixture)
{
  shared_ptr<Face> dummy = make_shared<DummyFace>();
  addFace(dummy);

  shared_ptr<InternalFace> face(make_shared<InternalFace>());

  LocalControlHeaderManager manager(bind(&LocalControlHeaderManagerFixture::getFace, this, _1),
                                        face);

  Name enable("/localhost/nfd/control-header/nexthop-faceid/enable");

  face->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         enable, 200, "Success");

  Interest enableCommand(enable);
  enableCommand.setIncomingFaceId(1);
  manager.onLocalControlHeaderRequest(enableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));


  face->onReceiveData.clear();
  resetCallbackFired();

  Name disable("/localhost/nfd/control-header/nexthop-faceid/disable");

  face->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         disable, 200, "Success");

  Interest disableCommand(disable);
  disableCommand.setIncomingFaceId(1);
  manager.onLocalControlHeaderRequest(disableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
}

BOOST_FIXTURE_TEST_CASE(ShortCommand, LocalControlHeaderManagerFixture)
{
  shared_ptr<Face> dummy = make_shared<DummyFace>();
  addFace(dummy);

  shared_ptr<InternalFace> face(make_shared<InternalFace>());

  LocalControlHeaderManager manager(bind(&LocalControlHeaderManagerFixture::getFace, this, _1),
                                        face);

  Name commandName("/localhost/nfd/control-header");

  face->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         commandName, 400, "Malformed command");

  Interest command(commandName);
  command.setIncomingFaceId(1);
  manager.onLocalControlHeaderRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
}

BOOST_FIXTURE_TEST_CASE(ShortCommandModule, LocalControlHeaderManagerFixture)
{
  shared_ptr<Face> dummy = make_shared<DummyFace>();
  addFace(dummy);

  shared_ptr<InternalFace> face(make_shared<InternalFace>());

  LocalControlHeaderManager manager(bind(&LocalControlHeaderManagerFixture::getFace, this, _1),
                                        face);

  Name commandName("/localhost/nfd/control-header/in-faceid");

  face->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         commandName, 400, "Malformed command");

  Interest command(commandName);
  command.setIncomingFaceId(1);
  manager.onLocalControlHeaderRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
}

BOOST_FIXTURE_TEST_CASE(UnsupportedModule, LocalControlHeaderManagerFixture)
{
  shared_ptr<Face> dummy = make_shared<DummyFace>();
  addFace(dummy);

  shared_ptr<InternalFace> face(make_shared<InternalFace>());

  LocalControlHeaderManager manager(bind(&LocalControlHeaderManagerFixture::getFace, this, _1),
                                        face);

  Name commandName("/localhost/nfd/control-header/madeup/moremadeup");

  face->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         commandName, 501, "Unsupported");

  Interest command(commandName);
  command.setIncomingFaceId(1);
  manager.onLocalControlHeaderRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
}

BOOST_FIXTURE_TEST_CASE(InFaceIdUnsupportedVerb, LocalControlHeaderManagerFixture)
{
  shared_ptr<Face> dummy = make_shared<DummyFace>();
  addFace(dummy);

  shared_ptr<InternalFace> face(make_shared<InternalFace>());

  LocalControlHeaderManager manager(bind(&LocalControlHeaderManagerFixture::getFace, this, _1),
                                        face);

  Name commandName("/localhost/nfd/control-header/in-faceid/madeup");

  face->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         commandName, 501, "Unsupported");

  Interest command(commandName);
  command.setIncomingFaceId(1);
  manager.onLocalControlHeaderRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
}

BOOST_FIXTURE_TEST_CASE(NextHopFaceIdUnsupportedVerb, LocalControlHeaderManagerFixture)
{
  shared_ptr<Face> dummy = make_shared<DummyFace>();
  addFace(dummy);

  shared_ptr<InternalFace> face(make_shared<InternalFace>());

  LocalControlHeaderManager manager(bind(&LocalControlHeaderManagerFixture::getFace, this, _1),
                                        face);

  Name commandName("/localhost/nfd/control-header/nexthop-faceid/madeup");

  face->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         commandName, 501, "Unsupported");

  Interest command(commandName);
  command.setIncomingFaceId(1);
  manager.onLocalControlHeaderRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd

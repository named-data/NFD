/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/local-control-header-manager.hpp"
#include "mgmt/internal-face.hpp"
#include "tests/face/dummy-face.hpp"

#include "tests/test-common.hpp"
#include "validation-common.hpp"

namespace nfd {
namespace tests {

NFD_LOG_INIT("LocalControlHeaderManagerTest");

class LocalControlHeaderManagerFixture : protected BaseFixture
{
public:

  shared_ptr<Face>
  getFace(FaceId id)
  {
    if (id > 0 && static_cast<size_t>(id) <= m_faces.size())
      {
        return m_faces[id - 1];
      }
    NFD_LOG_DEBUG("No face found returning NULL");
    return shared_ptr<DummyFace>();
  }

  void
  addFace(shared_ptr<Face> face)
  {
    m_faces.push_back(face);
  }

  shared_ptr<InternalFace>
  getInternalFace()
  {
    return m_face;
  }

  LocalControlHeaderManager&
  getManager()
  {
    return m_manager;
  }

  void
  addInterestRule(const std::string& regex,
                  ndn::IdentityCertificate& certificate)
  {
    m_manager.addInterestRule(regex, certificate);
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

protected:
  LocalControlHeaderManagerFixture()
    : m_face(make_shared<InternalFace>()),
      m_manager(bind(&LocalControlHeaderManagerFixture::getFace, this, _1),
                m_face),
      m_callbackFired(false)
  {
  }

private:
  shared_ptr<InternalFace> m_face;
  LocalControlHeaderManager m_manager;
  std::vector<shared_ptr<Face> > m_faces;
  bool m_callbackFired;
};

template <typename T> class AuthorizedCommandFixture:
    public CommandFixture<T>
{
public:
  AuthorizedCommandFixture()
  {
    const std::string regex = "^<localhost><nfd><control-header>";
    T::addInterestRule(regex, *CommandFixture<T>::m_certificate);
  }

  virtual
  ~AuthorizedCommandFixture()
  {
  }
};

BOOST_FIXTURE_TEST_SUITE(MgmtLocalControlHeaderManager,
                         AuthorizedCommandFixture<LocalControlHeaderManagerFixture>)

BOOST_AUTO_TEST_CASE(InFaceId)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  addFace(dummy);

  Name enable("/localhost/nfd/control-header/in-faceid/enable");
  shared_ptr<Interest> enableCommand(make_shared<Interest>(enable));
  enableCommand->setIncomingFaceId(1);

  generateCommand(*enableCommand);

  getInternalFace()->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         enableCommand->getName(), 200, "Success");

  getManager().onLocalControlHeaderRequest(*enableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));

  getInternalFace()->onReceiveData.clear();
  resetCallbackFired();

  Name disable("/localhost/nfd/control-header/in-faceid/disable");
  shared_ptr<Interest> disableCommand(make_shared<Interest>(disable));
  disableCommand->setIncomingFaceId(1);

  generateCommand(*disableCommand);

  getInternalFace()->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         disableCommand->getName(), 200, "Success");

  getManager().onLocalControlHeaderRequest(*disableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
}

BOOST_AUTO_TEST_CASE(NextHopFaceId)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  addFace(dummy);

  Name enable("/localhost/nfd/control-header/nexthop-faceid/enable");

  shared_ptr<Interest> enableCommand(make_shared<Interest>(enable));
  enableCommand->setIncomingFaceId(1);
  generateCommand(*enableCommand);

  getInternalFace()->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         enableCommand->getName(), 200, "Success");

  getManager().onLocalControlHeaderRequest(*enableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));

  getInternalFace()->onReceiveData.clear();
  resetCallbackFired();

  Name disable("/localhost/nfd/control-header/nexthop-faceid/disable");
  shared_ptr<Interest> disableCommand(make_shared<Interest>(disable));
  disableCommand->setIncomingFaceId(1);

  generateCommand(*disableCommand);

  getInternalFace()->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         disableCommand->getName(), 200, "Success");

  getManager().onLocalControlHeaderRequest(*disableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
}

BOOST_AUTO_TEST_CASE(ShortCommand)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  addFace(dummy);

  Name commandName("/localhost/nfd/control-header");
  Interest command(commandName);
  command.setIncomingFaceId(1);

  getInternalFace()->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         commandName, 400, "Malformed command");

  getManager().onLocalControlHeaderRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
}

BOOST_AUTO_TEST_CASE(ShortCommandModule)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  addFace(dummy);

  Name commandName("/localhost/nfd/control-header/in-faceid");

  getInternalFace()->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         commandName, 400, "Malformed command");

  Interest command(commandName);
  command.setIncomingFaceId(1);
  getManager().onLocalControlHeaderRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
}

BOOST_AUTO_TEST_CASE(UnsupportedModule)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  addFace(dummy);

  Name commandName("/localhost/nfd/control-header/madeup/moremadeup");
  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  command->setIncomingFaceId(1);

  generateCommand(*command);

  getInternalFace()->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         command->getName(), 501, "Unsupported");

  getManager().onLocalControlHeaderRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
}

BOOST_AUTO_TEST_CASE(InFaceIdUnsupportedVerb)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  addFace(dummy);

  Name commandName("/localhost/nfd/control-header/in-faceid/madeup");
  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  command->setIncomingFaceId(1);


  generateCommand(*command);

  getInternalFace()->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         command->getName(), 501, "Unsupported");

  getManager().onLocalControlHeaderRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
}

BOOST_AUTO_TEST_CASE(NextHopFaceIdUnsupportedVerb)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  addFace(dummy);

  Name commandName("/localhost/nfd/control-header/nexthop-faceid/madeup");
  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  command->setIncomingFaceId(1);

  generateCommand(*command);

  getInternalFace()->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         command->getName(), 501, "Unsupported");

  getManager().onLocalControlHeaderRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));
}

BOOST_FIXTURE_TEST_CASE(UnauthorizedCommand,
                        UnauthorizedCommandFixture<LocalControlHeaderManagerFixture>)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  addFace(dummy);

  Name enable("/localhost/nfd/control-header/in-faceid/enable");
  shared_ptr<Interest> enableCommand(make_shared<Interest>(enable));
  enableCommand->setIncomingFaceId(1);

  generateCommand(*enableCommand);

  getInternalFace()->onReceiveData +=
    bind(&LocalControlHeaderManagerFixture::validateControlResponse, this, _1,
         enableCommand->getName(), 403, "Unauthorized command");

  getManager().onLocalControlHeaderRequest(*enableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd

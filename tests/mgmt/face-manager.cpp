// /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// /**
//  * Copyright (C) 2014 Named Data Networking Project
//  * See COPYING for copyright and distribution information.
//  */

#include "mgmt/face-manager.hpp"
#include "mgmt/internal-face.hpp"
#include "face/face.hpp"
#include "../face/dummy-face.hpp"
#include "fw/face-table.hpp"
#include "fw/forwarder.hpp"

#include "common.hpp"
#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

NFD_LOG_INIT("FaceManagerTest");

class TestDummyFace : public DummyFace
{
public:

  TestDummyFace()
    : m_closeFired(false)
  {

  }

  virtual
  ~TestDummyFace()
  {

  }

  virtual void
  close()
  {
    m_closeFired = true;
  }

  bool
  didCloseFire() const
  {
    return m_closeFired;
  }

private:
  bool m_closeFired;
};

class TestFaceTable : public FaceTable
{
public:
  TestFaceTable(Forwarder& forwarder)
    : FaceTable(forwarder),
      m_addFired(false),
      m_removeFired(false),
      m_getFired(false),
      m_dummy(make_shared<TestDummyFace>())
  {

  }

  virtual
  ~TestFaceTable()
  {

  }

  virtual void
  add(shared_ptr<Face> face)
  {
    m_addFired = true;
  }

  virtual void
  remove(shared_ptr<Face> face)
  {
    m_removeFired = true;
  }

  virtual shared_ptr<Face>
  get(FaceId id) const
  {
    m_getFired = true;
    return m_dummy;
  }

  bool
  didAddFire() const
  {
    return m_addFired;
  }

  bool
  didRemoveFire() const
  {
    return m_removeFired;
  }

  bool
  didGetFire() const
  {
    return m_getFired;
  }

  void
  reset()
  {
    m_addFired = false;
    m_removeFired = false;
    m_getFired = false;
  }

  shared_ptr<TestDummyFace>&
  getDummyFace()
  {
    return m_dummy;
  }

private:
  bool m_addFired;
  bool m_removeFired;
  mutable bool m_getFired;
  shared_ptr<TestDummyFace> m_dummy;
};


class TestFaceTableFixture : public BaseFixture
{
public:
  TestFaceTableFixture()
    : m_faceTable(m_forwarder)
  {

  }

  virtual
  ~TestFaceTableFixture()
  {

  }

protected:
  Forwarder m_forwarder;
  TestFaceTable m_faceTable;
};

class TestFaceManagerCommon
{
public:
  TestFaceManagerCommon()
    : m_face(make_shared<InternalFace>()),
      m_callbackFired(false)
  {

  }

  virtual
  ~TestFaceManagerCommon()
  {

  }

  shared_ptr<InternalFace>&
  getFace()
  {
    return m_face;
  }

  void
  validateControlResponseCommon(const Data& response,
                                const Name& expectedName,
                                uint32_t expectedCode,
                                const std::string& expectedText,
                                ControlResponse& control)
  {
    m_callbackFired = true;
    Block controlRaw = response.getContent().blockFromValue();

    control.wireDecode(controlRaw);

    // NFD_LOG_DEBUG("received control response"
    //               << " Name: " << response.getName()
    //               << " code: " << control.getCode()
    //               << " text: " << control.getText());

    BOOST_CHECK_EQUAL(response.getName(), expectedName);
    BOOST_CHECK_EQUAL(control.getCode(), expectedCode);
    BOOST_CHECK_EQUAL(control.getText(), expectedText);
  }

  void
  validateControlResponse(const Data& response,
                          const Name& expectedName,
                          uint32_t expectedCode,
                          const std::string& expectedText)
  {
    ControlResponse control;
    validateControlResponseCommon(response, expectedName,
                                  expectedCode, expectedText, control);

    if (!control.getBody().empty())
      {
        BOOST_FAIL("found unexpected control response body");
      }
  }

  void
  validateControlResponse(const Data& response,
                          const Name& expectedName,
                          uint32_t expectedCode,
                          const std::string& expectedText,
                          const Block& expectedBody)
  {
    ControlResponse control;
    validateControlResponseCommon(response, expectedName,
                                  expectedCode, expectedText, control);

    BOOST_REQUIRE(!control.getBody().empty());
    BOOST_REQUIRE(control.getBody().value_size() == expectedBody.value_size());

    BOOST_CHECK(memcmp(control.getBody().value(), expectedBody.value(),
                       expectedBody.value_size()) == 0);

  }

  bool
  didCallbackFire() const
  {
    return m_callbackFired;
  }

  void
  resetCallbackFired()
  {
    m_callbackFired = false;
  }

protected:
  shared_ptr<InternalFace> m_face;

private:
  bool m_callbackFired;
};

class FaceManagerFixture : public TestFaceTableFixture, public TestFaceManagerCommon
{
public:
  FaceManagerFixture()
    : m_manager(m_faceTable, m_face)
  {
    m_manager.setConfigFile(m_config);
  }

  void
  parseConfig(const std::string configuration, bool isDryRun)
  {
    m_config.parse(configuration, isDryRun);
  }

  virtual
  ~FaceManagerFixture()
  {

  }

  FaceManager&
  getManager()
  {
    return m_manager;
  }

  bool
  didFaceTableAddFire() const
  {
    return m_faceTable.didAddFire();
  }

  bool
  didFaceTableRemoveFire() const
  {
    return m_faceTable.didRemoveFire();
  }

  bool
  didFaceTableGetFire() const
  {
    return m_faceTable.didGetFire();
  }

  void
  resetFaceTable()
  {
    m_faceTable.reset();
  }

private:
  FaceManager m_manager;
  ConfigFile m_config;
};

BOOST_FIXTURE_TEST_SUITE(MgmtFaceManager, FaceManagerFixture)

bool
isExpectedException(const ConfigFile::Error& error, const std::string& expectedMessage)
{
  if (error.what() != expectedMessage)
    {
      NFD_LOG_ERROR("expected: " << expectedMessage << "\tgot: " << error.what());
    }
  return error.what() == expectedMessage;
}

BOOST_AUTO_TEST_CASE(TestProcessSectionUnix)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  unix\n"
    "  {\n"
    "    listen yes\n"
    "    path /tmp/nfd.sock\n"
    "  }\n"
    "}\n";
  BOOST_TEST_CHECKPOINT("Calling parse");
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionUnixDryRun)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  unix\n"
    "  {\n"
    "    listen yes\n"
    "    path /var/run/nfd.sock\n"
    "  }\n"
    "}\n";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionUnixBadListen)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  unix\n"
    "  {\n"
    "    listen hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "Invalid value for option \"listen\" in \"unix\" section"));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionUnixUnknownOption)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  unix\n"
    "  {\n"
    "    hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "Unrecognized option \"hello\" in \"unix\" section"));
}



BOOST_AUTO_TEST_CASE(TestProcessSectionTcp)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  tcp\n"
    "  {\n"
    "    listen yes\n"
    "    port 6363\n"
    "  }\n"
    "}\n";
  try
    {
      parseConfig(CONFIG, false);
    }
  catch (const std::runtime_error& e)
    {
      const std::string reason = e.what();
      if (reason.find("Address in use") != std::string::npos)
        {
          BOOST_FAIL(reason);
        }
    }
}

BOOST_AUTO_TEST_CASE(TestProcessSectionTcpDryRun)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  tcp\n"
    "  {\n"
    "    listen yes\n"
    "    port 6363\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionTcpBadListen)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  tcp\n"
    "  {\n"
    "    listen hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "Invalid value for option \"listen\" in \"tcp\" section"));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionTcpUnknownOption)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  tcp\n"
    "  {\n"
    "    hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "Unrecognized option \"hello\" in \"tcp\" section"));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionUdp)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    port 6363\n"
    "    idle_timeout 30\n"
    "    keep_alive_interval 25\n"
    "    mcast yes\n"
    "    mcast_port 56363\n"
    "    mcast_group 224.0.23.170\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionUdpDryRun)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    port 6363\n"
    "    idle_timeout 30\n"
    "    keep_alive_interval 25\n"
    "    mcast yes\n"
    "    mcast_port 56363\n"
    "    mcast_group 224.0.23.170\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionUdpBadIdleTimeout)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    idle_timeout hello\n"
    "  }\n"
    "}\n";

  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "Invalid value for option \"idle_timeout\" in \"udp\" section"));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionUdpBadMcast)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    mcast hello\n"
    "  }\n"
    "}\n";

  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "Invalid value for option \"mcast\" in \"udp\" section"));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionUdpBadMcastGroup)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    mcast no\n"
    "    mcast_port 50\n"
    "    mcast_group hello\n"
    "  }\n"
    "}\n";

  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "Invalid value for option \"mcast_group\" in \"udp\" section"));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionUdpBadMcastGroupV6)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    mcast no\n"
    "    mcast_port 50\n"
    "    mcast_group ::1\n"
    "  }\n"
    "}\n";

  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "Invalid value for option \"mcast_group\" in \"udp\" section"));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionUdpUnknownOption)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "Unrecognized option \"hello\" in \"udp\" section"));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionEther)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    mcast yes\n"
    "    mcast_group 01:00:5E:00:17:AA\n"
    "  }\n"
    "}\n";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionEtherDryRun)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    mcast yes\n"
    "    mcast_group 01:00:5E:00:17:AA\n"
    "  }\n"
    "}\n";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionEtherBadMcast)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    mcast hello\n"
    "  }\n"
    "}\n";

  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "Invalid value for option \"mcast\" in \"ether\" section"));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionEtherBadMcastGroup)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    mcast yes\n"
    "    mcast_group\n"
    "  }\n"
    "}\n";

  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "Invalid value for option \"mcast_group\" in \"ether\" section"));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionEtherUnknownOption)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    hello\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "Unrecognized option \"hello\" in \"ether\" section"));
}

BOOST_AUTO_TEST_CASE(TestFireInterestFilter)
{
  shared_ptr<Interest> command(make_shared<Interest>("/localhost/nfd/faces"));

  getFace()->onReceiveData +=
    bind(&FaceManagerFixture::validateControlResponse, this,  _1,
         command->getName(), 400, "Malformed command");

  getFace()->sendInterest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(MalformedCommmand)
{
  shared_ptr<Interest> command(make_shared<Interest>("/localhost/nfd/faces"));

  getFace()->onReceiveData +=
    bind(&FaceManagerFixture::validateControlResponse, this, _1,
         command->getName(), 400, "Malformed command");

  getManager().onFaceRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

/// \todo add tests for unsigned and unauthorized commands

BOOST_AUTO_TEST_CASE(UnsupportedVerb)
{
  ndn::nfd::FaceManagementOptions options;

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("unsupported");
  commandName.append(encodedOptions);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  getFace()->onReceiveData +=
    bind(&FaceManagerFixture::validateControlResponse, this, _1,
         command->getName(), 501, "Unsupported command");

  getManager().onFaceRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

class ValidatedFaceRequestFixture : public TestFaceTableFixture,
                                    public TestFaceManagerCommon,
                                    public FaceManager
{
public:

  ValidatedFaceRequestFixture()
    : FaceManager(TestFaceTableFixture::m_faceTable, TestFaceManagerCommon::m_face),
      m_createFaceFired(false),
      m_destroyFaceFired(false)
  {

  }

  virtual void
  createFace(const Name& requestName,
             ndn::nfd::FaceManagementOptions& options)
  {
    m_createFaceFired = true;
  }

  virtual void
  destroyFace(const Name& requestName,
              ndn::nfd::FaceManagementOptions& options)
  {
    m_destroyFaceFired = true;
  }

  virtual
  ~ValidatedFaceRequestFixture()
  {

  }

  bool
  didCreateFaceFire() const
  {
    return m_createFaceFired;
  }

  bool
  didDestroyFaceFire() const
  {
    return m_destroyFaceFired;
  }

private:
  bool m_createFaceFired;
  bool m_destroyFaceFired;
};

BOOST_FIXTURE_TEST_CASE(ValidatedFaceRequestBadOptionParse, ValidatedFaceRequestFixture)
{
  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append("NotReallyOptions");

  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  getFace()->onReceiveData +=
    bind(&ValidatedFaceRequestFixture::validateControlResponse, this, _1,
         command->getName(), 400, "Malformed command");

  onValidatedFaceRequest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(ValidatedFaceRequestCreateFace, ValidatedFaceRequestFixture)
{
  ndn::nfd::FaceManagementOptions options;
  options.setUri("tcp://127.0.0.1");

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedOptions);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  onValidatedFaceRequest(command);
  BOOST_CHECK(didCreateFaceFire());
}

BOOST_FIXTURE_TEST_CASE(ValidatedFaceRequestDestroyFace, ValidatedFaceRequestFixture)
{
  ndn::nfd::FaceManagementOptions options;
  options.setUri("tcp://127.0.0.1");

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("destroy");
  commandName.append(encodedOptions);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  onValidatedFaceRequest(command);
  BOOST_CHECK(didDestroyFaceFire());
}

class FaceFixture : public TestFaceTableFixture,
                          public TestFaceManagerCommon,
                          public FaceManager
{
public:
  FaceFixture()
    : FaceManager(TestFaceTableFixture::m_faceTable, TestFaceManagerCommon::m_face)
  {

  }

  virtual
  ~FaceFixture()
  {

  }
};

BOOST_FIXTURE_TEST_CASE(CreateFaceBadUri, FaceFixture)
{
  ndn::nfd::FaceManagementOptions options;
  options.setUri("tcp:/127.0.0.1");

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedOptions);

  getFace()->onReceiveData +=
    bind(&FaceFixture::validateControlResponse, this, _1,
         commandName, 400, "Malformed command");

  createFace(commandName, options);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(CreateFaceUnknownScheme, FaceFixture)
{
  ndn::nfd::FaceManagementOptions options;
  // this will be an unsupported protocol because no factories have been
  // added to the face manager
  options.setUri("tcp://127.0.0.1");

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedOptions);

  getFace()->onReceiveData +=
    bind(&FaceFixture::validateControlResponse, this, _1,
         commandName, 501, "Unsupported protocol");

  createFace(commandName, options);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(OnCreated, FaceFixture)
{
  ndn::nfd::FaceManagementOptions options;
  options.setUri("tcp://127.0.0.1");

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedOptions);

  ndn::nfd::FaceManagementOptions resultOptions;
  resultOptions.setUri("tcp://127.0.0.1");
  resultOptions.setFaceId(-1);

  Block encodedResultOptions(resultOptions.wireEncode());

  getFace()->onReceiveData +=
    bind(&FaceFixture::validateControlResponse, this, _1,
         commandName, 200, "Success", encodedResultOptions);

  onCreated(commandName, options, make_shared<DummyFace>());

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK(TestFaceTableFixture::m_faceTable.didAddFire());
}

BOOST_FIXTURE_TEST_CASE(OnConnectFailed, FaceFixture)
{
    ndn::nfd::FaceManagementOptions options;
  options.setUri("tcp://127.0.0.1");

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedOptions);

  getFace()->onReceiveData +=
    bind(&FaceFixture::validateControlResponse, this, _1,
         commandName, 400, "Failed to create face");

  onConnectFailed(commandName, "unit-test-reason");

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK_EQUAL(TestFaceTableFixture::m_faceTable.didAddFire(), false);
}


BOOST_FIXTURE_TEST_CASE(DestroyFace, FaceFixture)
{
  ndn::nfd::FaceManagementOptions options;
  options.setUri("tcp://127.0.0.1");

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("destroy");
  commandName.append(encodedOptions);

  getFace()->onReceiveData +=
    bind(&FaceFixture::validateControlResponse, this, _1,
         commandName, 200, "Success");

  destroyFace(commandName, options);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK(TestFaceTableFixture::m_faceTable.didRemoveFire());
  BOOST_CHECK(TestFaceTableFixture::m_faceTable.getDummyFace()->didCloseFire());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd


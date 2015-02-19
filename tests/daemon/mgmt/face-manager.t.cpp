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

#include "mgmt/face-manager.hpp"
#include "mgmt/internal-face.hpp"
#include "mgmt/face-status-publisher.hpp"

#include "face/face.hpp"
#include "../face/dummy-face.hpp"
#include "fw/face-table.hpp"
#include "fw/forwarder.hpp"
#include "face/udp-factory.hpp"

#ifdef HAVE_LIBPCAP
#include "face/ethernet-factory.hpp"
#endif // HAVE_LIBPCAP

#include "common.hpp"
#include "tests/test-common.hpp"
#include "validation-common.hpp"
#include "face-status-publisher-common.hpp"
#include "face-query-status-publisher-common.hpp"
#include "channel-status-common.hpp"

#include <ndn-cxx/encoding/tlv.hpp>
#include <ndn-cxx/management/nfd-face-event-notification.hpp>

namespace nfd {
namespace tests {

NFD_LOG_INIT("FaceManagerTest");

class FaceManagerTestFace : public DummyFace
{
public:

  FaceManagerTestFace()
    : m_closeFired(false)
  {

  }

  virtual
  ~FaceManagerTestFace()
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
      m_getFired(false),
      m_dummy(make_shared<FaceManagerTestFace>())
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
  didGetFire() const
  {
    return m_getFired;
  }

  void
  reset()
  {
    m_addFired = false;
    m_getFired = false;
  }

  shared_ptr<FaceManagerTestFace>&
  getDummyFace()
  {
    return m_dummy;
  }

private:
  bool m_addFired;
  mutable bool m_getFired;
  shared_ptr<FaceManagerTestFace> m_dummy;
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
  bool m_callbackFired;
  ndn::KeyChain m_testKeyChain;
};

class FaceManagerFixture : public TestFaceTableFixture, public TestFaceManagerCommon
{
public:
  FaceManagerFixture()
    : m_manager(m_faceTable, m_face, m_testKeyChain)
  {
    m_manager.setConfigFile(m_config);
  }

  virtual
  ~FaceManagerFixture()
  {

  }

  void
  parseConfig(const std::string configuration, bool isDryRun)
  {
    m_config.parse(configuration, isDryRun, "dummy-config");
  }

  FaceManager&
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

  bool
  didFaceTableAddFire() const
  {
    return m_faceTable.didAddFire();
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

protected:
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

#ifdef HAVE_UNIX_SOCKETS

BOOST_AUTO_TEST_CASE(TestProcessSectionUnix)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  unix\n"
    "  {\n"
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
    "    path /var/run/nfd.sock\n"
    "  }\n"
    "}\n";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
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

#endif // HAVE_UNIX_SOCKETS


BOOST_AUTO_TEST_CASE(TestProcessSectionTcp)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  tcp\n"
    "  {\n"
    "    listen yes\n"
    "    port 6363\n"
    "    enable_v4 yes\n"
    "    enable_v6 yes\n"
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
    "    enable_v4 yes\n"
    "    enable_v6 yes\n"
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

BOOST_AUTO_TEST_CASE(TestProcessSectionTcpChannelsDisabled)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  tcp\n"
    "  {\n"
    "    port 6363\n"
    "    enable_v4 no\n"
    "    enable_v6 no\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "IPv4 and IPv6 channels have been disabled."
                             " Remove \"tcp\" section to disable TCP channels or"
                             " re-enable at least one channel type."));
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
    "    enable_v4 yes\n"
    "    enable_v6 yes\n"
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

BOOST_AUTO_TEST_CASE(TestProcessSectionUdpChannelsDisabled)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    port 6363\n"
    "    enable_v4 no\n"
    "    enable_v6 no\n"
    "    idle_timeout 30\n"
    "    keep_alive_interval 25\n"
    "    mcast yes\n"
    "    mcast_port 56363\n"
    "    mcast_group 224.0.23.170\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "IPv4 and IPv6 channels have been disabled."
                             " Remove \"udp\" section to disable UDP channels or"
                             " re-enable at least one channel type."));
}

BOOST_AUTO_TEST_CASE(TestProcessSectionUdpConflictingMcast)
{
  const std::string CONFIG =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    port 6363\n"
    "    enable_v4 no\n"
    "    enable_v6 yes\n"
    "    idle_timeout 30\n"
    "    keep_alive_interval 25\n"
    "    mcast yes\n"
    "    mcast_port 56363\n"
    "    mcast_group 224.0.23.170\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_EXCEPTION(parseConfig(CONFIG, false), ConfigFile::Error,
                        bind(&isExpectedException, _1,
                             "IPv4 multicast requested, but IPv4 channels"
                             " have been disabled (conflicting configuration options set)"));
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


BOOST_AUTO_TEST_CASE(TestProcessSectionUdpMulticastReinit)
{
  const std::string CONFIG_WITH_MCAST =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    mcast yes\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG_WITH_MCAST, false));

  shared_ptr<UdpFactory> factory = static_pointer_cast<UdpFactory>(getManager().findFactory("udp"));
  BOOST_REQUIRE(static_cast<bool>(factory));

  if (factory->getMulticastFaces().size() == 0) {
    BOOST_TEST_MESSAGE("Destroying multicast faces is not tested because "
                       "no UDP multicast faces are available");
  }

  const std::string CONFIG_WITHOUT_MCAST =
    "face_system\n"
    "{\n"
    "  udp\n"
    "  {\n"
    "    mcast no\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG_WITHOUT_MCAST, false));
  BOOST_CHECK_EQUAL(factory->getMulticastFaces().size(), 0);
}


#ifdef HAVE_LIBPCAP

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

BOOST_AUTO_TEST_CASE(TestProcessSectionEtherMulticastReinit)
{
  const std::string CONFIG_WITH_MCAST =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    mcast yes\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG_WITH_MCAST, false));

  shared_ptr<EthernetFactory> factory =
    static_pointer_cast<EthernetFactory>(getManager().findFactory("ether"));
  BOOST_REQUIRE(static_cast<bool>(factory));

  if (factory->getMulticastFaces().size() == 0) {
    BOOST_TEST_MESSAGE("Destroying multicast faces is not tested because "
                       "no Ethernet multicast faces are available");
  }

  const std::string CONFIG_WITHOUT_MCAST =
    "face_system\n"
    "{\n"
    "  ether\n"
    "  {\n"
    "    mcast no\n"
    "  }\n"
    "}\n";
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG_WITHOUT_MCAST, false));
  BOOST_CHECK_EQUAL(factory->getMulticastFaces().size(), 0);
}

#endif // HAVE_LIBPCAP

BOOST_AUTO_TEST_CASE(ShortName)
{
  shared_ptr<Interest> command(make_shared<Interest>("/localhost/nfd/faces"));

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  getFace()->sendInterest(*command);
  g_io.run_one();

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(MalformedCommmand)
{
  shared_ptr<Interest> command(make_shared<Interest>("/localhost/nfd/faces"));

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  getManager().onFaceRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(UnsignedCommand)
{
  ControlParameters parameters;
  parameters.setUri("tcp4://127.0.0.1:6363");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 401, "Signature required");
  });

  getManager().onFaceRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(UnauthorizedCommand, UnauthorizedCommandFixture<FaceManagerFixture>)
{
  ControlParameters parameters;
  parameters.setUri("tcp4://127.0.0.1:6363");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 403, "Unauthorized command");
  });

  getManager().onFaceRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

template <typename T> class AuthorizedCommandFixture : public CommandFixture<T>
{
public:
  AuthorizedCommandFixture()
  {
    const std::string regex = "^<localhost><nfd><faces>";
    T::addInterestRule(regex, *CommandFixture<T>::m_certificate);
  }

  virtual
  ~AuthorizedCommandFixture()
  {

  }
};

BOOST_FIXTURE_TEST_CASE(UnsupportedCommand, AuthorizedCommandFixture<FaceManagerFixture>)
{
  ControlParameters parameters;

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("unsupported");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 501, "Unsupported command");
  });

  getManager().onFaceRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

class ValidatedFaceRequestFixture : public TestFaceTableFixture,
                                    public TestFaceManagerCommon,
                                    public FaceManager
{
public:

  ValidatedFaceRequestFixture()
    : FaceManager(TestFaceTableFixture::m_faceTable, getFace(), m_testKeyChain),
      m_createFaceFired(false),
      m_destroyFaceFired(false)
  {

  }

  virtual void
  createFace(const Interest& request,
             ControlParameters& parameters)
  {
    m_createFaceFired = true;
  }

  virtual void
  destroyFace(const Interest& request,
              ControlParameters& parameters)
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

BOOST_FIXTURE_TEST_CASE(ValidatedFaceRequestBadOptionParse,
                        AuthorizedCommandFixture<ValidatedFaceRequestFixture>)
{
  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append("NotReallyParameters");

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  onValidatedFaceRequest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(ValidatedFaceRequestCreateFace,
                        AuthorizedCommandFixture<ValidatedFaceRequestFixture>)
{
  ControlParameters parameters;
  parameters.setUri("tcp4://127.0.0.1:6363");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  onValidatedFaceRequest(command);
  BOOST_CHECK(didCreateFaceFire());
}

BOOST_FIXTURE_TEST_CASE(ValidatedFaceRequestDestroyFace,
                        AuthorizedCommandFixture<ValidatedFaceRequestFixture>)
{
  ControlParameters parameters;
  parameters.setUri("tcp4://127.0.0.1:6363");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("destroy");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  onValidatedFaceRequest(command);
  BOOST_CHECK(didDestroyFaceFire());
}

class FaceTableFixture
{
public:
  FaceTableFixture()
    : m_faceTable(m_forwarder)
  {
  }

  virtual
  ~FaceTableFixture()
  {
  }

protected:
  Forwarder m_forwarder;
  FaceTable m_faceTable;
};

class LocalControlFixture : public FaceTableFixture,
                            public TestFaceManagerCommon,
                            public FaceManager
{
public:
  LocalControlFixture()
    : FaceManager(FaceTableFixture::m_faceTable, getFace(), m_testKeyChain)
  {
  }
};

BOOST_FIXTURE_TEST_CASE(LocalControlInFaceId,
                        AuthorizedCommandFixture<LocalControlFixture>)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  BOOST_REQUIRE(dummy->isLocal());
  FaceTableFixture::m_faceTable.add(dummy);

  ControlParameters parameters;
  parameters.setLocalControlFeature(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID);

  Block encodedParameters(parameters.wireEncode());

  Name enable("/localhost/nfd/faces/enable-local-control");
  enable.append(encodedParameters);

  shared_ptr<Interest> enableCommand(make_shared<Interest>(enable));
  enableCommand->setIncomingFaceId(dummy->getId());

  generateCommand(*enableCommand);

  signal::Connection conn = getFace()->onReceiveData.connect(
      [this, enableCommand, encodedParameters] (const Data& response) {
        this->validateControlResponse(response, enableCommand->getName(),
                                      200, "Success", encodedParameters);
      });

  onValidatedFaceRequest(enableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID));

  conn.disconnect();
  resetCallbackFired();

  Name disable("/localhost/nfd/faces/disable-local-control");
  disable.append(encodedParameters);

  shared_ptr<Interest> disableCommand(make_shared<Interest>(disable));
  disableCommand->setIncomingFaceId(dummy->getId());

  generateCommand(*disableCommand);

  getFace()->onReceiveData.connect(
      [this, disableCommand, encodedParameters] (const Data& response) {
        this->validateControlResponse(response, disableCommand->getName(),
                                      200, "Success", encodedParameters);
      });

  onValidatedFaceRequest(disableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID));
}

BOOST_FIXTURE_TEST_CASE(LocalControlInFaceIdFaceNotFound,
                        AuthorizedCommandFixture<LocalControlFixture>)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  BOOST_REQUIRE(dummy->isLocal());
  FaceTableFixture::m_faceTable.add(dummy);

  ControlParameters parameters;
  parameters.setLocalControlFeature(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID);

  Block encodedParameters(parameters.wireEncode());

  Name enable("/localhost/nfd/faces/enable-local-control");
  enable.append(encodedParameters);

  shared_ptr<Interest> enableCommand(make_shared<Interest>(enable));
  enableCommand->setIncomingFaceId(dummy->getId() + 100);

  generateCommand(*enableCommand);

  signal::Connection conn = getFace()->onReceiveData.connect(
      [this, enableCommand] (const Data& response) {
        this->validateControlResponse(response, enableCommand->getName(), 410, "Face not found");
      });

  onValidatedFaceRequest(enableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID));

  conn.disconnect();
  resetCallbackFired();

  Name disable("/localhost/nfd/faces/disable-local-control");
  disable.append(encodedParameters);

  shared_ptr<Interest> disableCommand(make_shared<Interest>(disable));
  disableCommand->setIncomingFaceId(dummy->getId() + 100);

  generateCommand(*disableCommand);

  getFace()->onReceiveData.connect(
      [this, disableCommand] (const Data& response) {
        this->validateControlResponse(response, disableCommand->getName(), 410, "Face not found");
      });

  onValidatedFaceRequest(disableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID));
}

BOOST_FIXTURE_TEST_CASE(LocalControlMissingFeature,
                        AuthorizedCommandFixture<LocalControlFixture>)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  BOOST_REQUIRE(dummy->isLocal());
  FaceTableFixture::m_faceTable.add(dummy);

  ControlParameters parameters;

  Block encodedParameters(parameters.wireEncode());

  Name enable("/localhost/nfd/faces/enable-local-control");
  enable.append(encodedParameters);

  shared_ptr<Interest> enableCommand(make_shared<Interest>(enable));
  enableCommand->setIncomingFaceId(dummy->getId());

  generateCommand(*enableCommand);

  signal::Connection conn = getFace()->onReceiveData.connect(
      [this, enableCommand] (const Data& response) {
        this->validateControlResponse(response, enableCommand->getName(),
                                      400, "Malformed command");
      });

  onValidatedFaceRequest(enableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID));
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID));

  conn.disconnect();
  resetCallbackFired();

  Name disable("/localhost/nfd/faces/disable-local-control");
  disable.append(encodedParameters);

  shared_ptr<Interest> disableCommand(make_shared<Interest>(disable));
  disableCommand->setIncomingFaceId(dummy->getId());

  generateCommand(*disableCommand);

  getFace()->onReceiveData.connect(
      [this, disableCommand] (const Data& response) {
        this->validateControlResponse(response, disableCommand->getName(),
                                      400, "Malformed command");
      });

  onValidatedFaceRequest(disableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID));
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID));
}

BOOST_FIXTURE_TEST_CASE(LocalControlInFaceIdNonLocal,
                        AuthorizedCommandFixture<LocalControlFixture>)
{
  shared_ptr<DummyFace> dummy = make_shared<DummyFace>();
  BOOST_REQUIRE(!dummy->isLocal());
  FaceTableFixture::m_faceTable.add(dummy);

  ControlParameters parameters;
  parameters.setLocalControlFeature(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID);

  Block encodedParameters(parameters.wireEncode());

  Name enable("/localhost/nfd/faces/enable-local-control");
  enable.append(encodedParameters);

  shared_ptr<Interest> enableCommand(make_shared<Interest>(enable));
  enableCommand->setIncomingFaceId(dummy->getId());

  generateCommand(*enableCommand);

  signal::Connection conn = getFace()->onReceiveData.connect(
      [this, enableCommand] (const Data& response) {
        this->validateControlResponse(response, enableCommand->getName(),
                                      412, "Face is non-local");
      });

  onValidatedFaceRequest(enableCommand);

  BOOST_REQUIRE(didCallbackFire());

  conn.disconnect();
  resetCallbackFired();

  Name disable("/localhost/nfd/faces/disable-local-control");
  enable.append(encodedParameters);

  shared_ptr<Interest> disableCommand(make_shared<Interest>(enable));
  disableCommand->setIncomingFaceId(dummy->getId());

  generateCommand(*disableCommand);

  getFace()->onReceiveData.connect(
      [this, disableCommand] (const Data& response) {
        this->validateControlResponse(response, disableCommand->getName(),
                                      412, "Face is non-local");
      });

  onValidatedFaceRequest(disableCommand);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(LocalControlNextHopFaceId,
                        AuthorizedCommandFixture<LocalControlFixture>)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  BOOST_REQUIRE(dummy->isLocal());
  FaceTableFixture::m_faceTable.add(dummy);

  ControlParameters parameters;
  parameters.setLocalControlFeature(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID);

  Block encodedParameters(parameters.wireEncode());

  Name enable("/localhost/nfd/faces/enable-local-control");
  enable.append(encodedParameters);

  shared_ptr<Interest> enableCommand(make_shared<Interest>(enable));
  enableCommand->setIncomingFaceId(dummy->getId());

  generateCommand(*enableCommand);

  signal::Connection conn = getFace()->onReceiveData.connect(
      [this, enableCommand, encodedParameters] (const Data& response) {
        this->validateControlResponse(response, enableCommand->getName(),
                                      200, "Success", encodedParameters);
      });

  onValidatedFaceRequest(enableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID));


  conn.disconnect();
  resetCallbackFired();

  Name disable("/localhost/nfd/faces/disable-local-control");
  disable.append(encodedParameters);

  shared_ptr<Interest> disableCommand(make_shared<Interest>(disable));
  disableCommand->setIncomingFaceId(dummy->getId());

  generateCommand(*disableCommand);

  getFace()->onReceiveData.connect(
      [this, disableCommand, encodedParameters] (const Data& response) {
        this->validateControlResponse(response, disableCommand->getName(),
                                      200, "Success", encodedParameters);
      });

  onValidatedFaceRequest(disableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID));
}

BOOST_FIXTURE_TEST_CASE(LocalControlNextHopFaceIdFaceNotFound,
                        AuthorizedCommandFixture<LocalControlFixture>)
{
  shared_ptr<LocalFace> dummy = make_shared<DummyLocalFace>();
  BOOST_REQUIRE(dummy->isLocal());
  FaceTableFixture::m_faceTable.add(dummy);

  ControlParameters parameters;
  parameters.setLocalControlFeature(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID);

  Block encodedParameters(parameters.wireEncode());

  Name enable("/localhost/nfd/faces/enable-local-control");
  enable.append(encodedParameters);

  shared_ptr<Interest> enableCommand(make_shared<Interest>(enable));
  enableCommand->setIncomingFaceId(dummy->getId() + 100);

  generateCommand(*enableCommand);

  signal::Connection conn = getFace()->onReceiveData.connect(
      [this, enableCommand] (const Data& response) {
        this->validateControlResponse(response, enableCommand->getName(), 410, "Face not found");
      });

  onValidatedFaceRequest(enableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID));


  conn.disconnect();
  resetCallbackFired();

  Name disable("/localhost/nfd/faces/disable-local-control");
  disable.append(encodedParameters);

  shared_ptr<Interest> disableCommand(make_shared<Interest>(disable));
  disableCommand->setIncomingFaceId(dummy->getId() + 100);

  generateCommand(*disableCommand);

  getFace()->onReceiveData.connect(
      [this, disableCommand] (const Data& response) {
        this->validateControlResponse(response, disableCommand->getName(),
                                      410, "Face not found");
      });

  onValidatedFaceRequest(disableCommand);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID));
  BOOST_CHECK(!dummy->isLocalControlHeaderEnabled(LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID));
}

BOOST_FIXTURE_TEST_CASE(LocalControlNextHopFaceIdNonLocal,
                        AuthorizedCommandFixture<LocalControlFixture>)
{
  shared_ptr<DummyFace> dummy = make_shared<DummyFace>();
  BOOST_REQUIRE(!dummy->isLocal());
  FaceTableFixture::m_faceTable.add(dummy);

  ControlParameters parameters;
  parameters.setLocalControlFeature(LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID);

  Block encodedParameters(parameters.wireEncode());

  Name enable("/localhost/nfd/faces/enable-local-control");
  enable.append(encodedParameters);

  shared_ptr<Interest> enableCommand(make_shared<Interest>(enable));
  enableCommand->setIncomingFaceId(dummy->getId());

  generateCommand(*enableCommand);

  signal::Connection conn = getFace()->onReceiveData.connect(
      [this, enableCommand] (const Data& response) {
        this->validateControlResponse(response, enableCommand->getName(),
                                      412, "Face is non-local");
      });

  onValidatedFaceRequest(enableCommand);

  BOOST_REQUIRE(didCallbackFire());

  conn.disconnect();
  resetCallbackFired();

  Name disable("/localhost/nfd/faces/disable-local-control");
  disable.append(encodedParameters);

  shared_ptr<Interest> disableCommand(make_shared<Interest>(disable));
  disableCommand->setIncomingFaceId(dummy->getId());

  generateCommand(*disableCommand);

  getFace()->onReceiveData.connect(
      [this, disableCommand] (const Data& response) {
        this->validateControlResponse(response, disableCommand->getName(),
                                      412, "Face is non-local");
      });

  onValidatedFaceRequest(disableCommand);

  BOOST_REQUIRE(didCallbackFire());
}

class FaceFixture : public FaceTableFixture,
                    public TestFaceManagerCommon,
                    public FaceManager
{
public:
  FaceFixture()
    : FaceManager(FaceTableFixture::m_faceTable,
                  getFace(),
                  m_testKeyChain)
    , m_receivedNotification(false)
  {

  }

  virtual
  ~FaceFixture()
  {

  }

  void
  callbackDispatch(const Data& response,
                   const Name& expectedName,
                   uint32_t expectedCode,
                   const std::string& expectedText,
                   const Block& expectedBody,
                   const ndn::nfd::FaceEventNotification& expectedFaceEvent)
  {
    Block payload = response.getContent().blockFromValue();
    if (payload.type() == ndn::tlv::nfd::ControlResponse)
      {
        validateControlResponse(response, expectedName, expectedCode,
                                expectedText, expectedBody);
      }
    else if (payload.type() == ndn::tlv::nfd::FaceEventNotification)
      {
        validateFaceEvent(payload, expectedFaceEvent);
      }
    else
      {
        BOOST_FAIL("Received unknown message type: #" << payload.type());
      }
  }

  void
  callbackDispatch(const Data& response,
                   const Name& expectedName,
                   uint32_t expectedCode,
                   const std::string& expectedText,
                   const ndn::nfd::FaceEventNotification& expectedFaceEvent)
  {
    Block payload = response.getContent().blockFromValue();
    if (payload.type() == ndn::tlv::nfd::ControlResponse)
      {
        validateControlResponse(response, expectedName,
                                expectedCode, expectedText);
      }
    else if (payload.type() == ndn::tlv::nfd::FaceEventNotification)
      {
        validateFaceEvent(payload, expectedFaceEvent);
      }
    else
      {
        BOOST_FAIL("Received unknown message type: #" << payload.type());
      }
  }

  void
  validateFaceEvent(const Block& wire,
                    const ndn::nfd::FaceEventNotification& expectedFaceEvent)
  {

    m_receivedNotification = true;

    ndn::nfd::FaceEventNotification notification(wire);

    BOOST_CHECK_EQUAL(notification.getKind(), expectedFaceEvent.getKind());
    BOOST_CHECK_EQUAL(notification.getFaceId(), expectedFaceEvent.getFaceId());
    BOOST_CHECK_EQUAL(notification.getRemoteUri(), expectedFaceEvent.getRemoteUri());
    BOOST_CHECK_EQUAL(notification.getLocalUri(), expectedFaceEvent.getLocalUri());
    BOOST_CHECK_EQUAL(notification.getFaceScope(), expectedFaceEvent.getFaceScope());
    BOOST_CHECK_EQUAL(notification.getFacePersistency(), expectedFaceEvent.getFacePersistency());
    BOOST_CHECK_EQUAL(notification.getLinkType(), expectedFaceEvent.getLinkType());
  }

  bool
  didReceiveNotication() const
  {
    return m_receivedNotification;
  }

protected:
  bool m_receivedNotification;
};

BOOST_FIXTURE_TEST_CASE(CreateFaceBadUri, AuthorizedCommandFixture<FaceFixture>)
{
  ControlParameters parameters;
  parameters.setUri("tcp4:/127.0.0.1:6363");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  createFace(*command, parameters);

  BOOST_REQUIRE(didCallbackFire());
}
BOOST_FIXTURE_TEST_CASE(CreateFaceNoncanonicalUri, AuthorizedCommandFixture<FaceFixture>)
{
  ControlParameters parameters;
  parameters.setUri("tcp://127.0.0.1");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Non-canonical URI");
  });

  createFace(*command, parameters);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(CreateFaceMissingUri, AuthorizedCommandFixture<FaceFixture>)
{
  ControlParameters parameters;

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  createFace(*command, parameters);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(CreateFaceUnknownScheme, AuthorizedCommandFixture<FaceFixture>)
{
  ControlParameters parameters;
  // this will be an unsupported protocol because no factories have been
  // added to the face manager
  parameters.setUri("tcp4://127.0.0.1:6363");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 501, "Unsupported protocol");
  });

  createFace(*command, parameters);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(OnCreated, AuthorizedCommandFixture<FaceFixture>)
{
  ControlParameters parameters;
  parameters.setUri("tcp4://127.0.0.1:6363");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  ControlParameters resultParameters;
  resultParameters.setUri("dummy://");
  resultParameters.setFaceId(FACEID_RESERVED_MAX + 1);

  shared_ptr<DummyFace> dummy(make_shared<DummyFace>());

  ndn::nfd::FaceEventNotification expectedFaceEvent;
  expectedFaceEvent.setKind(ndn::nfd::FACE_EVENT_CREATED)
                   .setFaceId(FACEID_RESERVED_MAX + 1)
                   .setRemoteUri(dummy->getRemoteUri().toString())
                   .setLocalUri(dummy->getLocalUri().toString())
                   .setFaceScope(ndn::nfd::FACE_SCOPE_NON_LOCAL)
                   .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT);

  Block encodedResultParameters(resultParameters.wireEncode());

  getFace()->onReceiveData.connect(
      [this, command, encodedResultParameters, expectedFaceEvent] (const Data& response) {
        this->callbackDispatch(response,command->getName(), 200, "Success",
                               encodedResultParameters, expectedFaceEvent);
      });

  onCreated(command->getName(), parameters, dummy);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(didReceiveNotication());
}

BOOST_FIXTURE_TEST_CASE(OnConnectFailed, AuthorizedCommandFixture<FaceFixture>)
{
  ControlParameters parameters;
  parameters.setUri("tcp4://127.0.0.1:6363");

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  getFace()->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 408, "unit-test-reason");
  });

  onConnectFailed(command->getName(), "unit-test-reason");

  BOOST_REQUIRE(didCallbackFire());
  BOOST_CHECK_EQUAL(didReceiveNotication(), false);
}


BOOST_FIXTURE_TEST_CASE(DestroyFace, AuthorizedCommandFixture<FaceFixture>)
{
  shared_ptr<DummyFace> dummy(make_shared<DummyFace>());
  FaceTableFixture::m_faceTable.add(dummy);

  ControlParameters parameters;
  parameters.setFaceId(dummy->getId());

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/faces");
  commandName.append("destroy");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  ndn::nfd::FaceEventNotification expectedFaceEvent;
  expectedFaceEvent.setKind(ndn::nfd::FACE_EVENT_DESTROYED)
                   .setFaceId(dummy->getId())
                   .setRemoteUri(dummy->getRemoteUri().toString())
                   .setLocalUri(dummy->getLocalUri().toString())
                   .setFaceScope(ndn::nfd::FACE_SCOPE_NON_LOCAL)
                   .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT);

  getFace()->onReceiveData.connect(
      [this, command, encodedParameters, expectedFaceEvent] (const Data& response) {
        this->callbackDispatch(response,command->getName(), 200, "Success",
                               encodedParameters, expectedFaceEvent);
      });

  destroyFace(*command, parameters);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(didReceiveNotication());
}

class FaceListFixture : public FaceStatusPublisherFixture
{
public:
  FaceListFixture()
    : m_manager(m_table, m_face, m_testKeyChain)
  {

  }

  virtual
  ~FaceListFixture()
  {

  }

protected:
  FaceManager m_manager;
  ndn::KeyChain m_testKeyChain;
};

BOOST_FIXTURE_TEST_CASE(TestFaceList, FaceListFixture)
{
  Name commandName("/localhost/nfd/faces/list");
  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  // MAX_SEGMENT_SIZE == 4400, FaceStatus size with filler counters is 75
  // use 59 FaceStatuses to force a FaceStatus to span Data packets
  for (int i = 0; i < 59; i++)
    {
      shared_ptr<TestCountersFace> dummy(make_shared<TestCountersFace>());

      uint64_t filler = std::numeric_limits<uint64_t>::max() - 1;
      dummy->setCounters(filler, filler, filler, filler, filler, filler);

      m_referenceFaces.push_back(dummy);

      add(dummy);
    }

  ndn::EncodingBuffer buffer;

  m_face->onReceiveData.connect(bind(&FaceStatusPublisherFixture::decodeFaceStatusBlock,
                                     this, _1));

  m_manager.listFaces(*command);
  BOOST_REQUIRE(m_finished);
}

class ChannelStatusFixture : public FaceManagerFixture
{
public:
  void
  validatePublish(const Data& data, const ndn::nfd::ChannelStatus& expectedEntry)
  {
    m_callbackFired = true;
    Block b = data.getContent().blockFromValue();
    ndn::nfd::ChannelStatus entry(b);
    BOOST_CHECK_EQUAL(entry.getLocalUri(), expectedEntry.getLocalUri());
  }

  virtual shared_ptr<DummyProtocolFactory>
  addProtocolFactory(const std::string& protocol)
  {
    shared_ptr<DummyProtocolFactory> factory(make_shared<DummyProtocolFactory>());
    m_manager.m_factories[protocol] = factory;

    return factory;
  }
};

BOOST_FIXTURE_TEST_CASE(TestChannelStatus, ChannelStatusFixture)
{
  shared_ptr<DummyProtocolFactory> factory(addProtocolFactory("dummy"));
  factory->addChannel("dummy://");

  Name requestName("/localhost/nfd/faces/channels");
  shared_ptr<Interest> request(make_shared<Interest>(requestName));

  ndn::nfd::ChannelStatus expectedEntry;
  expectedEntry.setLocalUri(DummyChannel("dummy://").getUri().toString());

  m_face->onReceiveData.connect(bind(&ChannelStatusFixture::validatePublish,
                                     this, _1, expectedEntry));

  m_manager.listChannels(*request);
  BOOST_REQUIRE(m_callbackFired);
}

class FaceQueryListFixture : public FaceQueryStatusPublisherFixture
{
public:
  FaceQueryListFixture()
    : m_manager(m_table, m_face, m_testKeyChain)
  {

  }

  virtual
  ~FaceQueryListFixture()
  {

  }

protected:
  FaceManager m_manager;
  ndn::KeyChain m_testKeyChain;
};

BOOST_FIXTURE_TEST_CASE(TestValidQueryFilter, FaceQueryListFixture)
{
  Name queryName("/localhost/nfd/faces/query");
  ndn::nfd::FaceQueryFilter queryFilter;
  queryFilter.setUriScheme("dummy");
  queryName.append(queryFilter.wireEncode());

  shared_ptr<Interest> query(make_shared<Interest>(queryName));

  // add expected faces
  shared_ptr<DummyLocalFace> expectedFace1(make_shared<DummyLocalFace>());
  m_referenceFaces.push_back(expectedFace1);
  add(expectedFace1);

  shared_ptr<DummyFace> expectedFace2(make_shared<DummyFace>());
  m_referenceFaces.push_back(expectedFace2);
  add(expectedFace2);

  // add other faces
  shared_ptr<DummyFace> face1(make_shared<DummyFace>("udp://", "udp://"));
  add(face1);
  shared_ptr<DummyLocalFace> face2(make_shared<DummyLocalFace>("tcp://", "tcp://"));
  add(face2);

  m_face->onReceiveData.connect(bind(&FaceQueryStatusPublisherFixture::decodeFaceStatusBlock,
                                     this, _1));

  m_manager.listQueriedFaces(*query);
  BOOST_REQUIRE(m_finished);
}

BOOST_FIXTURE_TEST_CASE(TestInvalidQueryFilter, FaceQueryListFixture)
{
  Name queryName("/localhost/nfd/faces/query");
  ndn::nfd::FaceStatus queryFilter;
  queryName.append(queryFilter.wireEncode());

  shared_ptr<Interest> query(make_shared<Interest>(queryName));

  shared_ptr<DummyLocalFace> face(make_shared<DummyLocalFace>());
  add(face);

  m_face->onReceiveData.connect(bind(&FaceQueryStatusPublisherFixture::decodeNackBlock, this, _1));

  m_manager.listQueriedFaces(*query);
  BOOST_REQUIRE(m_finished);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd

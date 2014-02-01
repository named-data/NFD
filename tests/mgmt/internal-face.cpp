/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/internal-face.hpp"
#include "mgmt/fib-manager.hpp"
#include "table/fib.hpp"
#include "../face/dummy-face.hpp"

#include <ndn-cpp-dev/management/fib-management-options.hpp>
#include <ndn-cpp-dev/management/control-response.hpp>

#include <boost/test/unit_test.hpp>

static nfd::FaceId g_faceCount = 1;
static std::vector<nfd::shared_ptr<nfd::Face> > g_faces;

static nfd::shared_ptr<nfd::Face>
getFace(nfd::FaceId id)
{
  if (g_faces.size() < id)
    {
      BOOST_FAIL("Attempted to access invalid FaceId: " << id);
    }
  return g_faces[id-1];
}



namespace nfd {

NFD_LOG_INIT("InternalFaceTest");

void
receiveValidNextHopControlResponse(const ndn::Data& response)
{
  // Path 1 - runtime exception on wireDecode.
  // Extract Block from response's payload and attempt
  // to decode.
  // {
  //   ndn::ControlResponse control;
  //   Block controlRaw(response.getContent());

  //   NFD_LOG_DEBUG("received raw control block size = " << controlRaw.size());

  //   control.wireDecode(controlRaw);

  //   NFD_LOG_DEBUG("received control response (Path 1)"
  //                 << " Name: " << response.getName()
  //                 << " code: " << control.getCode()
  //                 << " text: " << control.getText());

  //   BOOST_REQUIRE(control.getCode() == 200);
  //   BOOST_REQUIRE(control.getText() == "OK");
  // }

  // Path 1.5 - same as Path 1, but offset the payload's
  // encoded block by 2 bytes before decoding. 2 bytes
  // is the measured Block size difference between
  // ManagerBase's sendResponse and above Path 1's
  // received size.
  {
    ndn::ControlResponse control;
    Block controlRaw(response.getContent());

    NFD_LOG_DEBUG("received raw control block size = " << controlRaw.size());
    // controlRaw is currently 2 bytes larger than what was sent
    // try to offset it manually and create a new block

    BOOST_REQUIRE(controlRaw.hasWire());
    const uint8_t* buf = controlRaw.wire() + 2;
    size_t bufSize = controlRaw.size() - 2;

    Block alt(buf, bufSize);
    control.wireDecode(alt);

    NFD_LOG_DEBUG("received control response (Path 1)"
                  << " Name: " << response.getName()
                  << " code: " << control.getCode()
                  << " text: " << control.getText());

    BOOST_REQUIRE(control.getCode() == 200);
    BOOST_REQUIRE(control.getText() == "OK");
  }

  // Path 2 - works, but not conformant to protocol.
  // Extract decode and ControlResponse from last
  // component of response's name.
  // {
  //   const Name& responseName = response.getName();
  //   const ndn::Buffer& controlBuffer =
  //     responseName[responseName.size()-1].getValue();

  //   shared_ptr<const ndn::Buffer> tmpBuffer(new ndn::Buffer(controlBuffer));
  //   Block controlRaw(tmpBuffer);

  //   NFD_LOG_DEBUG("received raw control block size = " << controlRaw.size());

  //   ndn::ControlResponse control;
  //   control.wireDecode(controlRaw);

  //   NFD_LOG_DEBUG("received control response (Path 2)"
  //                 << " Name: " << response.getName()
  //                 << " code: " << control.getCode()
  //                 << " text: " << control.getText());

  //   BOOST_REQUIRE(control.getCode() == 200);
  //   BOOST_REQUIRE(control.getText() == "OK");
  // }


}

BOOST_AUTO_TEST_SUITE(MgmtInternalFace)

BOOST_AUTO_TEST_CASE(ValidAddNextHop)
{
  g_faceCount = 1;
  g_faces.clear();
  g_faces.push_back(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib, &getFace, face);

  face->setInterestFilter(manager.getRequestPrefix(),
                          bind(&FibManager::onFibRequest,
                               &manager, _2));

  face->onReceiveData +=
    bind(&receiveValidNextHopControlResponse, _1);

  Name regName(manager.getRequestPrefix());
  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(1);

  Block encodedOptions(options.wireEncode());

  Name commandName(manager.getRequestPrefix());
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  Interest command(commandName);
  face->sendInterest(command);

  shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch("/hello");

  if (entry)
    {
      const fib::NextHopList& hops = entry->getNextHops();
      BOOST_REQUIRE(hops.size() == 1);
      BOOST_CHECK(hops[0].getCost() == 1);
    }
  else
    {
      BOOST_FAIL("Failed to find expected fib entry");
    }
}

BOOST_AUTO_TEST_CASE(InvalidPrefixRegistration)
{
  g_faceCount = 1;
  g_faces.clear();
  g_faces.push_back(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib, &getFace, face);

  face->setInterestFilter(manager.getRequestPrefix(),
                          bind(&FibManager::onFibRequest,
                               &manager, _2));

  Interest nonRegInterest("/hello");
  face->sendInterest(nonRegInterest);

  shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch("/hello");

  if (entry)
    {
      const fib::NextHopList& hops = entry->getNextHops();
      BOOST_REQUIRE(hops.size() == 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd

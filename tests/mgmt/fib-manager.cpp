/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/fib-manager.hpp"
#include "fw/forwarder.hpp"
#include "table/fib.hpp"
#include "face/face.hpp"
#include "mgmt/internal-face.hpp"
#include "../face/dummy-face.hpp"

#include <ndn-cpp-dev/management/fib-management-options.hpp>

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

// namespace nfd {

// NFD_LOG_INIT("FibManagerTest");

// BOOST_AUTO_TEST_SUITE(MgmtFibManager)

// BOOST_AUTO_TEST_CASE(MalformedCommmand)
// {
//   shared_ptr<InternalFace> face(new InternalFace);
//   Fib fib;
//   FibManager manager(fib, &getFace, face);

//   Interest command(manager.getRequestPrefix());
//   manager.onFibRequest(command);
// }

// BOOST_AUTO_TEST_CASE(UnsupportedVerb)
// {
//   shared_ptr<InternalFace> face(new InternalFace);
//   Fib fib;
//   FibManager manager(fib, &getFace, face);

//   ndn::FibManagementOptions options;
//   options.setName("/hello");
//   options.setFaceId(1);
//   options.setCost(1);

//   Block encodedOptions(options.wireEncode());

//   Name commandName(manager.getRequestPrefix());
//   commandName.append("unsupported");
//   commandName.append(encodedOptions);

//   Interest command(commandName);
//   manager.onFibRequest(command);
// }

// BOOST_AUTO_TEST_CASE(AddNextHopVerbInitialAdd)
// {
//   g_faceCount = 1;
//   g_faces.clear();
//   g_faces.push_back(make_shared<DummyFace>());

//   shared_ptr<InternalFace> face(new InternalFace);
//   Fib fib;
//   FibManager manager(fib, &getFace, face);

//   ndn::FibManagementOptions options;
//   options.setName("/hello");
//   options.setFaceId(1);
//   options.setCost(1);

//   Block encodedOptions(options.wireEncode());

//   Name commandName(manager.getRequestPrefix());
//   commandName.append("add-nexthop");
//   commandName.append(encodedOptions);

//   Interest command(commandName);
//   manager.onFibRequest(command);

//   shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch("/hello");

//   if (entry)
//     {
//       const fib::NextHopList& hops = entry->getNextHops();
//       BOOST_REQUIRE(hops.size() == 1);
//       //      BOOST_CHECK(hops[0].getFace()->getFaceId() == 1);
//       BOOST_CHECK(hops[0].getCost() == 1);
//     }
//   else
//     {
//       BOOST_FAIL("Failed to find expected fib entry");
//     }
// }

// BOOST_AUTO_TEST_CASE(AddNextHopVerbAddToExisting)
// {
//   g_faceCount = 1;
//   g_faces.clear();
//   g_faces.push_back(make_shared<DummyFace>());
//   g_faces.push_back(make_shared<DummyFace>());

//   shared_ptr<InternalFace> face(new InternalFace);
//   Fib fib;
//   FibManager manager(fib, &getFace, face);

//   // Add faces with cost == FaceID for the name /hello
//   // This test assumes:
//   //   FaceIDs are assigned from 1 to N
//   //   Faces are store sequentially in the NextHopList
//   //   NextHopList supports random access

//   for (int i = 1; i <= 2; i++)
//     {
//       ndn::FibManagementOptions options;
//       options.setName("/hello");
//       options.setFaceId(i);
//       options.setCost(i);

//       Block encodedOptions(options.wireEncode());

//       Name commandName(manager.getRequestPrefix());
//       commandName.append("add-nexthop");
//       commandName.append(encodedOptions);

//       Interest command(commandName);
//       manager.onFibRequest(command);

//       shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch("/hello");

//       if (entry)
//         {
//           const fib::NextHopList& hops = entry->getNextHops();
//           for (int j = 1; j <= i; j++)
//             {
//               BOOST_REQUIRE(hops.size() == i);
//               BOOST_CHECK(hops[j-1].getCost() == j);
//             }
//         }
//       else
//         {
//           BOOST_FAIL("Failed to find expected fib entry");
//         }
//     }
// }

// BOOST_AUTO_TEST_CASE(AddNextHopVerbUpdateFaceCost)
// {
//   g_faceCount = 1;
//   g_faces.clear();
//   g_faces.push_back(make_shared<DummyFace>());

//   shared_ptr<InternalFace> face(new InternalFace);
//   Fib fib;
//   FibManager manager(fib, &getFace, face);

//   ndn::FibManagementOptions options;
//   options.setName("/hello");
//   options.setFaceId(1);

//   {
//     options.setCost(1);

//     Block encodedOptions(options.wireEncode());

//     Name commandName(manager.getRequestPrefix());
//     commandName.append("add-nexthop");
//     commandName.append(encodedOptions);

//     Interest command(commandName);
//     manager.onFibRequest(command);
//   }

//   {
//     options.setCost(2);

//     Block encodedOptions(options.wireEncode());

//     Name commandName(manager.getRequestPrefix());
//     commandName.append("add-nexthop");
//     commandName.append(encodedOptions);

//     Interest command(commandName);
//     manager.onFibRequest(command);
//   }

//   shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch("/hello");

//   // Add faces with cost == FaceID for the name /hello
//   // This test assumes:
//   //   FaceIDs are assigned from 1 to N
//   //   Faces are store sequentially in the NextHopList
//   //   NextHopList supports random access
//   if (entry)
//     {
//       const fib::NextHopList& hops = entry->getNextHops();
//       BOOST_REQUIRE(hops.size() == 1);
//       // BOOST_CHECK(hops[0].getFace()->getFaceId() == 1);
//       BOOST_CHECK(hops[0].getCost() == 2);
//     }
//   else
//     {
//       BOOST_FAIL("Failed to find expected fib entry");
//     }
// }

// BOOST_AUTO_TEST_SUITE_END()

// } // namespace nfd

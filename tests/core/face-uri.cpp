/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "core/face-uri.hpp"
#ifdef HAVE_PCAP
#include "face/ethernet.hpp"
#endif // HAVE_PCAP

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(CoreFaceUri, BaseFixture)

BOOST_AUTO_TEST_CASE(Internal)
{
  FaceUri uri;

  BOOST_CHECK(uri.parse("internal://"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "internal");
  BOOST_CHECK_EQUAL(uri.getHost(), "");
  BOOST_CHECK_EQUAL(uri.getPort(), "");
  BOOST_CHECK_EQUAL(uri.getPath(), "");

  BOOST_CHECK_EQUAL(uri.parse("internal:"), false);
  BOOST_CHECK_EQUAL(uri.parse("internal:/"), false);
}

BOOST_AUTO_TEST_CASE(Udp)
{
  BOOST_CHECK_NO_THROW(FaceUri("udp://hostname:6363"));
  BOOST_CHECK_THROW(FaceUri("udp//hostname:6363"), FaceUri::Error);
  BOOST_CHECK_THROW(FaceUri("udp://hostname:port"), FaceUri::Error);

  FaceUri uri;
  BOOST_CHECK_EQUAL(uri.parse("udp//hostname:6363"), false);

  BOOST_CHECK(uri.parse("udp://hostname:80"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "udp");
  BOOST_CHECK_EQUAL(uri.getHost(), "hostname");
  BOOST_CHECK_EQUAL(uri.getPort(), "80");
  BOOST_CHECK_EQUAL(uri.getPath(), "");

  BOOST_CHECK(uri.parse("udp4://192.0.2.1:20"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "udp4");
  BOOST_CHECK_EQUAL(uri.getHost(), "192.0.2.1");
  BOOST_CHECK_EQUAL(uri.getPort(), "20");
  BOOST_CHECK_EQUAL(uri.getPath(), "");

  BOOST_CHECK(uri.parse("udp6://[2001:db8:3f9:0::1]:6363"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "udp6");
  BOOST_CHECK_EQUAL(uri.getHost(), "2001:db8:3f9:0::1");
  BOOST_CHECK_EQUAL(uri.getPort(), "6363");
  BOOST_CHECK_EQUAL(uri.getPath(), "");

  BOOST_CHECK(uri.parse("udp6://[2001:db8:3f9:0:3025:ccc5:eeeb:86d3]:6363"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "udp6");
  BOOST_CHECK_EQUAL(uri.getHost(), "2001:db8:3f9:0:3025:ccc5:eeeb:86d3");
  BOOST_CHECK_EQUAL(uri.getPort(), "6363");
  BOOST_CHECK_EQUAL(uri.getPath(), "");

  BOOST_CHECK_EQUAL(uri.parse("udp6://[2001:db8:3f9:0:3025:ccc5:eeeb:86dg]:6363"), false);

  using namespace boost::asio;
  ip::udp::endpoint endpoint4(ip::address_v4::from_string("192.0.2.1"), 7777);
  BOOST_REQUIRE_NO_THROW(FaceUri(endpoint4));
  BOOST_CHECK_EQUAL(FaceUri(endpoint4).toString(), "udp4://192.0.2.1:7777");

  ip::udp::endpoint endpoint6(ip::address_v6::from_string("2001:DB8::1"), 7777);
  BOOST_REQUIRE_NO_THROW(FaceUri(endpoint6));
  BOOST_CHECK_EQUAL(FaceUri(endpoint6).toString(), "udp6://[2001:db8::1]:7777");
}

BOOST_AUTO_TEST_CASE(Tcp)
{
  FaceUri uri;

  BOOST_CHECK(uri.parse("tcp://random.host.name"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "tcp");
  BOOST_CHECK_EQUAL(uri.getHost(), "random.host.name");
  BOOST_CHECK_EQUAL(uri.getPort(), "");
  BOOST_CHECK_EQUAL(uri.getPath(), "");

  BOOST_CHECK_EQUAL(uri.parse("tcp://192.0.2.1:"), false);
  BOOST_CHECK_EQUAL(uri.parse("tcp://[::zzzz]"), false);

  using namespace boost::asio;
  ip::tcp::endpoint endpoint4(ip::address_v4::from_string("192.0.2.1"), 7777);
  BOOST_REQUIRE_NO_THROW(FaceUri(endpoint4));
  BOOST_CHECK_EQUAL(FaceUri(endpoint4).toString(), "tcp4://192.0.2.1:7777");

  ip::tcp::endpoint endpoint6(ip::address_v6::from_string("2001:DB8::1"), 7777);
  BOOST_REQUIRE_NO_THROW(FaceUri(endpoint6));
  BOOST_CHECK_EQUAL(FaceUri(endpoint6).toString(), "tcp6://[2001:db8::1]:7777");
}

BOOST_AUTO_TEST_CASE(Unix)
{
  FaceUri uri;

  BOOST_CHECK(uri.parse("unix:///var/run/example.sock"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "unix");
  BOOST_CHECK_EQUAL(uri.getHost(), "");
  BOOST_CHECK_EQUAL(uri.getPort(), "");
  BOOST_CHECK_EQUAL(uri.getPath(), "/var/run/example.sock");

  //BOOST_CHECK_EQUAL(uri.parse("unix://var/run/example.sock"), false);
  // This is not a valid unix:/ URI, but the parse would treat "var" as host

#ifdef HAVE_UNIX_SOCKETS
  using namespace boost::asio;
  local::stream_protocol::endpoint endpoint("/var/run/example.sock");
  BOOST_REQUIRE_NO_THROW(FaceUri(endpoint));
  BOOST_CHECK_EQUAL(FaceUri(endpoint).toString(), "unix:///var/run/example.sock");
#endif // HAVE_UNIX_SOCKETS
}

BOOST_AUTO_TEST_CASE(Fd)
{
  FaceUri uri;

  BOOST_CHECK(uri.parse("fd://6"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "fd");
  BOOST_CHECK_EQUAL(uri.getHost(), "6");
  BOOST_CHECK_EQUAL(uri.getPort(), "");
  BOOST_CHECK_EQUAL(uri.getPath(), "");

  int fd = 21;
  BOOST_REQUIRE_NO_THROW(FaceUri::fromFd(fd));
  BOOST_CHECK_EQUAL(FaceUri::fromFd(fd).toString(), "fd://21");
}

BOOST_AUTO_TEST_CASE(Ether)
{
  FaceUri uri;

  BOOST_CHECK(uri.parse("ether://08:00:27:01:dd:01"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "ether");
  BOOST_CHECK_EQUAL(uri.getHost(), "08:00:27:01:dd:01");
  BOOST_CHECK_EQUAL(uri.getPort(), "");
  BOOST_CHECK_EQUAL(uri.getPath(), "");

  BOOST_CHECK_EQUAL(uri.parse("ether://08:00:27:zz:dd:01"), false);

#ifdef HAVE_PCAP
  ethernet::Address address = ethernet::Address::fromString("33:33:01:01:01:01");
  BOOST_REQUIRE_NO_THROW(FaceUri(address));
  BOOST_CHECK_EQUAL(FaceUri(address).toString(), "ether://33:33:01:01:01:01");
#endif // HAVE_PCAP
}

BOOST_AUTO_TEST_CASE(Dev)
{
  FaceUri uri;

  BOOST_CHECK(uri.parse("dev://eth0"));
  BOOST_CHECK_EQUAL(uri.getScheme(), "dev");
  BOOST_CHECK_EQUAL(uri.getHost(), "eth0");
  BOOST_CHECK_EQUAL(uri.getPort(), "");
  BOOST_CHECK_EQUAL(uri.getPath(), "");

  std::string ifname = "en1";
  BOOST_REQUIRE_NO_THROW(FaceUri::fromDev(ifname));
  BOOST_CHECK_EQUAL(FaceUri::fromDev(ifname).toString(), "dev://en1");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd

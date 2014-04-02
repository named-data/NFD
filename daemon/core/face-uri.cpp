/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face-uri.hpp"
#include "core/logger.hpp"
#ifdef HAVE_PCAP
#include "face/ethernet.hpp"
#endif // HAVE_PCAP

#include <boost/regex.hpp>

NFD_LOG_INIT("FaceUri");

namespace nfd {

FaceUri::FaceUri(const std::string& uri)
{
  if (!parse(uri)) {
    throw Error("Malformed URI: " + uri);
  }
}

FaceUri::FaceUri(const char* uri)
{
  if (!parse(uri)) {
    throw Error("Malformed URI: " + std::string(uri));
  }
}

bool
FaceUri::parse(const std::string& uri)
{
  m_scheme.clear();
  m_host.clear();
  m_isV6 = false;
  m_port.clear();
  m_path.clear();

  static const boost::regex protocolExp("(\\w+\\d?)://([^/]*)(\\/[^?]*)?");
  boost::smatch protocolMatch;
  if (!boost::regex_match(uri, protocolMatch, protocolExp)) {
    return false;
  }
  m_scheme = protocolMatch[1];
  const std::string& authority = protocolMatch[2];
  m_path = protocolMatch[3];

  // pattern for IPv6 address enclosed in [ ], with optional port number
  static const boost::regex v6Exp("^\\[([a-fA-F0-9:]+)\\](?:\\:(\\d+))?$");
  // pattern for Ethernet address in standard hex-digits-and-colons notation
  static const boost::regex etherExp("^((?:[a-fA-F0-9]{1,2}\\:){5}(?:[a-fA-F0-9]{1,2}))$");
  // pattern for IPv4/hostname/fd/ifname, with optional port number
  static const boost::regex v4HostExp("^([^:]+)(?:\\:(\\d+))?$");

  if (authority.empty()) {
    // UNIX, internal
  }
  else {
    boost::smatch match;
    m_isV6 = boost::regex_match(authority, match, v6Exp);
    if (m_isV6 ||
        boost::regex_match(authority, match, etherExp) ||
        boost::regex_match(authority, match, v4HostExp)) {
      m_host = match[1];
      m_port = match[2];
    }
    else {
      return false;
    }
  }

  NFD_LOG_DEBUG("URI [" << uri << "] parsed into: " <<
                m_scheme << ", " << m_host << ", " << m_port << ", " << m_path);
  return true;
}

FaceUri::FaceUri(const boost::asio::ip::tcp::endpoint& endpoint)
{
  m_isV6 = endpoint.address().is_v6();
  m_scheme = m_isV6 ? "tcp6" : "tcp4";
  m_host = endpoint.address().to_string();
  m_port = boost::lexical_cast<std::string>(endpoint.port());
}

FaceUri::FaceUri(const boost::asio::ip::udp::endpoint& endpoint)
{
  m_isV6 = endpoint.address().is_v6();
  m_scheme = m_isV6 ? "udp6" : "udp4";
  m_host = endpoint.address().to_string();
  m_port = boost::lexical_cast<std::string>(endpoint.port());
}

#ifdef HAVE_UNIX_SOCKETS
FaceUri::FaceUri(const boost::asio::local::stream_protocol::endpoint& endpoint)
  : m_isV6(false)
{
  m_scheme = "unix";
  m_path = endpoint.path();
}
#endif // HAVE_UNIX_SOCKETS

FaceUri
FaceUri::fromFd(int fd)
{
  FaceUri uri;
  uri.m_scheme = "fd";
  uri.m_host = boost::lexical_cast<std::string>(fd);
  return uri;
}

#ifdef HAVE_PCAP
FaceUri::FaceUri(const ethernet::Address& address)
  : m_isV6(false)
{
  m_scheme = "ether";
  m_host = address.toString(':');
}
#endif // HAVE_PCAP

FaceUri
FaceUri::fromDev(const std::string& ifname)
{
  FaceUri uri;
  uri.m_scheme = "dev";
  uri.m_host = ifname;
  return uri;
}

} // namespace nfd

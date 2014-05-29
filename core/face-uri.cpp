/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology,
 *                     The University of Memphis
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
 **/

#include "face-uri.hpp"
#include "core/logger.hpp"

#ifdef HAVE_LIBPCAP
#include "ethernet.hpp"
#endif // HAVE_LIBPCAP

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
  static const boost::regex etherExp("^\\[((?:[a-fA-F0-9]{1,2}\\:){5}(?:[a-fA-F0-9]{1,2}))\\]$");
  // pattern for IPv4-mapped IPv6 address, with optional port number
  static const boost::regex v4MappedV6Exp("^\\[::ffff:(\\d+(?:\\.\\d+){3})\\](?:\\:(\\d+))?$");
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
        boost::regex_match(authority, match, v4MappedV6Exp) ||
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

FaceUri::FaceUri(const boost::asio::ip::tcp::endpoint& endpoint, const std::string& scheme)
  : m_scheme(scheme)
{
  m_isV6 = endpoint.address().is_v6();
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

#ifdef HAVE_LIBPCAP
FaceUri::FaceUri(const ethernet::Address& address)
  : m_isV6(true)
{
  m_scheme = "ether";
  m_host = address.toString();
}
#endif // HAVE_LIBPCAP

FaceUri
FaceUri::fromDev(const std::string& ifname)
{
  FaceUri uri;
  uri.m_scheme = "dev";
  uri.m_host = ifname;
  return uri;
}

} // namespace nfd

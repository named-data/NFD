/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face-uri.hpp"
#include "core/logger.hpp"
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

  boost::regex protocolExp("(\\w+\\d?)://([^/]*)(\\/[^?]*)?");
  boost::smatch protocolMatch;
  if (!boost::regex_match(uri, protocolMatch, protocolExp)) {
    return false;
  }
  m_scheme = protocolMatch[1];
  m_path = protocolMatch[3];

  const std::string& authority = protocolMatch[2];

  boost::regex v6Exp("^\\[(([a-fA-F0-9:]+))\\](:(\\d+))?$"); // [host]:port
  boost::regex v4Exp("^((\\d+\\.){3}\\d+)(:(\\d+))?$");
  boost::regex hostExp("^(([^:]*))(:(\\d+))?$"); // host:port

  boost::smatch match;
  m_isV6 = boost::regex_match(authority, match, v6Exp);
  if (m_isV6 ||
      boost::regex_match(authority, match, v4Exp) ||
      boost::regex_match(authority, match, hostExp)) {
    m_host = match[1];
    m_port = match[4];
  }
  else {
    if (m_path.empty()) {
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

FaceUri::FaceUri(const boost::asio::local::stream_protocol::endpoint& endpoint)
  : m_isV6(false)
{
  m_scheme = "unix";
  m_path = endpoint.path();
}

} // namespace nfd

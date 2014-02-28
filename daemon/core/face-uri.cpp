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
  if (!parse(uri))
    {
      throw Error("Malformed URI: " + uri);
    }
}

bool
FaceUri::parse(const std::string& uri)
{
  m_scheme.clear();
  m_domain.clear();
  m_port.clear();
  
  boost::regex protocolExp("(\\w+\\d?)://(.+)");
  boost::smatch protocolMatch;
  if (!boost::regex_match(uri, protocolMatch, protocolExp))
    {
      return false;
    }
  m_scheme = protocolMatch[1];

  const std::string& remote = protocolMatch[2];

  boost::regex v6Exp("^\\[(([a-fA-F0-9:]+))\\](:(\\d+))?$"); // [stuff]:port
  boost::regex v4Exp("^((\\d+\\.){3}\\d+)(:(\\d+))?$");
  boost::regex hostExp("^(([^:]+))(:(\\d+))?$"); // stuff:port

  boost::smatch match;
  if (boost::regex_match(remote, match, v6Exp) ||
      boost::regex_match(remote, match, v4Exp) ||
      boost::regex_match(remote, match, hostExp))
    {
      m_domain = match[1];
      m_port   = match[4];
    }
  else
    {
      return false;
    }
  
  NFD_LOG_DEBUG("URI [" << uri << "] parsed into: "
                << m_scheme << ", " << m_domain << ", " << m_port);
  return true;
}

} // namespace nfd

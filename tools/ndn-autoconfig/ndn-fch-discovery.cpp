/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "ndn-fch-discovery.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/regex.hpp>

#include <sstream>

namespace ndn {
namespace tools {
namespace autoconfig {

/**
 * A partial and specialized copy of ndn::FaceUri implementation
 *
 * Consider removing in favor of a library-provided URL parsing, if project
 * includes such a library.
 */
class Url
{
public:
  Url(const std::string& url)
    : m_isValid(false)
  {
    static const boost::regex protocolExp("(\\w+\\d?(\\+\\w+)?)://([^/]*)(\\/[^?]*)?");
    boost::smatch protocolMatch;
    if (!boost::regex_match(url, protocolMatch, protocolExp)) {
      return;
    }
    m_scheme = protocolMatch[1];
    const std::string& authority = protocolMatch[3];
    m_path = protocolMatch[4];

    // pattern for IPv6 address enclosed in [ ], with optional port number
    static const boost::regex v6Exp("^\\[([a-fA-F0-9:]+)\\](?:\\:(\\d+))?$");
    // pattern for IPv4-mapped IPv6 address, with optional port number
    static const boost::regex v4MappedV6Exp("^\\[::ffff:(\\d+(?:\\.\\d+){3})\\](?:\\:(\\d+))?$");
    // pattern for IPv4/hostname/fd/ifname, with optional port number
    static const boost::regex v4HostExp("^([^:]+)(?:\\:(\\d+))?$");

    if (authority.empty()) {
      // UNIX, internal
    }
    else {
      boost::smatch match;
      bool isV6 = boost::regex_match(authority, match, v6Exp);
      if (isV6 ||
          boost::regex_match(authority, match, v4MappedV6Exp) ||
          boost::regex_match(authority, match, v4HostExp)) {
        m_host = match[1];
        m_port = match[2];
      }
      else {
        return;
      }
    }
    if (m_port.empty()) {
      m_port = "80";
    }
    if (m_path.empty()) {
      m_path = "/";
    }
    m_isValid = true;
  }

  bool
  isValid() const
  {
    return m_isValid;
  }

  const std::string&
  getScheme() const
  {
    return m_scheme;
  }

  const std::string&
  getHost() const
  {
    return m_host;
  }

  const std::string&
  getPort() const
  {
    return m_port;
  }

  const std::string&
  getPath() const
  {
    return m_path;
  }

private:
  bool m_isValid;
  std::string m_scheme;
  std::string m_host;
  std::string m_port;
  std::string m_path;
};

class HttpException : public std::runtime_error
{
public:
  explicit
  HttpException(const std::string& what)
    : std::runtime_error(what)
  {
  }
};

NdnFchDiscovery::NdnFchDiscovery(const std::string& url)
  : m_url(url)
{
}

void
NdnFchDiscovery::doStart()
{
  try {
    boost::asio::ip::tcp::iostream requestStream;
#if BOOST_VERSION >= 106700
    requestStream.expires_after(std::chrono::seconds(3));
#else
    requestStream.expires_from_now(boost::posix_time::seconds(3));
#endif // BOOST_VERSION >= 106700

    Url url(m_url);
    if (!url.isValid()) {
      BOOST_THROW_EXCEPTION(HttpException("Invalid NDN-FCH URL: " + m_url));
    }
    if (!boost::iequals(url.getScheme(), "http")) {
      BOOST_THROW_EXCEPTION(HttpException("Only http:// NDN-FCH URLs are supported"));
    }

    requestStream.connect(url.getHost(), url.getPort());
    if (!requestStream) {
      BOOST_THROW_EXCEPTION(HttpException("HTTP connection error to " + m_url));
    }

    requestStream << "GET " << url.getPath() << " HTTP/1.0\r\n";
    requestStream << "Host: " << url.getHost() << ":" << url.getPort() << "\r\n";
    requestStream << "Accept: */*\r\n";
    requestStream << "Cache-Control: no-cache\r\n";
    requestStream << "Connection: close\r\n\r\n";
    requestStream.flush();

    std::string statusLine;
    std::getline(requestStream, statusLine);
    if (!requestStream) {
      BOOST_THROW_EXCEPTION(HttpException("HTTP communication error"));
    }

    std::stringstream responseStream(statusLine);
    std::string httpVersion;
    responseStream >> httpVersion;
    unsigned int statusCode;
    responseStream >> statusCode;
    std::string statusMessage;

    std::getline(responseStream, statusMessage);
    if (!static_cast<bool>(requestStream) || httpVersion.substr(0, 5) != "HTTP/") {
      BOOST_THROW_EXCEPTION(HttpException("HTTP communication error"));
    }
    if (statusCode != 200) {
      boost::trim(statusMessage);
      BOOST_THROW_EXCEPTION(HttpException("HTTP request failed: " + to_string(statusCode) + " " + statusMessage));
    }
    std::string header;
    while (std::getline(requestStream, header) && header != "\r")
      ;

    std::string hubHost;
    requestStream >> hubHost;
    if (hubHost.empty()) {
      BOOST_THROW_EXCEPTION(HttpException("NDN-FCH did not return hub host"));
    }

    this->provideHubFaceUri("udp://" + hubHost);
  }
  catch (const std::runtime_error& e) {
    this->fail(e.what());
  }
}

} // namespace autoconfig
} // namespace tools
} // namespace ndn

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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

#ifndef NFD_CORE_FACE_URI_H
#define NFD_CORE_FACE_URI_H

#include "common.hpp"

namespace nfd {

#ifdef HAVE_LIBPCAP
namespace ethernet {
class Address;
} // namespace ethernet
#endif // HAVE_LIBPCAP

/** \brief represents the underlying protocol and address used by a Face
 *  \sa http://redmine.named-data.net/projects/nfd/wiki/FaceMgmt#FaceUri
 */
class FaceUri
{
public:
  class Error : public std::invalid_argument
  {
  public:
    explicit
    Error(const std::string& what)
      : std::invalid_argument(what)
    {
    }
  };

  FaceUri();

  /** \brief construct by parsing
   *
   *  \param uri scheme://host[:port]/path
   *  \throw FaceUri::Error if URI cannot be parsed
   */
  explicit
  FaceUri(const std::string& uri);

  // This overload is needed so that calls with string literal won't be
  // resolved to boost::asio::local::stream_protocol::endpoint overload.
  explicit
  FaceUri(const char* uri);

  /// exception-safe parsing
  bool
  parse(const std::string& uri);

public: // scheme-specific construction
  /// construct tcp4 or tcp6 canonical FaceUri
  explicit
  FaceUri(const boost::asio::ip::tcp::endpoint& endpoint);

  /// construct udp4 or udp6 canonical FaceUri
  explicit
  FaceUri(const boost::asio::ip::udp::endpoint& endpoint);

  /// construct tcp canonical FaceUri with customized scheme
  FaceUri(const boost::asio::ip::tcp::endpoint& endpoint, const std::string& scheme);

#ifdef HAVE_UNIX_SOCKETS
  /// construct unix canonical FaceUri
  explicit
  FaceUri(const boost::asio::local::stream_protocol::endpoint& endpoint);
#endif // HAVE_UNIX_SOCKETS

  /// create fd FaceUri from file descriptor
  static FaceUri
  fromFd(int fd);

#ifdef HAVE_LIBPCAP
  /// construct ether canonical FaceUri
  explicit
  FaceUri(const ethernet::Address& address);
#endif // HAVE_LIBPCAP

  /// create dev FaceUri from network device name
  static FaceUri
  fromDev(const std::string& ifname);

public: // getters
  /// get scheme (protocol)
  const std::string&
  getScheme() const;

  /// get host (domain)
  const std::string&
  getHost() const;

  /// get port
  const std::string&
  getPort() const;

  /// get path
  const std::string&
  getPath() const;

  /// write as a string
  std::string
  toString() const;

public: // EqualityComparable concept
  /// equality operator
  bool
  operator==(const FaceUri& rhs) const;

  /// inequality operator
  bool
  operator!=(const FaceUri& rhs) const;

private:
  std::string m_scheme;
  std::string m_host;
  /// whether to add [] around host when writing string
  bool m_isV6;
  std::string m_port;
  std::string m_path;

  friend std::ostream& operator<<(std::ostream& os, const FaceUri& uri);
};

inline
FaceUri::FaceUri()
  : m_isV6(false)
{
}

inline const std::string&
FaceUri::getScheme() const
{
  return m_scheme;
}

inline const std::string&
FaceUri::getHost() const
{
  return m_host;
}

inline const std::string&
FaceUri::getPort() const
{
  return m_port;
}

inline const std::string&
FaceUri::getPath() const
{
  return m_path;
}

inline std::string
FaceUri::toString() const
{
  std::ostringstream os;
  os << *this;
  return os.str();
}

inline bool
FaceUri::operator==(const FaceUri& rhs) const
{
  return (m_scheme == rhs.m_scheme &&
          m_host == rhs.m_host &&
          m_isV6 == rhs.m_isV6 &&
          m_port == rhs.m_port &&
          m_path == rhs.m_path);
}

inline bool
FaceUri::operator!=(const FaceUri& rhs) const
{
  return !(*this == rhs);
}

inline std::ostream&
operator<<(std::ostream& os, const FaceUri& uri)
{
  os << uri.m_scheme << "://";
  if (uri.m_isV6) {
    os << "[" << uri.m_host << "]";
  }
  else {
    os << uri.m_host;
  }
  if (!uri.m_port.empty()) {
    os << ":" << uri.m_port;
  }
  os << uri.m_path;
  return os;
}

} // namespace nfd

#endif // NFD_CORE_FACE_URI_H

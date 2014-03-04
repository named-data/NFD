/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_FACE_URI_H
#define NFD_CORE_FACE_URI_H

#include "common.hpp"

namespace nfd {

/// represents a URI in Face Management protocol
class FaceUri
{
public:
  class Error : public std::runtime_error
  {
  public:
    Error(const std::string& what) : std::runtime_error(what) {}
  };

  FaceUri();

  /** \brief construct by parsing
   *
   * \param uri scheme://domain[:port]/path
   * \throw FaceUri::Error if URI cannot be parsed
   */
  explicit
  FaceUri(const std::string& uri);

  // This overload is needed so that calls with string literal won't be
  // resolved to boost::asio::local::stream_protocol::endpoint overload.
  explicit
  FaceUri(const char* uri);

  explicit
  FaceUri(const boost::asio::ip::tcp::endpoint& endpoint);

  explicit
  FaceUri(const boost::asio::ip::udp::endpoint& endpoint);

  explicit
  FaceUri(const boost::asio::local::stream_protocol::endpoint& endpoint);

  /// exception-safe parsing
  bool
  parse(const std::string& uri);

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

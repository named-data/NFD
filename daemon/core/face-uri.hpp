/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_FACE_URI_H
#define NFD_CORE_FACE_URI_H

#include "common.hpp"

namespace nfd {

class FaceUri
{
public:
  class Error : public std::runtime_error
  {
  public:
    Error(const std::string& what) : std::runtime_error(what) {}
  };

  FaceUri();
  
  /** \brief Construct URI object
   *
   * Expected format: scheme://domain:port/path?query_string#fragment_id
   *
   * \throw FaceUri::Error if URI cannot be parsed
   */
  explicit
  FaceUri(const std::string& uri);

  /** \brief Exception-safe parsing of URI
   */
  bool
  parse(const std::string& uri);

  /** \brief Get scheme (protocol) extracted from URI
   */
  const std::string&
  getScheme() const;

  /** \brief Get domain extracted from URI
   */
  const std::string&
  getDomain() const;

  /** \brief Get port extracted from URI
   *
   *  \return Extracted port or empty string if port wasn't present
   */
  const std::string&
  getPort() const;

  // other getters should be added, when necessary

private:
  std::string m_scheme;
  std::string m_domain;
  std::string m_port;
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
FaceUri::getDomain() const
{
  return m_domain;
}

inline const std::string&
FaceUri::getPort() const
{
  return m_port;
}

} // namespace nfd

#endif // NFD_CORE_FACE_URI_H

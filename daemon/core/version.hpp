/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_CORE_VERSION_HPP
#define NFD_CORE_VERSION_HPP

#include "common.hpp"

namespace nfd {

/** NFD version follows Semantic Versioning 2.0.0 specification
 *  http://semver.org/
 */

/** \brief NFD version represented as an integer
 *
 *  MAJOR*1000000 + MINOR*1000 + PATCH
 */
#define NFD_VERSION 1000

/** \brief NFD version represented as a string
 *
 *  MAJOR.MINOR.PATCH
 */
#define NFD_VERSION_STRING "0.1.0"

/// MAJOR version
#define NFD_VERSION_MAJOR (NFD_VERSION / 1000000)
/// MINOR version
#define NFD_VERSION_MINOR (NFD_VERSION % 1000000 / 1000)
/// PATCH version
#define NFD_VERSION_PATCH (NFD_VERSION % 1000)

} // namespace nfd

#endif // NFD_CORE_VERSION_HPP

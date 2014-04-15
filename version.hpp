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

#ifndef NFD_VERSION_HPP
#define NFD_VERSION_HPP

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

#endif // NFD_VERSION_HPP

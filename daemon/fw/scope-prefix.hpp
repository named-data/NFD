/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FW_SCOPE_PREFIX_HPP
#define NFD_DAEMON_FW_SCOPE_PREFIX_HPP

#include "core/common.hpp"

/** \brief contain name prefixes that affect namespace-based scope control
 *  \sa https://redmine.named-data.net/projects/nfd/wiki/ScopeControl
 */
namespace nfd {
namespace scope_prefix {

/** \brief ndn:/localhost
 *
 *  The localhost scope limits propagation to the applications on the originating host.
 *
 *  Interest and Data packets under prefix ndn:/localhost are restricted by these rules:
 *  \li Interest can come from and go to local faces only.
 *  \li Data can come from and go to local faces only.
 */
extern const Name LOCALHOST;

/** \brief ndn:/localhop
 *
 *  The localhop scope limits propagation to no further than the next node.
 *
 *  Interest packets under prefix ndn:/localhop are restricted by these rules:
 *  \li If an Interest is received from a local face, it can be forwarded to a non-local face.
 *  \li If an Interest is received from a non-local face, it cannot be forwarded to a non-local face.
 *  \li In either case the Interest can be forwarded to a local face.
 *  \li PIT entry can be satisfied by Data from any source.
 *
 *  Data packets under prefix ndn:/localhop are unrestricted.
 */
extern const Name LOCALHOP;

} // namespace scope_prefix
} // namespace nfd

#endif // NFD_DAEMON_FW_SCOPE_PREFIX_HPP

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FW_PIT_ALGORITHM_HPP
#define NFD_DAEMON_FW_PIT_ALGORITHM_HPP

#include "table/pit-entry.hpp"

/** \file
 *  This file contains algorithms that operate on a PIT entry.
 */

namespace nfd {

/** \brief contain Name prefix that affects namespace-based scope control
 *  \sa http://redmine.named-data.net/projects/nfd/wiki/ScopeControl
 */
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
 *  \li Interest can come from a local face or a non-local face.
 *  \li If PIT entry has at least one in-record from a local face,
 *      it can be forwarded to local faces and non-local faces.
 *  \li If PIT entry has all in-records from non-local faces,
 *      it can only be forwarded to local faces.
 *  \li PIT entry can be satisfied by Data from any source.
 *
 *  Data packets under prefix ndn:/localhop are unrestricted.
 */
extern const Name LOCALHOP;

} // namespace scope_prefix

namespace fw {

/** \brief determine whether forwarding the Interest in \p pitEntry to \p outFace would violate scope
 *  \sa http://redmine.named-data.net/projects/nfd/wiki/ScopeControl
 */
bool
violatesScope(const pit::Entry& pitEntry, const Face& outFace);

/** \brief decide whether Interest can be forwarded to face
 *
 *  \return true if out-record of this face does not exist or has expired,
 *          and there is an in-record not of this face,
 *          and scope is not violated
 *
 *  \note This algorithm has a weakness that it does not permit consumer retransmissions
 *        before out-record expires. Therefore, it's not recommended to use this function
 *        in new strategies.
 *  \todo find a better name for this function
 */
bool
canForwardToLegacy(const pit::Entry& pitEntry, const Face& face);

/** \brief indicates where duplicate Nonces are found
 */
enum DuplicateNonceWhere {
  DUPLICATE_NONCE_NONE      = 0,        ///< no duplicate Nonce is found
  DUPLICATE_NONCE_IN_SAME   = (1 << 0), ///< in-record of same face
  DUPLICATE_NONCE_IN_OTHER  = (1 << 1), ///< in-record of other face
  DUPLICATE_NONCE_OUT_SAME  = (1 << 2), ///< out-record of same face
  DUPLICATE_NONCE_OUT_OTHER = (1 << 3)  ///< out-record of other face
};

/** \brief determine whether \p pitEntry has duplicate Nonce \p nonce
 *  \return OR'ed DuplicateNonceWhere
 */
int
findDuplicateNonce(const pit::Entry& pitEntry, uint32_t nonce, const Face& face);

/** \brief determine whether \p pitEntry has any pending out-records
 *  \return true if there is at least one out-record waiting for Data
 */
bool
hasPendingOutRecords(const pit::Entry& pitEntry);

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_PIT_ALGORITHM_HPP

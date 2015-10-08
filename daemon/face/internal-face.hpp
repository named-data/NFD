/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FACE_INTERNAL_FACE_HPP
#define NFD_DAEMON_FACE_INTERNAL_FACE_HPP

#include "face.hpp"
#include <ndn-cxx/face.hpp>

namespace nfd {
namespace face {

/** \brief make a pair of forwarder-side face and client-side face
 *         that are connected with each other
 *
 *  Network-layer packets sent by one face will be received by the other face
 *  after io.poll().
 *
 *  \param clientKeyChain A KeyChain used by client-side face to sign
 *                        prefix registration commands.
 *  \return a forwarder-side face and a client-side face connected with each other
 */
std::tuple<shared_ptr<Face>, shared_ptr<ndn::Face>>
makeInternalFace(ndn::KeyChain& clientKeyChain);

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_INTERNAL_FACE_HPP

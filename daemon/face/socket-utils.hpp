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

#ifndef NFD_DAEMON_FACE_SOCKET_UTILS_HPP
#define NFD_DAEMON_FACE_SOCKET_UTILS_HPP

#include "core/common.hpp"

namespace nfd {
namespace face {

/** \brief obtain send queue length from a specified system socket
 *  \param fd file descriptor of the socket
 *  \retval QUEUE_UNSUPPORTED this operation is unsupported on the current platform
 *  \retval QUEUE_ERROR there was an error retrieving the send queue length
 *
 *  On Linux, ioctl() is used to obtain the value of SIOCOUTQ from the socket.
 *  On macOS, getsockopt() is used to obtain the value of SO_NWRITE from the socket.
 */
ssize_t
getTxQueueLength(int fd);

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_SOCKET_UTILS_HPP

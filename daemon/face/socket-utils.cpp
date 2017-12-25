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

#include "socket-utils.hpp"
#include "transport.hpp"

#if defined(__linux__)
#include <linux/sockios.h>
#include <sys/ioctl.h>
#elif defined(__APPLE__)
#include <sys/socket.h>
#endif

namespace nfd {
namespace face {

ssize_t
getTxQueueLength(int fd)
{
  int queueLength = QUEUE_UNSUPPORTED;
#if defined(__linux__)
  if (ioctl(fd, SIOCOUTQ, &queueLength) < 0) {
    queueLength = QUEUE_ERROR;
  }
#elif defined(__APPLE__)
  socklen_t queueLengthSize = sizeof(queueLength);
  if (getsockopt(fd, SOL_SOCKET, SO_NWRITE, &queueLength, &queueLengthSize) < 0) {
    queueLength = QUEUE_ERROR;
  }
#endif
  return queueLength;
}

} // namespace face
} // namespace nfd

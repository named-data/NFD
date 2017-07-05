/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FACE_CHANNEL_LOG_HPP
#define NFD_DAEMON_FACE_CHANNEL_LOG_HPP

#include "core/logger.hpp"

/** \defgroup ChannelLogging Channel logging macros
 *
 * These macros augment the log message with some channel-specific information,
 * such as the local URI, that are useful to distinguish which channel produced
 * the message. It is strongly recommended to use these macros instead of the
 * generic ones for all logging inside Channel subclasses.
 * @{
 */

/** \cond */
#define NFD_LOG_CHAN(level, msg) NFD_LOG_##level( \
  "[" << this->getUri() << "] " << msg)
/** \endcond */

/** \brief Log a message at TRACE level */
#define NFD_LOG_CHAN_TRACE(msg) NFD_LOG_CHAN(TRACE, msg)

/** \brief Log a message at DEBUG level */
#define NFD_LOG_CHAN_DEBUG(msg) NFD_LOG_CHAN(DEBUG, msg)

/** \brief Log a message at INFO level */
#define NFD_LOG_CHAN_INFO(msg)  NFD_LOG_CHAN(INFO,  msg)

/** \brief Log a message at WARN level */
#define NFD_LOG_CHAN_WARN(msg)  NFD_LOG_CHAN(WARN,  msg)

/** \brief Log a message at ERROR level */
#define NFD_LOG_CHAN_ERROR(msg) NFD_LOG_CHAN(ERROR, msg)

/** @} */

#endif // NFD_DAEMON_FACE_CHANNEL_LOG_HPP

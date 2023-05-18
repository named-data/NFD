/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#ifndef NFD_DAEMON_RIB_FIB_UPDATE_HPP
#define NFD_DAEMON_RIB_FIB_UPDATE_HPP

#include "core/common.hpp"

namespace nfd::rib {

/**
 * \brief Represents a FIB update.
 */
class FibUpdate
{
public:
  enum Action {
    ADD_NEXTHOP    = 0,
    REMOVE_NEXTHOP = 1,
  };

  static FibUpdate
  createAddUpdate(const Name& name, uint64_t faceId, uint64_t cost);

  static FibUpdate
  createRemoveUpdate(const Name& name, uint64_t faceId);

public: // non-member operators (hidden friends)
  friend bool
  operator==(const FibUpdate& lhs, const FibUpdate& rhs) noexcept
  {
    return lhs.name == rhs.name &&
           lhs.faceId == rhs.faceId &&
           lhs.cost == rhs.cost &&
           lhs.action == rhs.action;
  }

  friend bool
  operator!=(const FibUpdate& lhs, const FibUpdate& rhs) noexcept
  {
    return !(lhs == rhs);
  }

public:
  Name name;
  uint64_t faceId = 0;
  uint64_t cost = 0;
  Action action;
};

std::ostream&
operator<<(std::ostream& os, const FibUpdate& update);

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_FIB_UPDATE_HPP

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

#ifndef NFD_DAEMON_FW_STRATEGY_INFO_HPP
#define NFD_DAEMON_FW_STRATEGY_INFO_HPP

#include "core/common.hpp"

namespace nfd {
namespace fw {

/** \brief contains arbitrary information forwarding strategy places on table entries
 */
class StrategyInfo
{
public:
#ifdef DOXYGEN
  /** \return an integer that uniquely identifies this StrategyInfo type
   *  \sa https://redmine.named-data.net/projects/nfd/wiki/StrategyInfoType
   */
  static constexpr int
  getTypeId()
  {
    return <type-identifier>;
  }
#endif

  virtual
  ~StrategyInfo() = default;

protected:
  StrategyInfo() = default;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_STRATEGY_INFO_HPP

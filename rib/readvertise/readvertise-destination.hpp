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

#ifndef NFD_RIB_READVERTISE_READVERTISE_DESTINATION_HPP
#define NFD_RIB_READVERTISE_READVERTISE_DESTINATION_HPP

#include "readvertised-route.hpp"

namespace nfd {
namespace rib {

/** \brief a destination to readvertise into
 */
class ReadvertiseDestination : noncopyable
{
public:
  virtual
  ~ReadvertiseDestination() = default;

  virtual void
  advertise(const ReadvertisedRoute& rr,
            std::function<void()> successCb,
            std::function<void(const std::string&)> failureCb) = 0;

  virtual void
  withdraw(const ReadvertisedRoute& rr,
           std::function<void()> successCb,
           std::function<void(const std::string&)> failureCb) = 0;

  bool
  isAvailable() const
  {
    return m_isAvailable;
  }

protected:
  void
  setAvailability(bool isAvailable);

public:
  /** \brief signals when the destination becomes available or unavailable
   */
  signal::Signal<ReadvertiseDestination, bool> afterAvailabilityChange;

private:
  bool m_isAvailable = false;
};

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_READVERTISE_READVERTISE_DESTINATION_HPP

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis
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

#ifndef NFD_CORE_NETWORK_HPP
#define NFD_CORE_NETWORK_HPP

#include "common.hpp"

namespace nfd {

class Network
{
public:
  Network();

  Network(const boost::asio::ip::address& minAddress,
          const boost::asio::ip::address& maxAddress);

  bool
  doesContain(const boost::asio::ip::address& address) const
  {
    return m_minAddress <= address && address <= m_maxAddress;
  }

  static const Network&
  getMaxRangeV4();

  static const Network&
  getMaxRangeV6();

  static bool
  isValidCidr(const std::string& cidr);

  bool
  operator==(const Network& rhs) const
  {
    return m_minAddress == rhs.m_minAddress && m_maxAddress == rhs.m_maxAddress;
  }

  bool
  operator!=(const Network& rhs) const
  {
    return !(*this == rhs);
  }

private:
  boost::asio::ip::address m_minAddress;
  boost::asio::ip::address m_maxAddress;

  friend std::ostream&
  operator<<(std::ostream& os, const Network& network);

  friend std::istream&
  operator>>(std::istream& is, Network& network);
};

std::ostream&
operator<<(std::ostream& os, const Network& network);

std::istream&
operator>>(std::istream& is, Network& network);

} // namespace nfd

#endif // NFD_CORE_NETWORK_HPP

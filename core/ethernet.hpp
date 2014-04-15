/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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

#ifndef NFD_CORE_ETHERNET_HPP
#define NFD_CORE_ETHERNET_HPP

#include "common.hpp"

#include <boost/array.hpp>

#define ETHERTYPE_NDN 0x8624

namespace nfd {
namespace ethernet {

const size_t ADDR_LEN     = 6;      ///< Octets in one Ethernet address
const size_t TYPE_LEN     = 2;      ///< Octets in Ethertype field
const size_t HDR_LEN      = 14;     ///< Total octets in Ethernet header (without 802.1Q tag)
const size_t TAG_LEN      = 4;      ///< Octets in 802.1Q tag (TPID + priority + VLAN)
const size_t MIN_DATA_LEN = 46;     ///< Min octets in Ethernet payload (assuming no 802.1Q tag)
const size_t MAX_DATA_LEN = 1500;   ///< Max octets in Ethernet payload
const size_t CRC_LEN      = 4;      ///< Octets in Ethernet frame check sequence


class Address : public boost::array<uint8_t, ADDR_LEN>
{
public:
  /// Constructs a null Ethernet address (00:00:00:00:00:00)
  Address();

  /// Constructs a new Ethernet address with the given octets
  Address(uint8_t a1, uint8_t a2, uint8_t a3,
          uint8_t a4, uint8_t a5, uint8_t a6);

  /// Constructs a new Ethernet address with the given octets
  explicit
  Address(const uint8_t octets[ADDR_LEN]);

  /// Copy constructor
  Address(const Address& address);

  /// True if this is a broadcast address (ff:ff:ff:ff:ff:ff)
  bool
  isBroadcast() const;

  /// True if this is a multicast address
  bool
  isMulticast() const;

  /// True if this is a null address (00:00:00:00:00:00)
  bool
  isNull() const;

  /**
   * @brief Converts the address to a human-readable string
   *
   * @param sep A character used to visually separate the octets,
   *            usually ':' (the default value) or '-'
   */
  std::string
  toString(char sep = ':') const;

  /**
   * @brief Creates an Address from a string containing an Ethernet address
   *        in hexadecimal notation, with colons or hyphens as separators
   *
   * @param str The string to be parsed
   * @return Always an instance of Address, which will be null
   *         if the parsing fails
   */
  static Address
  fromString(const std::string& str);
};

/// Returns the Ethernet broadcast address (ff:ff:ff:ff:ff:ff)
inline Address
getBroadcastAddress()
{
  static Address bcast(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
  return bcast;
}

/// Returns the default Ethernet multicast address for NDN
inline Address
getDefaultMulticastAddress()
{
  static Address mcast(0x01, 0x00, 0x5E, 0x00, 0x17, 0xAA);
  return mcast;
}

inline
Address::Address()
{
  assign(0);
}

inline
Address::Address(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6)
{
  elems[0] = a1;
  elems[1] = a2;
  elems[2] = a3;
  elems[3] = a4;
  elems[4] = a5;
  elems[5] = a6;
}

inline
Address::Address(const uint8_t octets[])
{
  std::copy(octets, octets + size(), begin());
}

inline
Address::Address(const Address& address)
{
  std::copy(address.begin(), address.end(), begin());
}

inline bool
Address::isBroadcast() const
{
  return elems[0] == 0xFF && elems[1] == 0xFF && elems[2] == 0xFF &&
         elems[3] == 0xFF && elems[4] == 0xFF && elems[5] == 0xFF;
}

inline bool
Address::isMulticast() const
{
  return (elems[0] & 1) != 0;
}

inline bool
Address::isNull() const
{
  return elems[0] == 0x0 && elems[1] == 0x0 && elems[2] == 0x0 &&
         elems[3] == 0x0 && elems[4] == 0x0 && elems[5] == 0x0;
}

std::ostream&
operator<<(std::ostream& o, const Address& a);

} // namespace ethernet
} // namespace nfd

#endif // NFD_FACE_ETHERNET_HPP

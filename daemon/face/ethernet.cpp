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

#include "ethernet.hpp"

#include <stdio.h>

namespace nfd {
namespace ethernet {

std::string
Address::toString(char sep) const
{
  char s[18]; // 12 digits + 5 separators + null terminator
  ::snprintf(s, sizeof(s), "%02x%c%02x%c%02x%c%02x%c%02x%c%02x",
             elems[0], sep, elems[1], sep, elems[2], sep,
             elems[3], sep, elems[4], sep, elems[5]);
  return std::string(s);
}

Address
Address::fromString(const std::string& str)
{
  unsigned short temp[ADDR_LEN];
  char sep[5][2]; // 5 * (1 separator char + 1 null terminator)
  int n = 0; // num of chars read from the input string

  // ISO C++98 does not support the 'hh' type modifier
  /// \todo use SCNx8 (cinttypes) when we enable C++11
  int ret = ::sscanf(str.c_str(), "%2hx%1[:-]%2hx%1[:-]%2hx%1[:-]%2hx%1[:-]%2hx%1[:-]%2hx%n",
                     &temp[0], &sep[0][0], &temp[1], &sep[1][0], &temp[2], &sep[2][0],
                     &temp[3], &sep[3][0], &temp[4], &sep[4][0], &temp[5], &n);

  if (ret < 11 || static_cast<size_t>(n) != str.length())
    return Address();

  Address a;
  for (size_t i = 0; i < ADDR_LEN; ++i)
    {
      // check that all separators are actually the same char (: or -)
      if (i < 5 && sep[i][0] != sep[0][0])
        return Address();

      // check that each value fits into a uint8_t
      if (temp[i] > 0xFF)
        return Address();

      a[i] = static_cast<uint8_t>(temp[i]);
    }

  return a;
}

std::ostream&
operator<<(std::ostream& o, const Address& a)
{
  return o << a.toString();
}

} // namespace ethernet
} // namespace nfd

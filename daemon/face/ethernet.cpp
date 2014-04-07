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
  Address a;
  int n, ret;

  // colon-separated
  ret = ::sscanf(str.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
                 &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &n);
  if (ret >= 6 && str.c_str()[n] == '\0')
    return a;

  // dash-separated
  ret = ::sscanf(str.c_str(), "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx%n",
                 &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &n);
  if (ret >= 6 && str.c_str()[n] == '\0')
    return a;

  return Address();
}

std::ostream&
operator<<(std::ostream& o, const Address& a)
{
  return o << a.toString();
}

} // namespace ethernet
} // namespace nfd

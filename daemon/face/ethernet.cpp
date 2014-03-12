/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

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

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

#include "network-interface-predicate.hpp"

#include "config-file.hpp"
#include "network.hpp"

#include <fnmatch.h>

namespace nfd {

NetworkInterfacePredicate::NetworkInterfacePredicate()
{
  this->clear();
}

void
NetworkInterfacePredicate::clear()
{
  m_whitelist = std::set<std::string>{"*"};
  m_blacklist.clear();
}

static void
parseList(std::set<std::string>& set, const boost::property_tree::ptree& list, const std::string& section)
{
  set.clear();

  for (const auto& item : list) {
    if (item.first == "*") {
      // insert wildcard
      set.insert(item.first);
    }
    else if (item.first == "ifname") {
      // very basic sanity check for interface names
      auto name = item.second.get_value<std::string>();
      if (name.empty()) {
        BOOST_THROW_EXCEPTION(ConfigFile::Error("Empty interface name in \"" + section + "\" section"));
      }
      set.insert(name);
    }
    else if (item.first == "ether") {
      // validate ethernet address
      auto addr = item.second.get_value<std::string>();
      if (ndn::ethernet::Address::fromString(addr).isNull()) {
        BOOST_THROW_EXCEPTION(ConfigFile::Error("Malformed ether address \"" + addr +
                                                "\" in \"" + section + "\" section"));
      }
      set.insert(addr);
    }
    else if (item.first == "subnet") {
      // example subnet: 10.0.0.0/8
      auto cidr = item.second.get_value<std::string>();
      if (!Network::isValidCidr(cidr)) {
        BOOST_THROW_EXCEPTION(ConfigFile::Error("Malformed subnet declaration \"" + cidr +
                                                "\" in \"" + section + "\" section"));
      }
      set.insert(cidr);
    }
  }
}

void
NetworkInterfacePredicate::parseWhitelist(const boost::property_tree::ptree& list)
{
  parseList(m_whitelist, list, "whitelist");
}

void
NetworkInterfacePredicate::parseBlacklist(const boost::property_tree::ptree& list)
{
  parseList(m_blacklist, list, "blacklist");
}

static bool
doesMatchPattern(const std::string& ifname, const std::string& pattern)
{
  // use fnmatch(3) to provide unix glob-style matching for interface names
  // fnmatch returns 0 if there is a match
  return ::fnmatch(pattern.data(), ifname.data(), 0) == 0;
}

static bool
doesMatchRule(const ndn::net::NetworkInterface& netif, const std::string& rule)
{
  // if '/' is in rule, this is a subnet, check if IP in subnet
  if (rule.find('/') != std::string::npos) {
    Network n = boost::lexical_cast<Network>(rule);
    for (const auto& addr : netif.getNetworkAddresses()) {
      if (n.doesContain(addr.getIp())) {
        return true;
      }
    }
  }

  return rule == "*" ||
         doesMatchPattern(netif.getName(), rule) ||
         netif.getEthernetAddress().toString() == rule;
}

bool
NetworkInterfacePredicate::operator()(const ndn::net::NetworkInterface& netif) const
{
  return std::any_of(m_whitelist.begin(), m_whitelist.end(), bind(&doesMatchRule, cref(netif), _1)) &&
         std::none_of(m_blacklist.begin(), m_blacklist.end(), bind(&doesMatchRule, cref(netif), _1));
}

bool
NetworkInterfacePredicate::operator==(const NetworkInterfacePredicate& other) const
{
  return this->m_whitelist == other.m_whitelist &&
         this->m_blacklist == other.m_blacklist;
}

} // namespace nfd

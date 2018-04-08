/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "network-predicate.hpp"

#include "config-file.hpp"
#include "network.hpp"

#include <fnmatch.h>

namespace nfd {

NetworkPredicateBase::NetworkPredicateBase()
{
  this->clear();
}

NetworkPredicateBase::~NetworkPredicateBase() = default;

void
NetworkPredicateBase::clear()
{
  m_whitelist = std::set<std::string>{"*"};
  m_blacklist.clear();
}

void
NetworkPredicateBase::parseList(std::set<std::string>& set, const boost::property_tree::ptree& list, const std::string& section)
{
  set.clear();

  for (const auto& item : list) {
    if (item.first == "*") {
      // insert wildcard
      set.insert(item.first);
    }
    else {
      if (!isRuleSupported(item.first)) {
        BOOST_THROW_EXCEPTION(ConfigFile::Error("Unrecognized rule \"" + item.first + "\" in \"" + section + "\" section"));
      }

      auto value = item.second.get_value<std::string>();
      if (!isRuleValid(item.first, value)) {
        BOOST_THROW_EXCEPTION(ConfigFile::Error("Malformed " + item.first + " \"" + value + "\" in \"" + section + "\" section"));
      }
      set.insert(value);
    }
  }
}

void
NetworkPredicateBase::parseList(std::set<std::string>& set, std::initializer_list<std::pair<std::string, std::string>> list)
{
  set.clear();

  for (const auto& item : list) {
    if (item.first == "*") {
      // insert wildcard
      set.insert(item.first);
    }
    else {
      if (!isRuleSupported(item.first)) {
        BOOST_THROW_EXCEPTION(std::runtime_error("Unrecognized rule \"" + item.first + "\""));
      }

      if (!isRuleValid(item.first, item.second)) {
        BOOST_THROW_EXCEPTION(std::runtime_error("Malformed " + item.first + " \"" + item.second + "\""));
      }
      set.insert(item.second);
    }
  }
}

void
NetworkPredicateBase::parseWhitelist(const boost::property_tree::ptree& list)
{
  parseList(m_whitelist, list, "whitelist");
}

void
NetworkPredicateBase::parseBlacklist(const boost::property_tree::ptree& list)
{
  parseList(m_blacklist, list, "blacklist");
}

void
NetworkPredicateBase::assign(std::initializer_list<std::pair<std::string, std::string>> whitelist,
                             std::initializer_list<std::pair<std::string, std::string>> blacklist)
{
  parseList(m_whitelist, whitelist);
  parseList(m_blacklist, blacklist);
}

bool
NetworkInterfacePredicate::isRuleSupported(const std::string& key)
{
  return key == "ifname" || key == "ether" || key == "subnet";
}

bool
NetworkInterfacePredicate::isRuleValid(const std::string& key, const std::string& value)
{
  if (key == "ifname") {
    // very basic sanity check for interface names
    return !value.empty();
  }
  else if (key == "ether") {
    // validate ethernet address
    return !ndn::ethernet::Address::fromString(value).isNull();
  }
  else if (key == "subnet") {
    // example subnet: 10.0.0.0/8
    return Network::isValidCidr(value);
  }
  else {
    BOOST_THROW_EXCEPTION(std::logic_error("Only supported rules are expected"));
  }
}

bool
IpAddressPredicate::isRuleSupported(const std::string& key)
{
  return key == "subnet";
}

bool
IpAddressPredicate::isRuleValid(const std::string& key, const std::string& value)
{
  if (key == "subnet") {
    // example subnet: 10.0.0.0/8
    return Network::isValidCidr(value);
  }
  else {
    BOOST_THROW_EXCEPTION(std::logic_error("Only supported rules are expected"));
  }
}

bool
NetworkPredicateBase::operator==(const NetworkPredicateBase& other) const
{
  return this->m_whitelist == other.m_whitelist &&
         this->m_blacklist == other.m_blacklist;
}

static bool
doesMatchPattern(const std::string& ifname, const std::string& pattern)
{
  // use fnmatch(3) to provide unix glob-style matching for interface names
  // fnmatch returns 0 if there is a match
  return ::fnmatch(pattern.data(), ifname.data(), 0) == 0;
}

static bool
doesNetifMatchRule(const ndn::net::NetworkInterface& netif, const std::string& rule)
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
  return std::any_of(m_whitelist.begin(), m_whitelist.end(), bind(&doesNetifMatchRule, cref(netif), _1)) &&
         std::none_of(m_blacklist.begin(), m_blacklist.end(), bind(&doesNetifMatchRule, cref(netif), _1));
}

static bool
doesAddressMatchRule(const boost::asio::ip::address& address, const std::string& rule)
{
  // if '/' is in rule, this is a subnet, check if IP in subnet
  if (rule.find('/') != std::string::npos) {
    Network n = boost::lexical_cast<Network>(rule);
    if (n.doesContain(address)) {
      return true;
    }
  }

  return rule == "*";
}

bool
IpAddressPredicate::operator()(const boost::asio::ip::address& address) const
{
  return std::any_of(m_whitelist.begin(), m_whitelist.end(), bind(&doesAddressMatchRule, cref(address), _1)) &&
         std::none_of(m_blacklist.begin(), m_blacklist.end(), bind(&doesAddressMatchRule, cref(address), _1));
}

} // namespace nfd

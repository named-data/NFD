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

#ifndef NFD_CORE_NETWORK_PREDICATE_HPP
#define NFD_CORE_NETWORK_PREDICATE_HPP

#include "common.hpp"
#include <ndn-cxx/net/network-interface.hpp>

namespace nfd {

class NetworkPredicateBase
{
public:
  NetworkPredicateBase();

  virtual
  ~NetworkPredicateBase();

  /**
   * \brief Set the whitelist to "*" and clear the blacklist
   */
  void
  clear();

  void
  parseWhitelist(const boost::property_tree::ptree& list);

  void
  parseBlacklist(const boost::property_tree::ptree& list);

  void
  assign(std::initializer_list<std::pair<std::string, std::string>> whitelist,
         std::initializer_list<std::pair<std::string, std::string>> blacklist);

  bool
  operator==(const NetworkPredicateBase& other) const;

  bool
  operator!=(const NetworkPredicateBase& other) const
  {
    return !this->operator==(other);
  }

private:
  virtual bool
  isRuleSupported(const std::string& key) = 0;

  virtual bool
  isRuleValid(const std::string& key, const std::string& value) = 0;

  void
  parseList(std::set<std::string>& set, const boost::property_tree::ptree& list, const std::string& section);

  void
  parseList(std::set<std::string>& set, std::initializer_list<std::pair<std::string, std::string>> list);

PUBLIC_WITH_TESTS_ELSE_PROTECTED:
  std::set<std::string> m_whitelist;
  std::set<std::string> m_blacklist;
};

/**
 * \brief Represents a predicate to accept or reject a ndn::net::NetworkInterface.
 *
 * The predicate consists of a whitelist and a blacklist. Whitelist and blacklist can contain,
 * in no particular order, interface names (e.g., `ifname eth0`), MAC addresses (e.g., `ether
 * 85:3b:4d:d3:5f:c2`), IPv4 and IPv6 subnets (e.g., `subnet 192.0.2.0/24` or `subnet
 * 2001:db8:2::/64`), or a wildcard (`*`) that matches all interfaces. A
 * ndn::net::NetworkInterface is accepted if it matches any entry in the whitelist and none of
 * the entries in the blacklist.
 */
class NetworkInterfacePredicate : public NetworkPredicateBase
{
public:
  bool
  operator()(const ndn::net::NetworkInterface& netif) const;

private:
  bool
  isRuleSupported(const std::string& key) final;

  bool
  isRuleValid(const std::string& key, const std::string& value) final;
};

/**
 * \brief Represents a predicate to accept or reject an IP address.
 *
 * The predicate consists of a whitelist and a blacklist. Whitelist and blacklist can contain,
 * in no particular order, IPv4 and IPv6 subnets (e.g., `subnet 192.0.2.0/24` or `subnet
 * 2001:db8:2::/64`) or a wildcard (`*`) that matches all IP addresses. An IP address is
 * accepted if it matches any entry in the whitelist and none of the entries in the blacklist.
 */
class IpAddressPredicate : public NetworkPredicateBase
{
public:
  bool
  operator()(const boost::asio::ip::address& address) const;

private:
  bool
  isRuleSupported(const std::string& key) final;

  bool
  isRuleValid(const std::string& key, const std::string& value) final;
};

} // namespace nfd

#endif // NFD_CORE_NETWORK_PREDICATE_HPP

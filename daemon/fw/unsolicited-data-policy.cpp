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

#include "unsolicited-data-policy.hpp"

namespace nfd {
namespace fw {

std::ostream&
operator<<(std::ostream& os, UnsolicitedDataDecision d)
{
  switch (d) {
    case UnsolicitedDataDecision::DROP:
      return os << "drop";
    case UnsolicitedDataDecision::CACHE:
      return os << "cache";
  }
  return os << static_cast<int>(d);
}

UnsolicitedDataPolicy::Registry&
UnsolicitedDataPolicy::getRegistry()
{
  static Registry registry;
  return registry;
}

unique_ptr<UnsolicitedDataPolicy>
UnsolicitedDataPolicy::create(const std::string& key)
{
  Registry& registry = getRegistry();
  auto i = registry.find(key);
  return i == registry.end() ? nullptr : i->second();
}

NFD_REGISTER_UNSOLICITED_DATA_POLICY(DropAllUnsolicitedDataPolicy, "drop-all");

UnsolicitedDataDecision
DropAllUnsolicitedDataPolicy::decide(const Face& inFace, const Data& data) const
{
  return UnsolicitedDataDecision::DROP;
}

NFD_REGISTER_UNSOLICITED_DATA_POLICY(AdmitLocalUnsolicitedDataPolicy, "admit-local");

UnsolicitedDataDecision
AdmitLocalUnsolicitedDataPolicy::decide(const Face& inFace, const Data& data) const
{
  if (inFace.getScope() == ndn::nfd::FACE_SCOPE_LOCAL) {
    return UnsolicitedDataDecision::CACHE;
  }
  return UnsolicitedDataDecision::DROP;
}

NFD_REGISTER_UNSOLICITED_DATA_POLICY(AdmitNetworkUnsolicitedDataPolicy, "admit-network");

UnsolicitedDataDecision
AdmitNetworkUnsolicitedDataPolicy::decide(const Face& inFace, const Data& data) const
{
  if (inFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL) {
    return UnsolicitedDataDecision::CACHE;
  }
  return UnsolicitedDataDecision::DROP;
}

NFD_REGISTER_UNSOLICITED_DATA_POLICY(AdmitAllUnsolicitedDataPolicy, "admit-all");

UnsolicitedDataDecision
AdmitAllUnsolicitedDataPolicy::decide(const Face& inFace, const Data& data) const
{
  return UnsolicitedDataDecision::CACHE;
}

} // namespace fw
} // namespace nfd

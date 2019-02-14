/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FACE_NETDEV_BOUND_HPP
#define NFD_DAEMON_FACE_NETDEV_BOUND_HPP

#include "protocol-factory.hpp"

namespace nfd {
namespace face {

class FaceSystem;

/** \brief manages netdev-bound faces
 */
class NetdevBound : noncopyable
{
public:
  class RuleParseError : public ConfigFile::Error
  {
  public:
    RuleParseError(int index, std::string msg)
      : Error("Error parsing face_system.netdev_bound.rule[" + to_string(index) + "]: " + msg)
      , index(index)
      , msg(std::move(msg))
    {
    }

  public:
    int index;
    std::string msg;
  };

  NetdevBound(const ProtocolFactoryCtorParams& params, const FaceSystem& faceSystem);

  /** \brief process face_system.netdev_bound config section
   */
  void
  processConfig(OptionalConfigSection configSection,
                FaceSystem::ConfigContext& context);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  struct Rule
  {
    std::vector<FaceUri> remotes;
    NetworkInterfacePredicate netifPredicate;
  };

  Rule
  parseRule(int index, const ConfigSection& confRule) const;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  const FaceSystem& m_faceSystem;
  FaceCreatedCallback m_addFace;
  shared_ptr<ndn::net::NetworkMonitor> m_netmon;

  std::vector<Rule> m_rules;

  using Key = std::pair<FaceUri, std::string>;
  std::map<Key, shared_ptr<Face>> m_faces;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_NETDEV_BOUND_HPP

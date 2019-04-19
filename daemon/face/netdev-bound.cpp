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

#include "netdev-bound.hpp"
#include "face-system.hpp"
#include "common/logger.hpp"

namespace nfd {
namespace face {

NFD_LOG_INIT(NetdevBound);

NetdevBound::NetdevBound(const ProtocolFactoryCtorParams& params, const FaceSystem& faceSystem)
  : m_faceSystem(faceSystem)
  , m_addFace(params.addFace)
  , m_netmon(params.netmon)
{
}

void
NetdevBound::processConfig(OptionalConfigSection configSection,
                           FaceSystem::ConfigContext& context)
{
  std::vector<Rule> rules;
  if (configSection) {
    int ruleIndex = 0;
    for (const auto& pair : *configSection) {
      const std::string& key = pair.first;
      const ConfigSection& value = pair.second;
      if (key == "rule") {
        rules.push_back(parseRule(ruleIndex++, value));
      }
      else {
        NDN_THROW(ConfigFile::Error("Unrecognized option face_system.netdev_bound." + key));
      }
    }
  }

  if (context.isDryRun) {
    return;
  }

  ///\todo #3521 this should be verified in dry-run, but PFs won't publish their *+dev schemes
  ///            in dry-run
  for (size_t i = 0; i < rules.size(); ++i) {
    const Rule& rule = rules[i];
    for (const FaceUri& remote : rule.remotes) {
      std::string devScheme = remote.getScheme() + "+dev";
      if (!m_faceSystem.hasFactoryForScheme(devScheme)) {
        NDN_THROW(RuleParseError(i, "scheme '" + devScheme + "' for " +
                                 remote.toString() + " is unavailable"));
      }
    }
  }
  NFD_LOG_DEBUG("processConfig: processed " << rules.size() << " rules");

  m_rules.swap(rules);
  std::map<Key, shared_ptr<Face>> oldFaces;
  oldFaces.swap(m_faces);

  ///\todo #3521 for each face needed under m_rules:
  ///            if it's in oldFaces, add to m_faces and remove from oldFaces;
  ///            otherwise, create via factory and add to m_faces

  ///\todo #3521 close faces in oldFaces
}

NetdevBound::Rule
NetdevBound::parseRule(int index, const ConfigSection& confRule) const
{
  Rule rule;

  bool hasWhitelist = false;
  bool hasBlacklist = false;
  for (const auto& pair : confRule) {
    const std::string& key = pair.first;
    const ConfigSection& value = pair.second;
    if (key == "remote") {
      try {
        rule.remotes.emplace_back(value.get_value<std::string>());
      }
      catch (const FaceUri::Error&) {
        NDN_THROW_NESTED(RuleParseError(index, "invalid remote FaceUri"));
      }
      if (!rule.remotes.back().isCanonical()) {
        NDN_THROW(RuleParseError(index, "remote FaceUri is not canonical"));
      }
    }
    else if (key == "whitelist") {
      if (hasWhitelist) {
        NDN_THROW(RuleParseError(index, "duplicate whitelist"));
      }
      try {
        rule.netifPredicate.parseWhitelist(value);
      }
      catch (const ConfigFile::Error&) {
        NDN_THROW_NESTED(RuleParseError(index, "invalid whitelist"));
      }
      hasWhitelist = true;
    }
    else if (key == "blacklist") {
      if (hasBlacklist) {
        NDN_THROW(RuleParseError(index, "duplicate blacklist"));
      }
      try {
        rule.netifPredicate.parseBlacklist(value);
      }
      catch (const ConfigFile::Error&) {
        NDN_THROW_NESTED(RuleParseError(index, "invalid blacklist"));
      }
      hasBlacklist = true;
    }
    else {
      NDN_THROW(RuleParseError(index, "unrecognized option " + key));
    }
  }

  if (rule.remotes.empty()) {
    NDN_THROW(RuleParseError(index, "remote FaceUri is missing"));
  }

  ///\todo #3521 for each remote, check that there is a factory providing scheme+dev scheme
  return rule;
}

} // namespace face
} // namespace nfd

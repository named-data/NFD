/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology,
 *                     The University of Memphis
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

#ifndef NFD_RIB_RIB_MANAGER_HPP
#define NFD_RIB_RIB_MANAGER_HPP

#include "rib.hpp"
#include "face-monitor.hpp"
#include "core/config-file.hpp"

#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/management/nfd-controller.hpp>
#include <ndn-cxx/management/nfd-control-command.hpp>
#include <ndn-cxx/management/nfd-control-response.hpp>
#include <ndn-cxx/management/nfd-control-parameters.hpp>

namespace nfd {
namespace rib {

using ndn::nfd::ControlCommand;
using ndn::nfd::ControlResponse;
using ndn::nfd::ControlParameters;

class RibManager : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  RibManager();

  void
  registerWithNfd();

  void
  enableLocalControlHeader();

  void
  setConfigFile(ConfigFile& configFile);

private:
  void
  onConfig(const ConfigSection& configSection,
           bool isDryRun,
           const std::string& filename);

  void
  onLocalhopRequest(const Interest& request);

  void
  onLocalhostRequest(const Interest& request);

  void
  sendResponse(const Name& name,
               const ControlResponse& response);

  void
  sendResponse(const Name& name,
               uint32_t code,
               const std::string& text);

  void
  registerEntry(const shared_ptr<const Interest>& request,
                ControlParameters& parameters);

  void
  unregisterEntry(const shared_ptr<const Interest>& request,
                  ControlParameters& parameters);

  void
  onCommandValidated(const shared_ptr<const Interest>& request);

  void
  onCommandValidationFailed(const shared_ptr<const Interest>& request,
                            const std::string& failureInfo);


  void
  onCommandError(uint32_t code, const std::string& error,
                 const shared_ptr<const Interest>& request,
                 const RibEntry& ribEntry);

  void
  onRegSuccess(const shared_ptr<const Interest>& request,
               const ControlParameters& parameters,
               const RibEntry& ribEntry);

  void
  onUnRegSuccess(const shared_ptr<const Interest>& request,
                 const ControlParameters& parameters,
                 const RibEntry& ribEntry);

  void
  onNrdCommandPrefixAddNextHopSuccess(const Name& prefix);

  void
  onNrdCommandPrefixAddNextHopError(const Name& name, const std::string& msg);

  void
  onAddNextHopSuccess(const Name& prefix);

  void
  onAddNextHopError(const Name& name, const std::string& msg);

  void
  onControlHeaderSuccess();

  void
  onControlHeaderError(uint32_t code, const std::string& reason);
  static bool
  extractParameters(const Name::Component& parameterComponent,
                    ControlParameters& extractedParameters);

  bool
  validateParameters(const ControlCommand& command,
                     ControlParameters& parameters);

  void
  onNotification(const FaceEventNotification& notification);

private:
  Rib m_managedRib;
  ndn::Face m_face;
  ndn::nfd::Controller m_nfdController;
  ndn::KeyChain m_keyChain;
  ndn::ValidatorConfig m_localhostValidator;
  ndn::ValidatorConfig m_localhopValidator;
  FaceMonitor m_faceMonitor;
  bool m_isLocalhopEnabled;

  typedef function<void(RibManager*,
                        const shared_ptr<const Interest>& request,
                        ControlParameters& parameters)> VerbProcessor;

  typedef std::map<name::Component, VerbProcessor> VerbDispatchTable;

  typedef std::pair<name::Component, VerbProcessor> VerbAndProcessor;


  const VerbDispatchTable m_verbDispatch;

  static const Name COMMAND_PREFIX; // /localhost/nrd
  static const Name REMOTE_COMMAND_PREFIX; // /localhop/nrd

  // number of components in an invalid, but not malformed, unsigned command.
  // (/localhost/nrd + verb + options) = 4
  static const size_t COMMAND_UNSIGNED_NCOMPS;

  // number of components in a valid signed Interest.
  // 8 with signed Interest support.
  static const size_t COMMAND_SIGNED_NCOMPS;

  static const VerbAndProcessor COMMAND_VERBS[];
};

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_RIB_MANAGER_HPP

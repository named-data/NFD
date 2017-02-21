/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#ifndef NFD_RIB_RIB_MANAGER_HPP
#define NFD_RIB_RIB_MANAGER_HPP

#include "auto-prefix-propagator.hpp"
#include "fib-updater.hpp"
#include "rib.hpp"

#include "core/config-file.hpp"
#include "core/manager-base.hpp"

#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/mgmt/nfd/face-event-notification.hpp>
#include <ndn-cxx/mgmt/nfd/face-monitor.hpp>
#include <ndn-cxx/util/scheduler-scoped-event-id.hpp>

namespace nfd {
namespace rib {

class AutoPrefixPropagator;
class Readvertise;

class RibManager : public nfd::ManagerBase
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

public:
  RibManager(Dispatcher& dispatcher, ndn::Face& face, ndn::KeyChain& keyChain);

  ~RibManager() override;

  void
  registerWithNfd();

  void
  enableLocalFields();

  void
  setConfigFile(ConfigFile& configFile);

  void
  onRibUpdateSuccess(const RibUpdate& update);

  void
  onRibUpdateFailure(const RibUpdate& update, uint32_t code, const std::string& error);

private: // initialization helpers
  void
  onConfig(const ConfigSection& configSection, bool isDryRun, const std::string& filename);

  void
  registerTopPrefix(const Name& topPrefix);

private: // ControlCommand and StatusDataset
  void
  registerEntry(const Name& topPrefix, const Interest& interest,
                ControlParameters parameters,
                const ndn::mgmt::CommandContinuation& done);

  void
  unregisterEntry(const Name& topPrefix, const Interest& interest,
                  ControlParameters parameters,
                  const ndn::mgmt::CommandContinuation& done);

  void
  listEntries(const Name& topPrefix, const Interest& interest,
              ndn::mgmt::StatusDatasetContext& context);

  void
  setFaceForSelfRegistration(const Interest& request, ControlParameters& parameters);

  ndn::mgmt::Authorization
  makeAuthorization(const std::string& verb) override;

private: // Face monitor
  void
  fetchActiveFaces();

  void
  onFetchActiveFacesFailure(uint32_t code, const std::string& reason);

  void
  onFaceDestroyedEvent(uint64_t faceId);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  scheduleActiveFaceFetch(const time::seconds& timeToWait);

  /**
   * @brief remove invalid faces
   *
   * @param status Face dataset
  */
  void
  removeInvalidFaces(const std::vector<ndn::nfd::FaceStatus>& activeFaces);

  /**
   * @brief response to face events
   *
   * @param notification
   */
  void
  onNotification(const ndn::nfd::FaceEventNotification& notification);

private:
  void
  onCommandPrefixAddNextHopSuccess(const Name& prefix, const ControlParameters& result);

  void
  onCommandPrefixAddNextHopError(const Name& name, const ControlResponse& response);

  void
  onEnableLocalFieldsSuccess();

  void
  onEnableLocalFieldsError(const ControlResponse& response);

private:
  ndn::Face& m_face;
  ndn::KeyChain& m_keyChain;
  ndn::nfd::Controller m_nfdController;
  ndn::nfd::FaceMonitor m_faceMonitor;
  ndn::ValidatorConfig m_localhostValidator;
  ndn::ValidatorConfig m_localhopValidator;
  bool m_isLocalhopEnabled;
  AutoPrefixPropagator m_prefixPropagator;
  unique_ptr<Readvertise> m_readvertiseNlsr;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  Rib m_rib;
  FibUpdater m_fibUpdater;

private:
  static const Name LOCAL_HOST_TOP_PREFIX;
  static const Name LOCAL_HOP_TOP_PREFIX;
  static const std::string MGMT_MODULE_NAME;
  static const Name FACES_LIST_DATASET_PREFIX;
  static const time::seconds ACTIVE_FACE_FETCH_INTERVAL;
  scheduler::ScopedEventId m_activeFaceFetchEvent;
  static const Name READVERTISE_NLSR_PREFIX;

  typedef std::set<uint64_t> FaceIdSet;
  /** \brief contains FaceIds with one or more Routes in the RIB
  */
  FaceIdSet m_registeredFaces;

  std::function<void(const Name& topPrefix)> m_addTopPrefix;
};

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_RIB_MANAGER_HPP

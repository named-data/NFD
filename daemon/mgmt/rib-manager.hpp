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

#ifndef NFD_DAEMON_MGMT_RIB_MANAGER_HPP
#define NFD_DAEMON_MGMT_RIB_MANAGER_HPP

#include "manager-base.hpp"
#include "rib/route.hpp"

#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/mgmt/nfd/face-event-notification.hpp>
#include <ndn-cxx/mgmt/nfd/face-monitor.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/util/scheduler.hpp>

namespace nfd {

namespace rib {
class Rib;
class RibUpdate;
} // namespace rib

/**
 * @brief Implements the RIB Management of NFD Management Protocol.
 * @sa https://redmine.named-data.net/projects/nfd/wiki/RibMgmt
 */
class RibManager : public ManagerBase
{
public:
  RibManager(rib::Rib& rib, ndn::Face& face, ndn::KeyChain& keyChain,
             ndn::nfd::Controller& nfdController, Dispatcher& dispatcher);

  /**
   * @brief Apply localhost_security configuration.
   */
  void
  applyLocalhostConfig(const ConfigSection& section, const std::string& filename);

  /**
   * @brief Apply localhop_security configuration and allow accepting commands on
   *        /localhop/nfd/rib prefix.
   */
  void
  enableLocalhop(const ConfigSection& section, const std::string& filename);

  /**
   * @brief Disallow accepting commands on /localhop/nfd/rib prefix.
   */
  void
  disableLocalhop();

  /**
   * @brief Apply prefix_announcement_validation configuration.
   */
  void
  applyPaConfig(const ConfigSection& section, const std::string& filename);

  /**
   * @brief Start accepting commands and dataset requests.
   */
  void
  registerWithNfd();

  /**
   * @brief Enable NDNLP IncomingFaceId field in order to support self-registration commands.
   */
  void
  enableLocalFields();

public: // self-learning support
  enum class SlAnnounceResult {
    OK,                 ///< RIB and FIB have been updated
    ERROR,              ///< unspecified error
    VALIDATION_FAILURE, ///< the announcement cannot be verified against the trust schema
    EXPIRED,            ///< the announcement has expired
    NOT_FOUND,          ///< route does not exist (slRenew only)
  };

  using SlAnnounceCallback = std::function<void(SlAnnounceResult res)>;
  using SlFindAnnCallback = std::function<void(optional<ndn::PrefixAnnouncement>)>;

  /** \brief Insert a route by prefix announcement from self-learning strategy.
   *  \param pa A prefix announcement. It must contain the Data.
   *  \param faceId Face on which the announcement arrives.
   *  \param maxLifetime Maximum route lifetime as imposed by self-learning strategy.
   *  \param cb Callback to receive the operation result.
   *
   *  If \p pa passes validation and is unexpired, inserts or replaces a route for the announced
   *  name and faceId whose lifetime is set to the earlier of now+maxLifetime or prefix
   *  announcement expiration time, updates FIB, and invokes \p cb with SlAnnounceResult::OK.
   *  In case \p pa expires when validation completes, invokes \p cb with SlAnnounceResult::EXPIRED.
   *  If \p pa cannot be verified by the trust schema given in rib.localhop_security config key,
   *  or the relevant config has not been loaded via \c enableLocalHop, invokes \p cb with
   *  SlAnnounceResult::VALIDATION_FAILURE.
   *
   *  Self-learning strategy invokes this method after receiving a Data carrying a prefix
   *  announcement.
   */
  void
  slAnnounce(const ndn::PrefixAnnouncement& pa, uint64_t faceId, time::milliseconds maxLifetime,
             const SlAnnounceCallback& cb);

  /** \brief Renew a route created by prefix announcement from self-learning strategy.
   *  \param name Data name, for finding RIB entry by longest-prefix-match.
   *  \param faceId Nexthop face.
   *  \param maxLifetime Maximum route lifetime as imposed by self-learning strategy.
   *  \param cb Callback to receive the operation result.
   *
   *  If the specified route exists, prolongs its lifetime to the earlier of now+maxLifetime or
   *  prefix announcement expiration time, and invokes \p cb with SlAnnounceResult::OK.
   *  If the prefix announcement has expired, invokes \p cb with SlAnnounceResult::EXPIRED.
   *  If the route is not found, invokes \p cb with SlAnnounceResult::NOT_FOUND.
   *
   *  Self-learning strategy invokes this method after an Interest forwarded via a learned route
   *  is satisfied.
   *
   *  \bug In current implementation, if an slAnnounce operation is in progress to create a Route
   *       or replace a prefix announcement, slRenew could fail because Route does not exist in
   *       existing RIB, or overwrite the new prefix announcement with an old one.
   */
  void
  slRenew(const Name& name, uint64_t faceId, time::milliseconds maxLifetime,
          const SlAnnounceCallback& cb);

  /** \brief Retrieve an outgoing prefix announcement for self-learning strategy.
   *  \param name Data name.
   *  \param cb Callback to receive a prefix announcement that announces a prefix of \p name, or
   *            nullopt if no RIB entry is found by longest-prefix-match of \p name.
   *
   *  Self-learning strategy invokes this method before sending a Data in reply to a discovery
   *  Interest, so as to attach a prefix announcement onto that Data.
   *
   *  \bug In current implementation, if an slAnnounce operation is in progress, slFindAnn does not
   *       wait for that operation to complete and its result reflects the prior RIB state.
   */
  void
  slFindAnn(const Name& name, const SlFindAnnCallback& cb) const;

private: // RIB and FibUpdater actions
  enum class RibUpdateResult
  {
    OK,
    ERROR,
    EXPIRED,
  };

  static SlAnnounceResult
  getSlAnnounceResultFromRibUpdateResult(RibUpdateResult r);

  /** \brief Start adding a route to RIB and FIB.
   *  \param name route name
   *  \param route route parameters; may contain absolute expiration time
   *  \param expires relative expiration time; if specified, overwrites \c route.expires
   *  \param done completion callback
   */
  void
  beginAddRoute(const Name& name, rib::Route route, optional<time::nanoseconds> expires,
                const std::function<void(RibUpdateResult)>& done);

  /** \brief Start removing a route from RIB and FIB.
   *  \param name route name
   *  \param route route parameters
   *  \param done completion callback
   */
  void
  beginRemoveRoute(const Name& name, const rib::Route& route,
                   const std::function<void(RibUpdateResult)>& done);

  void
  beginRibUpdate(const rib::RibUpdate& update,
                 const std::function<void(RibUpdateResult)>& done);

private: // management Dispatcher related
  void
  registerTopPrefix(const Name& topPrefix);

  /** \brief Serve rib/register command.
   */
  void
  registerEntry(const Name& topPrefix, const Interest& interest,
                ControlParameters parameters,
                const ndn::mgmt::CommandContinuation& done);

  /** \brief Serve rib/unregister command.
   */
  void
  unregisterEntry(const Name& topPrefix, const Interest& interest,
                  ControlParameters parameters,
                  const ndn::mgmt::CommandContinuation& done);

  /** \brief Serve rib/list dataset.
   */
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

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  scheduleActiveFaceFetch(const time::seconds& timeToWait);

  void
  removeInvalidFaces(const std::vector<ndn::nfd::FaceStatus>& activeFaces);

  void
  onNotification(const ndn::nfd::FaceEventNotification& notification);

public:
  static const Name LOCALHOP_TOP_PREFIX;

private:
  rib::Rib& m_rib;
  ndn::KeyChain& m_keyChain;
  ndn::nfd::Controller& m_nfdController;
  Dispatcher& m_dispatcher;

  ndn::nfd::FaceMonitor m_faceMonitor;
  ndn::ValidatorConfig m_localhostValidator;
  ndn::ValidatorConfig m_localhopValidator;
  ndn::ValidatorConfig m_paValidator;
  bool m_isLocalhopEnabled;

  scheduler::ScopedEventId m_activeFaceFetchEvent;
};

std::ostream&
operator<<(std::ostream& os, RibManager::SlAnnounceResult res);

} // namespace nfd

#endif // NFD_DAEMON_MGMT_RIB_MANAGER_HPP

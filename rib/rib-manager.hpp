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

/**
 * @brief Serve commands and datasets in NFD RIB management protocol.
 */
class RibManager : public nfd::ManagerBase
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  RibManager(Rib& rib, ndn::Face& face, ndn::nfd::Controller& nfdController, Dispatcher& dispatcher);

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
   * @brief Start accepting commands and dataset requests.
   */
  void
  registerWithNfd();

  /**
   * @brief Enable NDNLP IncomingFaceId field in order to support self-registration commands.
   */
  void
  enableLocalFields();

private: // RIB and FibUpdater actions
  enum class RibUpdateResult
  {
    OK,
    ERROR,
    EXPIRED,
  };

  /** \brief Start adding a route to RIB and FIB.
   *  \param name route name
   *  \param route route parameters; may contain absolute expiration time
   *  \param expires relative expiration time; if specified, overwrites \c route.expires
   *  \param done completion callback
   */
  void
  beginAddRoute(const Name& name, Route route, optional<time::nanoseconds> expires,
                const std::function<void(RibUpdateResult)>& done);

  /** \brief Start removing a route from RIB and FIB.
   *  \param name route name
   *  \param route route parameters
   *  \param done completion callback
   */
  void
  beginRemoveRoute(const Name& name, const Route& route,
                   const std::function<void(RibUpdateResult)>& done);

  void
  beginRibUpdate(const RibUpdate& update, const std::function<void(RibUpdateResult)>& done);

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
  Rib& m_rib;
  ndn::nfd::Controller& m_nfdController;
  Dispatcher& m_dispatcher;

  ndn::nfd::FaceMonitor m_faceMonitor;
  ndn::ValidatorConfig m_localhostValidator;
  ndn::ValidatorConfig m_localhopValidator;
  bool m_isLocalhopEnabled;

private:
  scheduler::ScopedEventId m_activeFaceFetchEvent;

  typedef std::set<uint64_t> FaceIdSet;
  /** \brief contains FaceIds with one or more Routes in the RIB
  */
  FaceIdSet m_registeredFaces;
};

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_RIB_MANAGER_HPP

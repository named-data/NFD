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

#ifndef NFD_TESTS_DAEMON_RIB_FIB_UPDATES_COMMON_HPP
#define NFD_TESTS_DAEMON_RIB_FIB_UPDATES_COMMON_HPP

#include "rib/fib-updater.hpp"
#include "common/global.hpp"

#include "tests/key-chain-fixture.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/rib/create-route.hpp"

#include <ndn-cxx/util/dummy-client-face.hpp>

namespace nfd {
namespace rib {
namespace tests {

using namespace nfd::tests;

class MockFibUpdater : public FibUpdater
{
public:
  using FibUpdater::FibUpdater;

  void
  sortUpdates()
  {
    updates.sort([] (const auto& lhs, const auto& rhs) {
      return std::tie(lhs.name, lhs.faceId, lhs.cost, lhs.action) <
             std::tie(rhs.name, rhs.faceId, rhs.cost, rhs.action);
    });
  }

private:
  void
  sendAddNextHopUpdate(const FibUpdate& update,
                       const FibUpdateSuccessCallback& onSuccess,
                       const FibUpdateFailureCallback& onFailure,
                       uint32_t nTimeouts) override
  {
    mockUpdate(update, onSuccess, onFailure, nTimeouts);
  }

  void
  sendRemoveNextHopUpdate(const FibUpdate& update,
                          const FibUpdateSuccessCallback& onSuccess,
                          const FibUpdateFailureCallback& onFailure,
                          uint32_t nTimeouts) override
  {
    mockUpdate(update, onSuccess, onFailure, nTimeouts);
  }

  void
  mockUpdate(const FibUpdate& update,
             const FibUpdateSuccessCallback& onSuccess,
             const FibUpdateFailureCallback& onFailure,
             uint32_t nTimeouts)
  {
    updates.push_back(update);
    getGlobalIoService().post([=] {
      if (mockSuccess) {
        onUpdateSuccess(update, onSuccess, onFailure);
      }
      else {
        ndn::mgmt::ControlResponse resp(504, "mocked failure");
        onUpdateError(update, onSuccess, onFailure, resp, nTimeouts);
      }
    });
  }

public:
  FibUpdateList updates;
  bool mockSuccess = true;
};

class FibUpdatesFixture : public GlobalIoFixture, public KeyChainFixture
{
public:
  FibUpdatesFixture()
    : face(g_io, m_keyChain)
    , controller(face, m_keyChain)
    , fibUpdater(rib, controller)
  {
  }

  void
  insertRoute(const Name& name, uint64_t faceId,
              std::underlying_type_t<ndn::nfd::RouteOrigin> origin,
              uint64_t cost,
              std::underlying_type_t<ndn::nfd::RouteFlags> flags)
  {
    Route route = createRoute(faceId, origin, cost, flags);

    RibUpdate update;
    update.setAction(RibUpdate::REGISTER)
          .setName(name)
          .setRoute(route);

    rib.beginApplyUpdate(update, nullptr, nullptr);
    pollIo();
  }

  void
  eraseRoute(const Name& name, uint64_t faceId,
             std::underlying_type_t<ndn::nfd::RouteOrigin> origin)
  {
    Route route = createRoute(faceId, origin, 0, 0);

    RibUpdate update;
    update.setAction(RibUpdate::UNREGISTER)
          .setName(name)
          .setRoute(route);

    rib.beginApplyUpdate(update, nullptr, nullptr);
    pollIo();
  }

  void
  destroyFace(uint64_t faceId)
  {
    rib.beginRemoveFace(faceId);
    pollIo();
  }

  const FibUpdater::FibUpdateList&
  getFibUpdates() const
  {
    return fibUpdater.updates;
  }

  const FibUpdater::FibUpdateList&
  getSortedFibUpdates()
  {
    fibUpdater.sortUpdates();
    return fibUpdater.updates;
  }

  void
  clearFibUpdates()
  {
    fibUpdater.updates.clear();
  }

public:
  ndn::util::DummyClientFace face;
  ndn::nfd::Controller controller;

  Rib rib;
  MockFibUpdater fibUpdater;
};

} // namespace tests
} // namespace rib
} // namespace nfd

#endif // NFD_TESTS_DAEMON_RIB_FIB_UPDATES_COMMON_HPP

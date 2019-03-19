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

#include "daemon/global.hpp"

namespace nfd {

static thread_local unique_ptr<boost::asio::io_service> g_ioService;
static thread_local unique_ptr<Scheduler> g_scheduler;
static boost::asio::io_service* g_mainIoService = nullptr;
static boost::asio::io_service* g_ribIoService = nullptr;

boost::asio::io_service&
getGlobalIoService()
{
  if (g_ioService == nullptr) {
    g_ioService = make_unique<boost::asio::io_service>();
  }
  return *g_ioService;
}

Scheduler&
getScheduler()
{
  if (g_scheduler == nullptr) {
    g_scheduler = make_unique<Scheduler>(getGlobalIoService());
  }
  return *g_scheduler;
}

#ifdef WITH_TESTS
void
resetGlobalIoService()
{
  g_scheduler.reset();
  g_ioService.reset();
}
#endif

boost::asio::io_service&
getMainIoService()
{
  BOOST_ASSERT(g_mainIoService != nullptr);
  return *g_mainIoService;
}

boost::asio::io_service&
getRibIoService()
{
  BOOST_ASSERT(g_ribIoService != nullptr);
  return *g_ribIoService;
}

void
setMainIoService(boost::asio::io_service* mainIo)
{
  g_mainIoService = mainIo;
}

void
setRibIoService(boost::asio::io_service* ribIo)
{
  g_ribIoService = ribIo;
}

void
runOnMainIoService(const std::function<void()>& f)
{
  getMainIoService().post(f);
}

void
runOnRibIoService(const std::function<void()>& f)
{
  getRibIoService().post(f);
}

} // namespace nfd

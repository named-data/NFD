/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#include "common/global.hpp"

namespace nfd {

static thread_local std::unique_ptr<boost::asio::io_context> g_ioCtx;
static thread_local std::unique_ptr<ndn::Scheduler> g_scheduler;
static boost::asio::io_context* g_mainIoCtx = nullptr;
static boost::asio::io_context* g_ribIoCtx = nullptr;

boost::asio::io_context&
getGlobalIoService()
{
  if (g_ioCtx == nullptr) {
    g_ioCtx = std::make_unique<boost::asio::io_context>();
  }
  return *g_ioCtx;
}

ndn::Scheduler&
getScheduler()
{
  if (g_scheduler == nullptr) {
    g_scheduler = std::make_unique<ndn::Scheduler>(getGlobalIoService());
  }
  return *g_scheduler;
}

#ifdef NFD_WITH_TESTS
void
resetGlobalIoService()
{
  g_scheduler.reset();
  g_ioCtx.reset();
}
#endif

boost::asio::io_context&
getMainIoService()
{
  BOOST_ASSERT(g_mainIoCtx != nullptr);
  return *g_mainIoCtx;
}

boost::asio::io_context&
getRibIoService()
{
  BOOST_ASSERT(g_ribIoCtx != nullptr);
  return *g_ribIoCtx;
}

void
setMainIoService(boost::asio::io_context* mainIo)
{
  g_mainIoCtx = mainIo;
}

void
setRibIoService(boost::asio::io_context* ribIo)
{
  g_ribIoCtx = ribIo;
}

} // namespace nfd

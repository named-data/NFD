/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024  Regents of the University of California,
 *                          Arizona Board of Regents,
 *                          Colorado State University,
 *                          University Pierre & Marie Curie, Sorbonne University,
 *                          Washington University in St. Louis,
 *                          Beijing Institute of Technology
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

#ifndef NFD_DAEMON_COMMON_GLOBAL_HPP
#define NFD_DAEMON_COMMON_GLOBAL_HPP

#include "core/config.hpp"

#include <boost/asio/io_context.hpp>
#include <ndn-cxx/util/scheduler.hpp>


/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * NFD Global IO and Scheduler Access
 * ---------------------------------------------------------------------------
 * 本ファイルでは、Named Data Networking Forwarding Daemon (NFD) 内で
 * グローバルに利用される IO コンテキストとスケジューラへのアクセスを提供します。
 *
 * 提供される主な機能:
 *  1. getGlobalIoService()
 *     - 現在のスレッドで使用されるグローバル io_context を返す
 *
 *  2. getScheduler()
 *     - 現在のスレッドで使用されるグローバル Scheduler を返す
 *
 *  3. getMainIoService()
 *     - メイン処理用の io_context を返す
 *
 *  4. getRibIoService()
 *     - RIB (Routing Information Base) 用の io_context を返す
 *
 *  5. setMainIoService(boost::asio::io_context*)
 *     - メイン用 io_context を設定
 *
 *  6. setRibIoService(boost::asio::io_context*)
 *     - RIB 用 io_context を設定
 *
 *  7. resetGlobalIoService() [テスト用]
 *     - グローバル io_context を破棄し、次回 getGlobalIoService() 呼び出し時に再作成
 *
 * 利用例:
 *  auto& io = nfd::getGlobalIoService();
 *  auto& sched = nfd::getScheduler();
 *  io.post([]{  非同期処理 *});
 *
 * 注意:
 *  - これらのグローバルオブジェクトはスレッドローカルで管理されるため、
 *    マルチスレッド環境での利用時にはスレッドごとに呼び出す必要あり
 * ---------------------------------------------------------------------------
 */

namespace nfd {

/**
 * \brief Returns the global io_context instance for the calling thread.
 */
boost::asio::io_context&
getGlobalIoService();

/**
 * \brief Returns the global Scheduler instance for the calling thread.
 */
ndn::Scheduler&
getScheduler();

boost::asio::io_context&
getMainIoService();

boost::asio::io_context&
getRibIoService();

void
setMainIoService(boost::asio::io_context* mainIo);

void
setRibIoService(boost::asio::io_context* ribIo);

#ifdef NFD_WITH_TESTS
/**
 * \brief Destroy the global io_context instance.
 *
 * It will be recreated at the next invocation of getGlobalIoService().
 */
void
resetGlobalIoService();
#endif

} // namespace nfd

#endif // NFD_DAEMON_COMMON_GLOBAL_HPP

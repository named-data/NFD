/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#ifndef NFD_DAEMON_MGMT_FORWARDER_STATUS_MANAGER_HPP
#define NFD_DAEMON_MGMT_FORWARDER_STATUS_MANAGER_HPP

#include "manager-base.hpp"

#include <ndn-cxx/mgmt/nfd/forwarder-status.hpp>

/*
 * ForwarderStatusManager.hpp
 *
 * 概要:
 *   ForwarderStatusManager は NFD (Named Data Networking Forwarding Daemon) における
 *   Forwarder Status 管理用のクラスです。
 *   NFD Management Protocol に準拠し、フォワーダ全体の状態情報を収集・提供します。
 *
 * 主な役割:
 *   - フォワーダの一般状態情報の収集 (collectGeneralStatus)
 *   - 一般状態情報の提供 (listGeneralStatus)
 *
 * 特徴:
 *   - Forwarder オブジェクトの状態を参照して情報を収集
 *   - Dispatcher を使用して管理プロトコルに基づく応答を実装
 *   - 起動時刻の管理により稼働時間などの統計情報の算出が可能
 *
 * 内部構造:
 *   - m_forwarder: 状態を収集する対象の Forwarder への参照
 *   - m_dispatcher: 管理コマンド応答のための Dispatcher への参照
 *   - m_startTimestamp: フォワーダ起動時刻の記録
 *
 * 参考:
 *   - NFD Management Protocol Wiki: https://redmine.named-data.net/projects/nfd/wiki/ForwarderStatus
 *
 * 注意点:
 *   - コマンド処理や状態提供は StatusDatasetContext を通じて行う
 *   - このクラスはコピー禁止 (noncopyable)
 */

namespace nfd {

class Forwarder;

/**
 * @brief Implements the Forwarder Status of NFD Management Protocol.
 * @sa https://redmine.named-data.net/projects/nfd/wiki/ForwarderStatus
 */
class ForwarderStatusManager : noncopyable
{
public:
  ForwarderStatusManager(Forwarder& forwarder, Dispatcher& dispatcher);

private:
  ndn::nfd::ForwarderStatus
  collectGeneralStatus();

  /**
   * \brief Provides the general status dataset.
   */
  void
  listGeneralStatus(ndn::mgmt::StatusDatasetContext& context);

private:
  Forwarder& m_forwarder;
  Dispatcher& m_dispatcher;
  time::system_clock::time_point m_startTimestamp;
};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_FORWARDER_STATUS_MANAGER_HPP

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#ifndef NFD_DAEMON_RIB_READVERTISE_NFD_RIB_READVERTISE_DESTINATION_HPP
#define NFD_DAEMON_RIB_READVERTISE_NFD_RIB_READVERTISE_DESTINATION_HPP

#include "readvertise-destination.hpp"
#include "rib/rib.hpp"

#include <ndn-cxx/mgmt/nfd/command-options.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/mgmt/nfd/control-parameters.hpp>

/*
 * このファイルは「NfdRibReadvertiseDestination」クラスの定義を含む。
 * 本クラスは、NDNのRIB（Routing Information Base）管理プロトコルを使用して、
 * Prefix（名前プレフィックス）の再広告（Readvertise）を行う役割を担う。
 *
 * ◆役割概要
 * - ReadvertiseDestination クラスを継承し、RIB への経路追加・削除を実装
 * - コントローラ（ndn::nfd::Controller）を介して NFD へ制御命令を送信する
 * - ControlParameters / CommandOptions を利用して NFD RIB 操作に必要な情報を設定
 *
 * ◆主要機能
 * 1. advertise()
 *    - ReadvertisedRoute をもとに NFD RIB にプレフィックスを追加
 *    - 成功時 / 失敗時にコールバックを実行
 *
 * 2. withdraw()
 *    - NFD RIB から該当プレフィックスを削除
 *    - 成功時 / 失敗時にコールバックを実行
 *
 * ◆内部メンバ
 * - m_controller:
 *     RIB 管理コマンドを発行するための管理インタフェース
 * - m_ribInsertConn / m_ribEraseConn:
 *     RIB 更新イベント（追加・削除）との接続管理
 * - m_commandOptions:
 *     コマンド発行時の設定（タイムアウトなど）
 * - m_controlParameters:
 *     経路操作に必要なパラメータ（Origin 属性など）
 *
 * ◆まとめ
 * NFD の管理プロトコルを利用して RIB に対する
 * 「経路の追加」「経路の削除」を自動的に行うための
 * Readvertise 機能を提供するクラス。
 */

namespace nfd::rib {

/** \brief A readvertise destination using NFD RIB management protocol.
 */
class NfdRibReadvertiseDestination : public ReadvertiseDestination
{
public:
  NfdRibReadvertiseDestination(ndn::nfd::Controller& controller,
                               Rib& rib,
                               const ndn::nfd::CommandOptions& options = ndn::nfd::CommandOptions(),
                               const ndn::nfd::ControlParameters& parameters =
                                 ndn::nfd::ControlParameters().setOrigin(ndn::nfd::ROUTE_ORIGIN_CLIENT));

  /** \brief Add a name prefix into NFD RIB.
   */
  void
  advertise(const ReadvertisedRoute& rr,
            std::function<void()> successCb,
            std::function<void(const std::string&)> failureCb) override;

  /** \brief Remove a name prefix from NFD RIB.
   */
  void
  withdraw(const ReadvertisedRoute& rr,
           std::function<void()> successCb,
           std::function<void(const std::string&)> failureCb) override;

protected:
  ndn::nfd::ControlParameters
  getControlParameters() const
  {
    return m_controlParameters;
  }

  ndn::nfd::CommandOptions
  getCommandOptions() const
  {
    return m_commandOptions;
  }

private:
  ndn::nfd::Controller& m_controller;

  signal::ScopedConnection m_ribInsertConn;
  signal::ScopedConnection m_ribEraseConn;

  ndn::nfd::CommandOptions m_commandOptions;
  ndn::nfd::ControlParameters m_controlParameters;
};

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_READVERTISE_NFD_RIB_READVERTISE_DESTINATION_HPP

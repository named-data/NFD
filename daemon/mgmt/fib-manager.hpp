/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2025,  Regents of the University of California,
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

#ifndef NFD_DAEMON_MGMT_FIB_MANAGER_HPP
#define NFD_DAEMON_MGMT_FIB_MANAGER_HPP

#include "manager-base.hpp"

/*
 * FibManager.hpp
 *
 * 概要:
 *   FibManager は NFD (Named Data Networking Forwarding Daemon) における
 *   FIB（Forwarding Information Base）管理用のクラスです。
 *   NFD Management Protocol に準拠し、FIB の次ホップ追加・削除や情報取得を行います。
 *
 * 主な役割:
 *   - FIB エントリの次ホップ追加 (addNextHop)
 *   - FIB エントリの次ホップ削除 (removeNextHop)
 *   - FIB の一覧取得 (listEntries)
 *
 * 特徴:
 *   - Interest および ControlParameters を用いて管理コマンドを処理
 *   - FIB の状態を管理し、必要に応じて FaceTable を参照して登録情報を取得
 *   - 自己登録用の Face 設定をサポート (setFaceForSelfRegistration)
 *
 * 内部構造:
 *   - m_fib: 管理対象の FIB への参照
 *   - m_faceTable: Face の登録テーブルへの参照
 *
 * 参考:
 *   - NFD Management Protocol Wiki: https://redmine.named-data.net/projects/nfd/wiki/FibMgmt
 *
 * 注意点:
 *   - コマンド処理後の結果通知は CommandContinuation を通じて行う
 *   - addNextHop や removeNextHop は Interest パケットに基づいて実行される
 */

namespace nfd {

namespace fib {
class Fib;
} // namespace fib

class FaceTable;

/**
 * @brief Implements the FIB Management of NFD Management Protocol.
 * @sa https://redmine.named-data.net/projects/nfd/wiki/FibMgmt
 */
class FibManager final : public ManagerBase
{
public:
  FibManager(fib::Fib& fib, const FaceTable& faceTable,
             Dispatcher& dispatcher, CommandAuthenticator& authenticator);

private:
  void
  addNextHop(const Interest& interest, ControlParameters parameters,
             const CommandContinuation& done);

  void
  removeNextHop(const Interest& interest, ControlParameters parameters,
                const CommandContinuation& done);

  void
  listEntries(ndn::mgmt::StatusDatasetContext& context);

private:
  void
  setFaceForSelfRegistration(const Interest& request, ControlParameters& parameters);

private:
  fib::Fib& m_fib;
  const FaceTable& m_faceTable;
};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_FIB_MANAGER_HPP

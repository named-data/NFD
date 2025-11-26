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

#ifndef NFD_DAEMON_FW_ASF_PROBING_MODULE_HPP
#define NFD_DAEMON_FW_ASF_PROBING_MODULE_HPP

#include "asf-measurements.hpp"

/* -------------------------------------------------------------------------
 * AsfProbingModule.hpp の解説
 *
 * ■ 概要
 *   - ASF（Adaptive SRTT-based Forwarding）戦略で利用されるプローブ機能。
 *   - FIB に登録された Face の性能（RTT、SRTT、コストなど）を定期的に測定。
 *   - 測定結果は ASF の経路選択に活用される。
 *
 * ■ 主な構造体
 *
 * FaceStats
 *   - Face ごとの RTT・SRTT・コスト情報を保持。
 *   - メンバ:
 *       - face  : Face のポインタ
 *       - rtt   : 最新 RTT
 *       - srtt  : 平滑化 RTT (Smoothed RTT)
 *       - cost  : Face のコスト
 *
 * FaceStatsProbingCompare
 *   - std::set 内で FaceStats を比較・順位付けする関数オブジェクト
 *   - RTT や SRTT を基準に評価
 *
 * ■ 主なクラス
 *
 * ProbingModule
 *   - コンストラクタ: AsfMeasurements への参照を受け取り初期化
 *   - scheduleProbe()  : 指定 FIB エントリの Face に対しプローブをスケジュール
 *   - getFaceToProbe() : 次にプローブすべき Face を決定
 *   - isProbingNeeded(): Interest に対してプローブが必要か判定
 *   - afterForwardingProbe(): プローブ送信後に統計を更新
 *   - set/getProbingInterval(): プローブ間隔を設定／取得
 *
 * ■ メンバ変数
 *   - m_probingInterval : プローブ間隔（time::milliseconds）
 *   - m_measurements    : Face ごとの RTT 統計管理 (AsfMeasurements)
 *
 * ■ 定数
 *   - DEFAULT_PROBING_INTERVAL = 1 分
 *   - MIN_PROBING_INTERVAL     = 1 秒
 *
 * ■ 特徴
 *   - ASF の動的経路選択を補助
 *   - RTT 情報の更新により Interest 転送の最適化を支援
 *   - Face の性能が低下した場合も適切に評価可能
 *
 * ■ 使用例
 *   - AsfStrategy クラス内で m_probing モジュールとして利用
 * ------------------------------------------------------------------------- */

namespace nfd::fw::asf {

/**
 * \brief Container for ranking-related values.
 */
struct FaceStats
{
  Face* face = nullptr;
  time::nanoseconds rtt = 0_ns;
  time::nanoseconds srtt = 0_ns;
  uint64_t cost = 0;
};

/**
 * \brief ASF Probing Module.
 */
class ProbingModule
{
public:
  explicit
  ProbingModule(AsfMeasurements& measurements);

  void
  scheduleProbe(const fib::Entry& fibEntry, time::milliseconds interval);

  Face*
  getFaceToProbe(const Face& inFace, const Interest& interest,
                 const fib::Entry& fibEntry, const Face& faceUsed);

  bool
  isProbingNeeded(const fib::Entry& fibEntry, const Name& interestName);

  void
  afterForwardingProbe(const fib::Entry& fibEntry, const Name& interestName);

  void
  setProbingInterval(time::milliseconds probingInterval);

  time::milliseconds
  getProbingInterval() const
  {
    return m_probingInterval;
  }

public:
  static constexpr time::milliseconds DEFAULT_PROBING_INTERVAL = 1_min;
  static constexpr time::milliseconds MIN_PROBING_INTERVAL = 1_s;

  struct FaceStatsProbingCompare
  {
    bool
    operator()(const FaceStats& lhs, const FaceStats& rhs) const noexcept;
  };
  using FaceStatsProbingSet = std::set<FaceStats, FaceStatsProbingCompare>;

private:
  time::milliseconds m_probingInterval;
  AsfMeasurements& m_measurements;
};

} // namespace nfd::fw::asf

#endif // NFD_DAEMON_FW_ASF_PROBING_MODULE_HPP

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

#ifndef NFD_DAEMON_FW_RETX_SUPPRESSION_FIXED_HPP
#define NFD_DAEMON_FW_RETX_SUPPRESSION_FIXED_HPP

#include "retx-suppression.hpp"
#include "table/pit-entry.hpp"

/* -------------------------------------------------------------------------
 * RetxSuppressionFixed.hpp の解説
 *
 * ■ 概要
 *   NFD における Interest 再送の抑制アルゴリズムの一種。
 *   「一定時間内の再送は抑制する」という単純なルールで
 *   再送 Interest の転送を制御する。
 *
 * ■ クラス: RetxSuppressionFixed
 *
 * public:
 *   - RetxSuppressionFixed(const time::milliseconds& minRetxInterval)
 *       - コンストラクタ
 *       - minRetxInterval: 再送とみなすまでの最小間隔（デフォルト 100ms）
 *
 *   - RetxSuppressionResult decidePerPitEntry(pit::Entry& pitEntry) const
 *       - 指定した PIT エントリに基づき、受信 Interest が新規か再送かを判定
 *       - 判定結果に応じて、NEW / FORWARD / SUPPRESS のいずれかを返す
 *
 * public static:
 *   - DEFAULT_MIN_RETX_INTERVAL = 100ms
 *       - デフォルトの再送抑制間隔
 *
 * private:
 *   - m_minRetxInterval
 *       - 再送とみなす最小間隔を保持
 *
 * ■ 動作のイメージ
 *   - PIT に同じ Interest が登録されてから m_minRetxInterval 内に
 *     再度同じ Interest が来た場合は SUPPRESS
 *   - この時間を過ぎると再送として FORWARD される場合もある
 *
 * ■ まとめ
 *   - 固定時間ルールによる簡単で効率的な再送抑制
 *   - Interest のループや無駄な転送を減らす
 * ------------------------------------------------------------------------- */

namespace nfd::fw {

/**
 * \brief A retransmission suppression decision algorithm that
 *        suppresses retransmissions within a fixed duration.
 */
class RetxSuppressionFixed
{
public:
  explicit
  RetxSuppressionFixed(const time::milliseconds& minRetxInterval = DEFAULT_MIN_RETX_INTERVAL);

  /** \brief Determines whether Interest is a retransmission,
   *         and if so, whether it shall be forwarded or suppressed.
   */
  RetxSuppressionResult
  decidePerPitEntry(pit::Entry& pitEntry) const;

public:
  static constexpr time::milliseconds DEFAULT_MIN_RETX_INTERVAL = 100_ms;

private:
  const time::milliseconds m_minRetxInterval;
};

} // namespace nfd::fw

#endif // NFD_DAEMON_FW_RETX_SUPPRESSION_FIXED_HPP

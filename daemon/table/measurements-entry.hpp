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

#ifndef NFD_DAEMON_TABLE_MEASUREMENTS_ENTRY_HPP
#define NFD_DAEMON_TABLE_MEASUREMENTS_ENTRY_HPP

#include "strategy-info-host.hpp"

#include <ndn-cxx/util/scheduler.hpp>

/*
 * Measurements Entry.hpp 解説
 *
 * 【概要】
 * Measurementsテーブルに格納されるエントリを表すクラス。
 * 各名前プレフィックスごとの通信状態や戦略情報を保持する。
 *
 * 【主な役割】
 * - 名前ごとの「Measurements情報（戦略が利用する統計や状態）」を保持
 * - StrategyInfoHostを継承しており、転送戦略が追加データを格納可能
 * - NameTree Entryと1対1で関連付けられる（m_nameTreeEntry）
 *
 * 【主要メンバ変数】
 * - m_name:
 *     対象のNDN Name（プレフィックス）
 *
 * - m_expiry:
 *     有効期限。古いMeasurements情報を削除するために使用
 *
 * - m_cleanup:
 *     削除処理用のスケジューライベントID
 *
 * - m_nameTreeEntry:
 *     対応するNameTree::Entry（内部リンクとして利用）
 *
 * 【設計意図】
 * - 名前付きデータ通信で得られた統計値等を保持し、Forwarding Strategyに提供
 * - NameTreeとの密接な関連によりエントリの探索効率を向上
 * - 自動的な期限処理（expiry）で不要なエントリを除去
 *
 * 【注意点】
 * - noncopyable：コピー不可 → 共有はポインタ参照が前提
 * - NameTreeとの対応付けは外部公開されず、内部管理のみ（friend指定）
 * - StrategyInfoHostの機能を利用して戦略固有情報を拡張できる
 */

namespace nfd::name_tree {
class Entry;
} // namespace nfd::name_tree

namespace nfd::measurements {

class Measurements;

/**
 * \brief Represents an entry in the %Measurements table.
 * \sa Measurements
 */
class Entry : public StrategyInfoHost, noncopyable
{
public:
  explicit
  Entry(const Name& name)
    : m_name(name)
  {
  }

  const Name&
  getName() const noexcept
  {
    return m_name;
  }

private:
  Name m_name;
  time::steady_clock::time_point m_expiry = time::steady_clock::time_point::min();
  ndn::scheduler::EventId m_cleanup;

  name_tree::Entry* m_nameTreeEntry = nullptr;

  friend ::nfd::name_tree::Entry;
  friend Measurements;
};

} // namespace nfd::measurements

#endif // NFD_DAEMON_TABLE_MEASUREMENTS_ENTRY_HPP

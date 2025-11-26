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

#ifndef NFD_DAEMON_TABLE_CS_HPP
#define NFD_DAEMON_TABLE_CS_HPP

#include "cs-policy.hpp"

/*
 * cs.hpp 解説
 *
 * このヘッダファイルは NFD (Named Data Networking Forwarding Daemon) における
 * Content Store (CS) を定義しています。
 * CS は Data パケットを一時的にキャッシュして、同じ Interest が来た場合に
 * 迅速に応答するために使用されます。
 *
 * クラス Cs の役割:
 * - Content Store の管理。
 * - Data パケットの挿入、検索、削除を行う。
 * - キャッシュの容量管理や置換ポリシーを提供。
 * - CS の機能の有効/無効 (admit / serve) を設定可能。
 *
 * 主要メンバ:
 * - Table m_table: データの格納コンテナ。std::set で Data 名でソート。
 * - unique_ptr<Policy> m_policy: キャッシュ置換ポリシー (LRU など)。
 * - signal::ScopedConnection m_beforeEvictConnection: エントリ削除時のシグナル管理。
 * - bool m_shouldAdmit: false の場合、新規 Data の追加を禁止。
 * - bool m_shouldServe: false の場合、CS 検索は常にミス扱い。
 *
 * 主要メソッド:
 * - insert(const Data&, bool): Data パケットを CS に挿入。
 * - erase(const Name&, size_t, Callback): 指定プレフィックスに一致する Data を非同期削除。
 * - find(const Interest&, HitCallback, MissCallback): Interest に一致する Data を検索し、
 *   ヒット/ミスコールバックを呼ぶ。
 * - size(): 現在の格納 Data 数を返す。
 *
 * CS 設定関連:
 * - getLimit() / setLimit(): CS の容量 (パケット数) を取得・設定。
 * - getPolicy() / setPolicy(): 置換ポリシーの取得・変更。変更前に CS は空である必要あり。
 * - shouldAdmit() / enableAdmit(): 新規 Data を受け入れるか設定。
 * - shouldServe() / enableServe(): CS から応答するか設定。
 *
 * イテレーション:
 * - begin() / end() で格納されている Data エントリを列挙可能。
 *
 * 内部処理:
 * - findPrefixRange(): 指定プレフィックスに一致するエントリ範囲を取得。
 * - eraseImpl(): 指定プレフィックスに一致するエントリを削除する内部処理。
 * - findImpl(): Interest に一致する Data エントリを検索する内部処理。
 * - setPolicyImpl(): ポリシー変更時の内部処理。
 *
 * 使用例:
 *   Cs cs(100); // 最大 100 パケット
 *   cs.insert(someData);
 *   cs.find(someInterest,
 *           [](const Interest&, const Data& data){  ヒット処理  },
 *           [](const Interest&){  ミス処理  });
 *   cs.erase(Name("/example"), 10, [](size_t n){  削除後処理  });
 */

namespace nfd {
namespace cs {

/** \brief Implements the Content Store.
 *
 *  This Content Store implementation consists of a Table and a replacement policy.
 *
 *  The Table is a container ( \c std::set ) sorted by full Names of stored Data packets.
 *  Data packets are wrapped in Entry objects. Each Entry contains the Data packet itself,
 *  and a few additional attributes such as when the Data becomes non-fresh.
 *
 *  The replacement policy is implemented in a subclass of \c Policy.
 */
class Cs : noncopyable
{
public:
  explicit
  Cs(size_t nMaxPackets = 10);

  /** \brief Inserts a Data packet.
   */
  void
  insert(const Data& data, bool isUnsolicited = false);

  /** \brief Asynchronously erases entries under \p prefix.
   *  \tparam AfterEraseCallback `void f(size_t nErased)`
   *  \param prefix name prefix of entries
   *  \param limit max number of entries to erase
   *  \param cb callback to receive the actual number of erased entries; must not be empty;
   *            it may be invoked either before or after erase() returns
   */
  template<typename AfterEraseCallback>
  void
  erase(const Name& prefix, size_t limit, AfterEraseCallback&& cb)
  {
    size_t nErased = eraseImpl(prefix, limit);
    cb(nErased);
  }

  /** \brief Finds the best matching Data packet.
   *  \tparam HitCallback `void f(const Interest&, const Data&)`
   *  \tparam MissCallback `void f(const Interest&)`
   *  \param interest the Interest for lookup
   *  \param hit a callback if a match is found; must not be empty
   *  \param miss a callback if there's no match; must not be empty
   *  \note A lookup invokes either callback exactly once.
   *        The callback may be invoked either before or after find() returns
   */
  template<typename HitCallback, typename MissCallback>
  void
  find(const Interest& interest, HitCallback&& hit, MissCallback&& miss) const
  {
    auto match = findImpl(interest);
    if (match == m_table.end()) {
      miss(interest);
      return;
    }
    hit(interest, match->getData());
  }

  /** \brief Get number of stored packets.
   */
  size_t
  size() const
  {
    return m_table.size();
  }

public: // configuration
  /** \brief Get capacity (in number of packets).
   */
  size_t
  getLimit() const noexcept
  {
    return m_policy->getLimit();
  }

  /** \brief Change capacity (in number of packets).
   */
  void
  setLimit(size_t nMaxPackets)
  {
    return m_policy->setLimit(nMaxPackets);
  }

  /** \brief Get replacement policy.
   */
  Policy*
  getPolicy() const noexcept
  {
    return m_policy.get();
  }

  /** \brief Change replacement policy.
   *  \pre size() == 0
   */
  void
  setPolicy(unique_ptr<Policy> policy);

  /** \brief Get CS_ENABLE_ADMIT flag.
   *  \sa https://redmine.named-data.net/projects/nfd/wiki/CsMgmt#Update-config
   */
  bool
  shouldAdmit() const noexcept
  {
    return m_shouldAdmit;
  }

  /** \brief Set CS_ENABLE_ADMIT flag.
   *  \sa https://redmine.named-data.net/projects/nfd/wiki/CsMgmt#Update-config
   */
  void
  enableAdmit(bool shouldAdmit) noexcept;

  /** \brief Get CS_ENABLE_SERVE flag.
   *  \sa https://redmine.named-data.net/projects/nfd/wiki/CsMgmt#Update-config
   */
  bool
  shouldServe() const noexcept
  {
    return m_shouldServe;
  }

  /** \brief Set CS_ENABLE_SERVE flag.
   *  \sa https://redmine.named-data.net/projects/nfd/wiki/CsMgmt#Update-config
   */
  void
  enableServe(bool shouldServe) noexcept;

public: // enumeration
  using const_iterator = Table::const_iterator;

  const_iterator
  begin() const
  {
    return m_table.begin();
  }

  const_iterator
  end() const
  {
    return m_table.end();
  }

private:
  std::pair<const_iterator, const_iterator>
  findPrefixRange(const Name& prefix) const;

  size_t
  eraseImpl(const Name& prefix, size_t limit);

  const_iterator
  findImpl(const Interest& interest) const;

  void
  setPolicyImpl(unique_ptr<Policy> policy);

private:
  Table m_table;
  unique_ptr<Policy> m_policy;
  signal::ScopedConnection m_beforeEvictConnection;

  bool m_shouldAdmit = true; ///< if false, no Data will be admitted
  bool m_shouldServe = true; ///< if false, all lookups will miss
};

} // namespace cs

using cs::Cs;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_CS_HPP

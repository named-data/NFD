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

#ifndef NFD_DAEMON_TABLE_PIT_HPP
#define NFD_DAEMON_TABLE_PIT_HPP

#include "name-tree.hpp"
#include "pit-entry.hpp"

/*
 * pit.hpp 解説
 *
 * このヘッダファイルは NFD (Named Data Networking Forwarding Daemon) における
 * Pending Interest Table (PIT) を定義しています。
 * PIT は、受信した Interest パケットの情報を保持し、対応する Data パケットが到着した際に
 * 転送先に返すために使われます。
 *
 * 型定義:
 * - DataMatchResult: Data パケットにマッチする PIT エントリをまとめた std::vector。
 *
 * クラス Iterator の役割:
 * - PIT 内の全エントリを列挙するためのイテレータ。
 * - NameTree 内の PIT エントリ群に対して順次アクセス可能。
 * - operator*() により現在の PIT エントリを参照。
 * - operator++() により次の PIT エントリに進む。
 *
 * クラス Pit の役割:
 * - NFD の Interest Table を管理する。
 * - Interest の登録、検索、削除、Data マッチングを提供。
 *
 * 主要メンバ:
 * - NameTree& m_nameTree: 名前ツリーへの参照。PIT エントリを階層的に管理。
 * - size_t m_nItems: 現在の PIT エントリ数。
 *
 * 主要メソッド:
 * - size(): 登録されている PIT エントリ数を返す。
 *
 * - find(const Interest& interest): 指定 Interest と完全一致する PIT エントリを返す。
 *   存在しない場合は nullptr。
 *
 * - insert(const Interest& interest): 指定 Interest の PIT エントリを挿入。
 *   既存エントリがあればそれを返し、新規なら true のフラグを返す。
 *
 * - findAllDataMatches(const Data& data): Data パケットにマッチするすべての PIT エントリを返す。
 *
 * - erase(Entry* entry): 指定 PIT エントリを削除。
 *
 * - deleteInOutRecords(Entry* entry, const Face& face): 指定 Face に関連する
 *   入力/出力レコードを削除。
 *
 * イテレーション:
 * - const_iterator = Iterator を使用。
 * - begin() / end() で PIT 内の全エントリを列挙可能。
 * - 列挙中に FIB/PIT/Measurements/StrategyChoice の変更があると未定義動作になる。
 *
 * 内部処理:
 * - erase(Entry*, bool): 内部で PIT エントリを削除する際に使用。
 * - findOrInsert(const Interest&, bool): Interest に対応する PIT エントリを検索・挿入。
 *   allowInsert フラグで新規挿入の可否を制御。
 *
 * 使用例:
 *   Pit pit(nameTree);
 *   auto [entry, isNew] = pit.insert(someInterest);
 *   auto matches = pit.findAllDataMatches(someData);
 *   pit.erase(entry.get());
 */

namespace nfd {
namespace pit {

/**
 * \brief An unordered iterable of all PIT entries matching a Data packet.
 */
using DataMatchResult = std::vector<shared_ptr<Entry>>;

/**
 * \brief PIT iterator.
 */
class Iterator : public boost::forward_iterator_helper<Iterator, const Entry>
{
public:
  /**
   * \brief Constructor.
   * \param ntIt a name tree iterator that visits name tree entries with one or more PIT entries
   * \param iPitEntry make this iterator to dereference to the i-th PIT entry in name tree entry
   */
  explicit
  Iterator(const NameTree::const_iterator& ntIt = {}, size_t iPitEntry = 0)
    : m_ntIt(ntIt)
    , m_iPitEntry(iPitEntry)
  {
  }

  const Entry&
  operator*() const noexcept
  {
    BOOST_ASSERT(m_ntIt != NameTree::const_iterator());
    BOOST_ASSERT(m_iPitEntry < m_ntIt->getPitEntries().size());
    return *m_ntIt->getPitEntries()[m_iPitEntry];
  }

  Iterator&
  operator++();

  friend bool
  operator==(const Iterator& lhs, const Iterator& rhs) noexcept
  {
    return lhs.m_ntIt == rhs.m_ntIt &&
           lhs.m_iPitEntry == rhs.m_iPitEntry;
  }

private:
  NameTree::const_iterator m_ntIt; ///< current name tree entry
  size_t m_iPitEntry; ///< current PIT entry within m_ntIt->getPitEntries()
};

/**
 * \brief NFD's Interest Table.
 */
class Pit : noncopyable
{
public:
  explicit
  Pit(NameTree& nameTree);

  /**
   * \brief Returns the number of entries.
   */
  size_t
  size() const
  {
    return m_nItems;
  }

  /** \brief Finds a PIT entry for \p interest
   *  \param interest the Interest
   *  \return an existing entry with same Name and Selectors; otherwise nullptr
   */
  shared_ptr<Entry>
  find(const Interest& interest) const
  {
    return const_cast<Pit*>(this)->findOrInsert(interest, false).first;
  }

  /** \brief Inserts a PIT entry for \p interest
   *  \param interest the Interest; must be created with make_shared
   *  \return a new or existing entry with same Name and Selectors,
   *          and true for new entry, false for existing entry
   */
  std::pair<shared_ptr<Entry>, bool>
  insert(const Interest& interest)
  {
    return this->findOrInsert(interest, true);
  }

  /** \brief Performs a Data match
   *  \return an iterable of all PIT entries matching \p data
   */
  DataMatchResult
  findAllDataMatches(const Data& data) const;

  /** \brief Deletes an entry
   */
  void
  erase(Entry* entry)
  {
    this->erase(entry, true);
  }

  /** \brief Deletes in-records and out-records for \p face
   */
  void
  deleteInOutRecords(Entry* entry, const Face& face);

public: // enumeration
  using const_iterator = Iterator;

  /** \return an iterator to the beginning
   *  \note Iteration order is implementation-defined.
   *  \warning Undefined behavior may occur if a FIB/PIT/Measurements/StrategyChoice entry
   *           is inserted or erased during iteration.
   */
  const_iterator
  begin() const;

  /** \return an iterator to the end
   *  \sa begin()
   */
  const_iterator
  end() const
  {
    return Iterator();
  }

private:
  void
  erase(Entry* pitEntry, bool canDeleteNte);

  /** \brief Finds or inserts a PIT entry for \p interest
   *  \param interest the Interest; must be created with make_shared if allowInsert
   *  \param allowInsert whether inserting a new entry is allowed
   *  \return if allowInsert, a new or existing entry with same Name+Selectors,
   *          and true for new entry, false for existing entry;
   *          if not allowInsert, an existing entry with same Name+Selectors and false,
   *          or `{nullptr, true}` if there's no existing entry
   */
  std::pair<shared_ptr<Entry>, bool>
  findOrInsert(const Interest& interest, bool allowInsert);

private:
  NameTree& m_nameTree;
  size_t m_nItems = 0;
};

} // namespace pit

using pit::Pit;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_PIT_HPP

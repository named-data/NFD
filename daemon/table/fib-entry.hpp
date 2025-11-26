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

#ifndef NFD_DAEMON_TABLE_FIB_ENTRY_HPP
#define NFD_DAEMON_TABLE_FIB_ENTRY_HPP

#include "core/common.hpp"

/*
 * FIB Entry.hpp 解説
 *
 * 【概要】
 * - FIB（Forwarding Information Base：転送情報ベース）の1エントリを表すクラス。
 *   NDN で名前（Prefix）に対して転送先（NextHop）の候補を保持する役割。
 *
 * 【構造】
 * ■ fib::Entry
 * - Prefix（Name）に対応
 * - NextHopList を保持（複数の転送候補）
 * - NameTree Entry へのリンク（階層管理）
 *
 * ■ NextHop クラス
 * - 転送先Face（Faceオブジェクト）へのポインタ
 * - 転送コスト（コスト最適化用）
 *
 * → Faceごとのレコード
 *
 * 【主な機能】
 * - getPrefix()  
 *    → この FIB entry の対象プレフィックスを返す
 *
 * - getNextHops(), hasNextHops()  
 *    →転送候補Faceの一覧取得、空判定
 *
 * - hasNextHop(face)  
 *    →指定されたFaceが既に登録されているかを確認
 *
 * - addOrUpdateNextHop(face, cost)  
 *    →次ホップ追加  
 *    →既存ならコスト更新  
 *
 * - removeNextHop(face)  
 *    →該当Faceを候補から削除
 *
 * - findNextHop(face)  
 *    →内部検索用（mutating iterator取得）
 *
 * - sortNextHops()  
 *    →コスト順などの整理
 *
 * 【内部設計ポイント】
 * - NextHopはFace&でなくポインタ管理（移動可能性の確保）
 * - Entryはnoncopyable  
 *   →複製を禁止し参照整合性を保つ
 * - FibとNameTree Entryが friend  
 *   →内部構造への直接アクセスを許可（効率性重視）
 *
 * 【役割のまとめ】
 * - このクラスは「名前空間に対する転送先集合」の最小単位
 *   例：/ndn/ucla → {Face#1(cost=10), Face#4(cost=15)}
 *
 * Forwarding Strategy はこの NextHopList を参照し、
 * もっとも適した Face に Interest を転送する。
 *
 * 【関連】
 * - Fib（FIB全体管理）
 * - NameTree（階層的名前空間の構造）
 *
 * 【活用例】
 * - Interest受信時のルート探索
 * - 複数インタフェース経由の冗長転送（マルチパス制御）
 */

namespace nfd {

namespace face {
class Face;
} // namespace face
using face::Face;

namespace name_tree {
class Entry;
} // namespace name_tree

namespace fib {

class Fib;

/**
 * \brief Represents a nexthop record in a FIB entry.
 */
class NextHop
{
public:
  explicit
  NextHop(Face& face) noexcept
    : m_face(&face)
  {
  }

  Face&
  getFace() const noexcept
  {
    return *m_face;
  }

  uint64_t
  getCost() const noexcept
  {
    return m_cost;
  }

  void
  setCost(uint64_t cost) noexcept
  {
    m_cost = cost;
  }

private:
  Face* m_face; // pointer instead of reference so that NextHop is movable
  uint64_t m_cost = 0;
};

/**
 * \brief A collection of nexthops in a FIB entry.
 */
using NextHopList = std::vector<NextHop>;

/**
 * \brief Represents an entry in the FIB.
 * \sa Fib
 */
class Entry : noncopyable
{
public:
  explicit
  Entry(const Name& prefix);

  const Name&
  getPrefix() const noexcept
  {
    return m_prefix;
  }

  const NextHopList&
  getNextHops() const noexcept
  {
    return m_nextHops;
  }

  /**
   * \brief Returns whether this Entry has any NextHop records.
   */
  bool
  hasNextHops() const noexcept
  {
    return !m_nextHops.empty();
  }

  /**
   * \brief Returns whether there is a NextHop record for \p face.
   */
  bool
  hasNextHop(const Face& face) const noexcept;

private:
  /** \brief Adds a NextHop record to the entry.
   *
   *  If a NextHop record for \p face already exists in the entry, its cost is set to \p cost.
   *
   *  \return the iterator to the new or updated NextHop and a bool indicating whether a new
   *  NextHop was inserted
   */
  std::pair<NextHopList::iterator, bool>
  addOrUpdateNextHop(Face& face, uint64_t cost);

  /** \brief Removes a NextHop record.
   *
   *  If no NextHop record for face exists, do nothing.
   */
  bool
  removeNextHop(const Face& face);

  /** \note This method is non-const because mutable iterators are needed by callers.
   */
  NextHopList::iterator
  findNextHop(const Face& face) noexcept;

  /** \brief Sorts the nexthop list.
   */
  void
  sortNextHops();

private:
  Name m_prefix;
  NextHopList m_nextHops;

  name_tree::Entry* m_nameTreeEntry = nullptr;

  friend ::nfd::name_tree::Entry;
  friend Fib;
};

} // namespace fib
} // namespace nfd

#endif // NFD_DAEMON_TABLE_FIB_ENTRY_HPP

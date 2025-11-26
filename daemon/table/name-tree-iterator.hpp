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

#ifndef NFD_DAEMON_TABLE_NAME_TREE_ITERATOR_HPP
#define NFD_DAEMON_TABLE_NAME_TREE_ITERATOR_HPP

#include "name-tree-hashtable.hpp"

#include <boost/operators.hpp>
#include <boost/range/iterator_range_core.hpp>

#include <functional>

/*
 * NameTree Iterator.hpp 解説
 *
 * 【概要】
 * NameTreeのイテレーション（列挙）機能を提供する。
 * NameTree内のエントリを順番に走査するためのクラスとヘルパー構造体を定義。
 * fullEnumerate, partialEnumerate, findAllMatchesなどで使用される。
 *
 * 【主な構成要素】
 * 1. EntrySelector / AnyEntry
 *    - エントリを受け入れるか拒否する述語関数
 *    - AnyEntryは全てのEntryを受け入れるデフォルト
 *
 * 2. EntrySubTreeSelector / AnyEntrySubTree
 *    - エントリおよび子ノードを受け入れるかどうかを返す述語
 *    - std::pair<bool,bool>で返却
 *       first = エントリを受け入れるか
 *       second = 子ノードを訪問するか
 *    - AnyEntrySubTreeは全てのEntryと子を受け入れる
 *
 * 3. Iteratorクラス
 *    - NameTreeの前方イテレータ
 *    - boost::forward_iterator_helperを継承
 *    - operator*()で現在のEntryを取得
 *    - operator++()で次のEntryに進む
 *    - m_impl: 列挙処理の実装を保持（nullptrならend）
 *    - m_entry: 現在のEntry
 *    - m_ref: 列挙実装用の参照Entry（部分列挙やプレフィックスマッチで使用）
 *    - m_state: 列挙処理状態
 *
 * 4. EnumerationImplクラス
 *    - 列挙処理の抽象基底クラス
 *    - advance(Iterator&)純粋仮想関数で次のEntryへ進める
 *    - nt, ht: 列挙対象のNameTreeとHashtable参照
 *
 * 5. 派生列挙実装
 *    - FullEnumerationImpl: 全Entryを列挙
 *    - PartialEnumerationImpl: 指定プレフィックス以下の部分列挙
 *    - PrefixMatchImpl: 最長プレフィックスマッチしたEntryからの列挙
 *
 * 6. Range
 *    - boost::iterator_range<Iterator>を用いた前方レンジ型
 *    - begin()/end()を持ち、range-based forで利用可能
 *
 * 【設計意図】
 * - NameTreeの全Entry、部分ツリー、プレフィックスマッチの列挙を統一的に扱う
 * - 述語関数による柔軟なフィルタリングが可能
 * - Iteratorは列挙中に安全にエントリを操作できる構造
 *
 * 【注意点】
 * - IteratorやEnumerationImplは列挙中にエントリが追加・削除されると
 *   挙動が未定義になる場合がある
 * - PartialEnumerationImplではm_stateのLSBが子ノード訪問フラグとして使用される
 */

namespace nfd::name_tree {

class NameTree;

/**
 * \brief A predicate to accept or reject an Entry in find operations.
 */
using EntrySelector = std::function<bool(const Entry&)>;

/**
 * \brief An #EntrySelector that accepts every Entry.
 */
struct AnyEntry
{
  constexpr bool
  operator()(const Entry&) const noexcept
  {
    return true;
  }
};

/** \brief A predicate to accept or reject an Entry and its children.
 *  \return `.first` indicates whether entry should be accepted;
 *          `.second` indicates whether entry's children should be visited
 */
using EntrySubTreeSelector = std::function<std::pair<bool, bool>(const Entry&)>;

/**
 * \brief An #EntrySubTreeSelector that accepts every Entry and its children.
 */
struct AnyEntrySubTree
{
  constexpr std::pair<bool, bool>
  operator()(const Entry&) const noexcept
  {
    return {true, true};
  }
};

class EnumerationImpl;

/**
 * \brief NameTree iterator.
 */
class Iterator : public boost::forward_iterator_helper<Iterator, const Entry>
{
public:
  Iterator();

  Iterator(shared_ptr<EnumerationImpl> impl, const Entry* ref);

  const Entry&
  operator*() const noexcept
  {
    BOOST_ASSERT(m_impl != nullptr);
    return *m_entry;
  }

  Iterator&
  operator++();

  friend bool
  operator==(const Iterator& lhs, const Iterator& rhs) noexcept
  {
    return lhs.m_entry == rhs.m_entry;
  }

private:
  /** \brief Enumeration implementation; nullptr for end iterator.
   */
  shared_ptr<EnumerationImpl> m_impl;

  /** \brief Current entry; nullptr for uninitialized iterator.
   */
  const Entry* m_entry = nullptr;

  /** \brief Reference entry used by enumeration implementation.
   */
  const Entry* m_ref = nullptr;

  /** \brief State used by enumeration implementation.
   */
  int m_state = 0;

  friend std::ostream& operator<<(std::ostream&, const Iterator&);
  friend class FullEnumerationImpl;
  friend class PartialEnumerationImpl;
  friend class PrefixMatchImpl;
};

std::ostream&
operator<<(std::ostream& os, const Iterator& i);

/**
 * \brief Enumeration operation implementation.
 */
class EnumerationImpl
{
public:
  explicit
  EnumerationImpl(const NameTree& nt);

  virtual void
  advance(Iterator& i) = 0;

protected:
  ~EnumerationImpl() = default;

protected:
  const NameTree& nt;
  const Hashtable& ht;
};

/**
 * \brief Full enumeration implementation.
 */
class FullEnumerationImpl final : public EnumerationImpl
{
public:
  FullEnumerationImpl(const NameTree& nt, const EntrySelector& pred);

  void
  advance(Iterator& i) final;

private:
  EntrySelector m_pred;
};

/**
 * \brief Partial enumeration implementation.
 *
 * Iterator::m_ref should be initialized to subtree root.
 * Iterator::m_state LSB indicates whether to visit children of m_entry.
 */
class PartialEnumerationImpl final : public EnumerationImpl
{
public:
  PartialEnumerationImpl(const NameTree& nt, const EntrySubTreeSelector& pred);

  void
  advance(Iterator& i) final;

private:
  EntrySubTreeSelector m_pred;
};

/**
 * \brief Partial enumeration implementation.
 *
 * Iterator::m_ref should be initialized to longest prefix matched entry.
 */
class PrefixMatchImpl final : public EnumerationImpl
{
public:
  PrefixMatchImpl(const NameTree& nt, const EntrySelector& pred);

private:
  void
  advance(Iterator& i) final;

private:
  EntrySelector m_pred;
};

/**
 * \brief A forward range of name tree entries.
 *
 * This type has `.begin()` and `.end()` methods which return Iterator.
 * This type is usable with range-based for loops.
 */
using Range = boost::iterator_range<Iterator>;

} // namespace nfd::name_tree

#endif // NFD_DAEMON_TABLE_NAME_TREE_ITERATOR_HPP

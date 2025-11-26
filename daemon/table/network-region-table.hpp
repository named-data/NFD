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

#ifndef NFD_DAEMON_TABLE_NETWORK_REGION_TABLE_HPP
#define NFD_DAEMON_TABLE_NETWORK_REGION_TABLE_HPP

#include "core/common.hpp"

#include <set>

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * NetworkRegionTable.hpp 解説
 * ------------------------------------------------------------
 * ■役割概要
 *   - Forwarding Hint を利用した転送判断を補助するテーブル
 *   - "Producer Region"（コンテンツ提供者領域）の情報を保持
 *   - Delegation（委譲名）が登録領域に一致したら
 *       → Producer側に到達したとみなして、本来のInterest Nameで転送
 *
 * ■使われる場面（超具体的）
 *   Interest が Linkオブジェクト（Forwarding Hint）を持つ場合：
 *
 *      ① まだ Producer Region に到達していない
 *         → Forwarding Hint を優先して転送（遠回り許容）
 *
 *      ② Producer Region に到達したと判定された
 *         → 本来の名前で転送（最終ルート探索へ切り替え）
 *
 *     → 「いつ本来のルーティングに切り替えるか」を判断する機能
 *
 * ■構造
 *   std::set<Name> を継承
 *     - insert(), clear(), find(), size() など利用可
 *
 * ■判定ロジック（isInProducerRegion）
 *   - forwardingHint の各 Delegation Name が
 *     NetworkRegionTable に登録された Region Name の prefix か？
 *   - 1つでも一致 → true（Producer Regionに到達）
 *   - 全て不一致   → false（Hintを使って転送継続）
 *
 * ■メリット
 *   - Forwarding Hint による効率的なルーティングをしつつ、
 *     最終接近時には無駄な迂回を防げる
 *
 * ------------------------------------------------------------
 * NDNにおける Forwarding Hint の賢い利用を支える
 * 小さいけどめちゃくちゃ重要な仕組み。
 * ------------------------------------------------------------
 */

/*
 * （以下、元コード）
 */

/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
 * ...
 */


namespace nfd {

/** \brief Stores a collection of producer region names.
 *
 *  This table is used in forwarding to process Interests with Link objects.
 *
 *  NetworkRegionTable exposes a set-like API, including methods `insert`, `clear`,
 *  `find`, `size`, `begin`, and `end`.
 */
class NetworkRegionTable : public std::set<Name>
{
public:
  /** \brief Determines whether an Interest has reached a producer region.
   *  \param forwardingHint forwarding hint of an Interest
   *  \retval true the Interest has reached a producer region
   *  \retval false the Interest has not reached a producer region
   *
   *  If any delegation name in the forwarding hint is a prefix of any region name,
   *  the Interest has reached the producer region and should be forwarded according to its Name;
   *  otherwise, the Interest should be forwarded according to the forwarding hint.
   */
  bool
  isInProducerRegion(span<const Name> forwardingHint) const;
};

} // namespace nfd

#endif // NFD_DAEMON_TABLE_NETWORK_REGION_TABLE_HPP

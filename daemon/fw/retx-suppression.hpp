/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FW_RETX_SUPPRESSION_HPP
#define NFD_DAEMON_FW_RETX_SUPPRESSION_HPP

/* -------------------------------------------------------------------------
 * RetxSuppression.hpp の解説
 *
 * ■ 概要
 *   NFD (Named Data Networking Forwarding Daemon) における
 *   Interest 再送の抑制結果を表す列挙型。
 *   再送 Interest をどう扱うかを戦略や転送ロジックで判定する際に使用。
 *
 * ■ 定義されている列挙値
 *
 * enum class RetxSuppressionResult
 *   1. NEW
 *      - この Interest は新しいもの (再送ではない)
 *      - 通常通り PIT に登録して転送する
 *
 *   2. FORWARD
 *      - この Interest は再送であるが、転送すべき
 *      - 再送であることを考慮しても、他の経路に転送する価値あり
 *
 *   3. SUPPRESS
 *      - この Interest は再送であり、転送を抑制すべき
 *      - PIT や FIB による重複転送を防ぐために無視
 *
 * ■ 使い方
 *   - Forwarding Strategy 内で、受信した Interest が再送かどうか判定し、
 *     この列挙値に基づいて転送または抑制を決定する
 *
 * ■ まとめ
 *   - 再送 Interest の扱いを明確に分類
 *   - 転送効率向上とループ抑制に貢献
 * ------------------------------------------------------------------------- */

namespace nfd::fw {

enum class RetxSuppressionResult {
  NEW,      ///< Interest is new (not a retransmission).
  FORWARD,  ///< Interest is a retransmission and should be forwarded.
  SUPPRESS, ///< Interest is a retransmission and should be suppressed.
};

} // namespace nfd::fw

#endif // NFD_DAEMON_FW_RETX_SUPPRESSION_HPP

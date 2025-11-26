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

#ifndef NFD_DAEMON_FACE_NULL_LINK_SERVICE_HPP
#define NFD_DAEMON_FACE_NULL_LINK_SERVICE_HPP

#include "link-service.hpp"


/*
 * NullLinkService.hpp
 *
 * 概要:
 *   NullLinkService は Named Data Networking Forwarding Daemon (NFD) における
 *   LinkService の派生クラスで、すべてのパケット送受信を無効化します。
 *
 * 主な用途:
 *   - 物理リンクを持たない Face のバックエンドとして使用
 *   - デバッグやテスト用に、ネットワーク送受信を伴わない Face を作成
 *
 * 動作の特徴:
 *   - Interest、Data、Nack の送信は何も行わない
 *   - 受信したパケットはすべて破棄
 *   - 実際のネットワーク通信を行わないため、リンク障害や遅延の影響を受けない
 *
 * 関連:
 *   - Face: NullLinkService は NullFace と組み合わせることで完全に「破棄専用 Face」となる
 *   - LinkService: NullLinkService は LinkService のインターフェースを実装するが、
 *     実際には何も処理しない
 *
 * 使用例:
 *   auto nullFace = makeNullFace();
 *   auto linkService = std::make_shared<NullLinkService>();
 *   // nullFace に linkService をセットすることで、完全に無効な Face が構築可能
 *
 * 注意点:
 *   - 実際のパケット通信は行われないため、アプリケーション層での動作確認用のみ
 *   - 運用ネットワークでは使用しない
 */

namespace nfd::face {

/** \brief A LinkService that drops every packet.
 */
class NullLinkService final : public LinkService
{
private:
  void
  doSendInterest(const Interest&) final
  {
  }

  void
  doSendData(const Data&) final
  {
  }

  void
  doSendNack(const lp::Nack&) final
  {
  }

  void
  doReceivePacket(const Block&, const EndpointId&) final
  {
  }
};

} // namespace nfd::face

#endif // NFD_DAEMON_FACE_NULL_LINK_SERVICE_HPP

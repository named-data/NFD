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

#ifndef NFD_DAEMON_FACE_NULL_TRANSPORT_HPP
#define NFD_DAEMON_FACE_NULL_TRANSPORT_HPP

#include "transport.hpp"

/*
 * NullTransport.hpp
 *
 * 概要:
 *   NullTransport は Named Data Networking Forwarding Daemon (NFD) における
 *   Transport の派生クラスで、すべてのパケット送信を無効化します。
 *
 * 主な用途:
 *   - 物理リンクや実際のネットワークを使用しない Face のバックエンドとして使用
 *   - デバッグやテスト用に、ネットワーク送受信を伴わない Transport を提供
 *
 * 動作の特徴:
 *   - doSend() メソッドは何もせず、送信パケットはすべて破棄
 *   - doClose() は TransportState を CLOSED に設定するのみ
 *   - 実際のネットワーク通信を行わないため、遅延や障害の影響を受けない
 *
 * 関連:
 *   - Face と組み合わせることで、完全に破棄専用の NullFace/NullTransport を構築可能
 *   - NullLinkService と同様に、テスト用やデバッグ用に利用
 *
 * 使用例:
 *   auto nullTransport = std::make_shared<NullTransport>();
 *   // この transport を Face にセットすれば、すべての送信が破棄される
 *
 * 注意点:
 *   - 実際のパケット送信は行わないため、運用ネットワークでの使用は不可
 *   - 完全に「破棄専用」の Transport として設計されている
 */

namespace nfd::face {

/** \brief A Transport that drops every packet.
 */
class NullTransport NFD_FINAL_UNLESS_WITH_TESTS : public Transport
{
public:
  explicit
  NullTransport(const FaceUri& localUri = FaceUri("null://"),
                const FaceUri& remoteUri = FaceUri("null://"),
                ndn::nfd::FaceScope scope = ndn::nfd::FACE_SCOPE_NON_LOCAL,
                ndn::nfd::FacePersistency persistency = ndn::nfd::FACE_PERSISTENCY_PERMANENT);

protected:
  void
  doClose() NFD_OVERRIDE_WITH_TESTS_ELSE_FINAL
  {
    setState(TransportState::CLOSED);
  }

private:
  void
  doSend(const Block&) NFD_OVERRIDE_WITH_TESTS_ELSE_FINAL
  {
  }
};

} // namespace nfd::face

#endif // NFD_DAEMON_FACE_NULL_TRANSPORT_HPP

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

#ifndef NFD_DAEMON_FACE_FACE_ENDPOINT_HPP
#define NFD_DAEMON_FACE_FACE_ENDPOINT_HPP

/*
 * 【FaceEndpoint クラスの概要】
 *
 * FaceEndpoint は、Forwarder 内でパケットの入出力に使用される
 * 「Face（通信インタフェース）＋通信エンドポイント」の組を表現するクラスである。
 *
 * Face とは？
 *   - アプリケーションやネットワークと接続する通信口
 *   - Interest / Data / Nack の送受信単位となる
 *
 * EndpointId とは？
 *   - Face 内のより細かい識別子
 *   - 同一 Face 上の複数の経路識別に利用される場合がある
 *
 * 【用途】
 *   - Forwarder のパイプライン（例：onIncomingInterest）で
 *     「どの Face のどのエンドポイントから受信したか」を明確に示す
 *
 *   - ログ出力やデバッグ時に、
 *     Face と EndpointId を組で表示可能にする（operator<< による出力）
 *
 * 【設計ポイント】
 *   - Face 参照を保持（所有権は持たない）
 *   - EndpointId は const として固定し不変性を保障
 *   - print() メソッドによりオブジェクトの可視化をサポート
 *
 * 【Forwarder との関係】
 *   - 受信処理では ingress（入力元）
 *   - 送信処理では egress（送信先）
 *   を表すために使われる
 *
 * 例：
 *   onIncomingInterest(const Interest&, const FaceEndpoint& ingress)
 *   → 「どこから届いたのか」が FaceEndpoint で明示される
 *
 * 【まとめ】
 * FaceEndpoint は、NDN ノード内のパケット伝達経路を
 * Face単位より細かく表現し、Forwarder の正確な入出力管理に寄与する。
 */

#include "face.hpp"

namespace nfd {

/**
 * \brief Represents a face-endpoint pair in the forwarder.
 * \sa face::Face, face::EndpointId
 */
class FaceEndpoint
{
public:
  explicit
  FaceEndpoint(Face& face, const EndpointId& endpoint = {});

private:
  void
  print(std::ostream& os) const;

  friend std::ostream&
  operator<<(std::ostream& os, const FaceEndpoint& fe)
  {
    fe.print(os);
    return os;
  }

public:
  Face& face;
  const EndpointId endpoint;
};

} // namespace nfd

#endif // NFD_DAEMON_FACE_FACE_ENDPOINT_HPP

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

#ifndef NFD_DAEMON_FACE_ETHERNET_CHANNEL_HPP
#define NFD_DAEMON_FACE_ETHERNET_CHANNEL_HPP

#include "channel.hpp"
#include "ethernet-protocol.hpp"
#include "pcap-helper.hpp"

#include <boost/asio/posix/stream_descriptor.hpp>
#include <ndn-cxx/net/network-interface.hpp>

#include <map>

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * NFD Ethernet Channel Header
 * ---------------------------------------------------------------------------
 * 本ファイルは、Named Data Networking Forwarding Daemon (NFD) における
 * Ethernetベースの Channel の実装を定義しています。
 *
 * 主な目的:
 *  - ネットワークインターフェース上で Ethernet フレームを受信・送信する Channel を提供
 *  - 新規リモートホストからのフレーム受信時に Face を生成
 *  - ユニキャスト向け Ethernet Face の接続や作成
 *  - 受信・送信、非同期 I/O、Face 管理、エラー処理を実装
 *
 * 提供されるクラス:
 *  - nfd::face::EthernetChannel : Channel 派生クラス
 *      - ローカルネットワークインターフェースにバインドして動作
 *      - listen() により着信フレームから Face を生成可能
 *      - connect() によりリモートエンドポイントへのユニキャスト Face を作成
 *
 * 注意:
 *  - 自動的に閉じられる on-demand Face は idleTimeout によって管理
 *  - 非同期読み取りで libpcap を利用
 *  - 受信フレームの解析に失敗した場合は Face を生成しない
 * ---------------------------------------------------------------------------
 */

namespace nfd::face {

/**
 * \brief Class implementing an Ethernet-based channel to create faces.
 */
class EthernetChannel final : public Channel
{
public:
  /**
   * \brief EthernetChannel-related error.
   */
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  /**
   * \brief Create an Ethernet channel on the given \p localEndpoint (network interface).
   *
   * To enable the creation of faces upon incoming connections, one needs to
   * explicitly call listen().
   */
  EthernetChannel(shared_ptr<const ndn::net::NetworkInterface> localEndpoint,
                  time::nanoseconds idleTimeout);

  bool
  isListening() const final
  {
    return m_isListening;
  }

  size_t
  size() const final
  {
    return m_channelFaces.size();
  }

  /**
   * \brief Create a unicast Ethernet face toward \p remoteEndpoint.
   */
  void
  connect(const ethernet::Address& remoteEndpoint,
          const FaceParams& params,
          const FaceCreatedCallback& onFaceCreated,
          const FaceCreationFailedCallback& onConnectFailed);

  /**
   * \brief Start listening.
   *
   * Enable listening on the local endpoint, waiting for incoming frames,
   * and creating a face when a frame is received from a new remote host.
   *
   * Faces created in this way will have on-demand persistency.
   *
   * \param onFaceCreated Callback to notify successful creation of a face
   * \param onFaceCreationFailed Callback to notify errors
   * \throw Error
   */
  void
  listen(const FaceCreatedCallback& onFaceCreated,
         const FaceCreationFailedCallback& onFaceCreationFailed);

private:
  void
  asyncRead(const FaceCreatedCallback& onFaceCreated,
            const FaceCreationFailedCallback& onReceiveFailed);

  void
  handleRead(const boost::system::error_code& error,
             const FaceCreatedCallback& onFaceCreated,
             const FaceCreationFailedCallback& onReceiveFailed);

  void
  processIncomingPacket(span<const uint8_t> packet,
                        const ethernet::Address& sender,
                        const FaceCreatedCallback& onFaceCreated,
                        const FaceCreationFailedCallback& onReceiveFailed);

  std::pair<bool, shared_ptr<Face>>
  createFace(const ethernet::Address& remoteEndpoint,
             const FaceParams& params);

  void
  updateFilter();

private:
  shared_ptr<const ndn::net::NetworkInterface> m_localEndpoint;
  bool m_isListening = false;
  boost::asio::posix::stream_descriptor m_socket;
  PcapHelper m_pcap;
  std::map<ethernet::Address, shared_ptr<Face>> m_channelFaces;
  const time::nanoseconds m_idleFaceTimeout; ///< Timeout for automatic closure of idle on-demand faces

#ifndef NDEBUG
  /// Number of frames dropped by the kernel, as reported by libpcap
  size_t m_nDropped = 0;
#endif
};

} // namespace nfd::face

#endif // NFD_DAEMON_FACE_ETHERNET_CHANNEL_HPP

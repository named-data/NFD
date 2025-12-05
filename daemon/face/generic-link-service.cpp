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

#include "generic-link-service.hpp"

#include <ndn-cxx/lp/fields.hpp>
#include <ndn-cxx/lp/pit-token.hpp>
#include <ndn-cxx/lp/tags.hpp>

#include <cmath>

/**
 * GenericLinkService は NFD のリンク層アダプテーションを担当するクラス。
 * 
 * 目的：
 *   - NDNLPv2 の機能（Fragmentation / Reliability / Congestion Marking / Local fields 等）
 *     を Transport と Network Layer（Interest/Data）との間で仲介する。
 *
 * 主な処理内容：
 *   - Interest / Data / Nack の送信処理（encode → fragment → reliability → send）
 *   - LpPacket の受信処理（reassemble → decode → 上位層へ通知）
 *   - Fragmentation（分割）と Reassembly（再構築）の管理
 *   - 信頼性機能（ACK/NACKベースの Loss Recovery）
 *   - 輻輳マーキング（CoDelベース）
 *   - Local Fields（IncomingFaceId / CongestionMark / PrefixAnnouncement 等）付加
 *
 * 内部構成：
 *   - m_fragmenter     : LpFragmenter クラス（パケットを MTU に合わせて分割）
 *   - m_reassembler    : LpReassembler クラス（分割されたパケットを再構築）
 *   - m_reliability    : LpReliability クラス（信頼性送信・再送制御）
 *   - Options          : 本サービスの設定一式
 *   - m_lastSeqNo      : LpPacket の Sequence 番号管理
 *
 * 大まかな動作の流れ：
 *   (送信側)
 *     doSendInterest / doSendData / doSendNack
 *        → encodeLpFields(タグ → LpHeader へ変換)
 *        → sendNetPacket（分割・シーケンス付与・信頼性制御）
 *        → sendLpPacket（最終的に wireEncode して Transport へ）
 *
 *   (受信側)
 *     doReceivePacket
 *        → 信頼性チェック（duplicate など）
 *        → fragment 判定 → Reassembler へ
 *        → decodeNetPacket（Interest/Data/Nack を識別して上位層へ通知）
 *
 * 各機能の詳細：
 *
 * ● encodeLpFields()
 *   - PacketBase に付与されているタグ（IncomingFaceIdTag / CongestionMarkTag 等）を
 *     LpPacket のヘッダフィールドへ変換し付与する。
 *
 * ● sendNetPacket()
 *   - MTU を考慮し fragmenter で複数の LpPacket に分割。
 *   - 信頼性機能が有効なら Sequence 番号を付与。
 *   - 各 fragment を sendLpPacket() に渡す。
 *
 * ● sendLpPacket()
 *   - Congestion Marking（CoDel）で輻輳判定し、必要なら CongestionMark を付加。
 *   - wireEncode → Transport 層の sendPacket() へ。
 *
 * ● doReceivePacket()
 *   - 受信 LpPacket を解析し、fragment があれば Reassembler へ。
 *   - 完成した network-layer packet は decodeNetPacket() へ。
 *
 * ● decodeInterest / decodeData / decodeNack
 *   - 再構築した netPkt を Interest や Data に decode。
 *   - 成功すれば NFD の上位（forwarder）側へ通知するシグナルを発火。
 *
 * ● checkCongestionLevel()
 *   - Transport の送信キュー長を見て、一定以上なら CongestionMark を付与（CoDel）。
 *
 *
 * このクラスの役割まとめ：
 *   - 「Transport から直接受け取る LpPacket」と
 *     「NFD の forwarder が扱う Interest/Data」との
 *     中間レイヤ（NDNLPv2）を完全に処理する中心的クラス。
 *
 */

 /**
 * ==== 受信処理系（receive path） ====
 *
 * doReceivePacket()
 *   - Transport から届いた raw LpPacket を受け取るエントリポイント。
 *   - 主な処理順序：
 *       1. ワイヤフォーマットの Block をパースし LpPacket に変換
 *       2. 信頼性チェック（ACK/NACK フラグの処理）
 *       3. Fragment 判定 → fragmenter へ渡す
 *       4. Reassember により network-layer packet が復元されたら decodeNetPacket() へ
 *
 *   - EndpointId:
 *       どの Transport / Face から届いたかを示す識別子。
 *
 *
 * decodeNetPacket()
 *   - Reassembler によって完成した netPkt (Block) を、Interest/Data/Nack に分岐して処理する。
 *   - firstPkt は最初の fragment の LpPacket（LocalFields はここに集約されている）
 *
 *   流れ：
 *      - TLV-TYPE 判定（Interest？Data？Nack？）
 *      - この種類に応じて decodeInterest / decodeData / decodeNack を呼ぶ
 *      - 成功すれば NFD forwarder へイベント通知（receiveInterest など）
 *      - 失敗した場合は logWarning
 *
 *
 * decodeInterest()
 *   - netPkt（Interest TLV）を Interest オブジェクトとして decode
 *   - Nack フィールドが firstPkt に含まれていないことを確認
 *   - 成功すれば receiveInterest シグナルを emit
 *   - LpHeader の parse error は tlv::Error を throw
 *
 *
 * decodeData()
 *   - Data パケットの解釈を担当
 *   - LocalFields（CongestionMark、IncomingFaceId など）もここで抽出され、
 *     上位の PIT / CS などに影響する
 *
 *
 * decodeNack()
 *   - Nack の decode を行う
 *   - Nack フィールドの種類で Reason（例：NO_ROUTE、CONGESTION 等）を読み取る
 *   - 受信した Interest の firstPkt の LocalFields と組み合わせて
 *     上位側に Nack イベントを通知する
 *
 *
 *
 * ==== Fragmentation / Reassembly ====
 *
 * m_fragmenter (LpFragmenter)
 *   - 送信時に MTU に合わせて TLV パケットを分割する
 *   - LpPacket に Fragment フィールドや Sequence 番号を付加する
 *   - 大きい Data パケット（署名付き等）は必ず複数 fragment になる
 *
 *
 * m_reassembler (LpReassembler)
 *   - 受信した fragment 群を組み立て、元の network-layer packet を復元する
 *   - 仕様：
 *        - fragment ID ごとのバッファリング
 *        - 欠損 fragment がある場合は一定時間待って捨てる
 *        - 完成したら Block として上位へ返す
 *
 *
 * ==== 信頼性制御（Loss Recovery） ====
 *
 * m_reliability (LpReliability)
 *   - NDNLPv2 の Reliability 機能を実装（TCP の ACK/NACK に近い）
 *   - 主な機能：
 *       - Sequence 番号（lp::Sequence）によるパケット識別
 *       - 送信済みフラグメントの再送制御
 *       - ACK/NACK の処理
 *   - 有効になっている場合、sendNetPacket() 内で SeqNo が自動付与される
 *
 *
 * ==== Sequence 番号管理 ====
 *
 * m_lastSeqNo
 *   - 送信に使った最後の Sequence 番号
 *   - static_cast<lp::Sequence>(-2) で初期化されている理由：
 *       → 初回使用時に -1 → 0 へ移行するように調整している
 *         （NDNLPv2 の仕様で Sequence=0 は特別扱いされるため）
 *
 *
 * ==== Options ====
 *
 * Options m_options;
 *   - サービス全体の設定集
 *   - 主な設定例：
 *       - isReliabilityEnabled
 *       - isFragmentationEnabled
 *       - mtu
 *       - allowCongestionMarking
 *       - allowIncomingFaceId
 *       - CoDel パラメータ（target / interval など）
 *
 *   - 実行時に Face(Transport) 側から渡される
 *
 *
 * ==== GenericLinkService の位置付け ====
 *
 *                ┌─────────────┐
 *                │   Forwarder  │
 *                │  (NFD core)  │
 *                └──────┬──────┘
 *                       ▲ decode
 *                       │ encode
 *                ┌──────┴──────┐
 *                │GenericLinkSvc│ ← ここ
 *                └──────┬──────┘
 *                       ▼
 *                ┌─────────────┐
 *                │  Transport   │
 *                │ (Ethernet,   │
 *                │   UDP, TCP)  │
 *                └─────────────┘
 *
 * - Forwarder（Interest/Data の処理本体）との間で
 *     「NDNLPv2 ヘッダ付け・分割・再構築・信頼性・LocalFields」
 *   を提供する役割。
 *
 * - Transport は単なる byte stream / packet stream なので
 *   network-layer packet をそのまま流すことはできない。
 *   → GenericLinkService が NDNLPv2 の規則に沿って適切に整形して送受信する。
 *
 *
 * ==== このクラスが扱う Local Fields の例 ====
 *
 * - IncomingFaceId
 * - CongestionMark
 * - HopLimit
 * - TxSequence / RxSequence
 * - PrefixAnnouncement（名前空間の告知）
 *
 * これらはすべて LpPacket のヘッダ部分に格納され、forwarder に渡る。
 *
 *
 * ==== まとめ ====
 *
 * GenericLinkService は Transport と Forwarder の間で NDNLPv2 を処理する中心クラスであり、
 *   - Fragmentation / Reassembly
 *   - Reliability
 *   - Local Fields（IncomingFaceId など）
 *   - Congestion Marking (CoDel)
 * を全て管理している。
 *
 * NDN のリンク層の仕様（RFC 8548 相当）を完全に実装する非常に重要なコンポーネント。
 *
 */

namespace nfd::face {

NFD_LOG_INIT(GenericLinkService);

constexpr size_t CONGESTION_MARK_SIZE = tlv::sizeOfVarNumber(lp::tlv::CongestionMark) + // type
                                        tlv::sizeOfVarNumber(sizeof(uint64_t)) +        // length
                                        tlv::sizeOfNonNegativeInteger(UINT64_MAX);      // value

GenericLinkService::GenericLinkService(const GenericLinkService::Options& options)
  : m_options(options)
  , m_fragmenter(m_options.fragmenterOptions, this)
  , m_reassembler(m_options.reassemblerOptions, this)
  , m_reliability(m_options.reliabilityOptions, this)
{
  m_reassembler.beforeTimeout.connect([this] (auto&&...) { ++nReassemblyTimeouts; });
  m_reliability.onDroppedInterest.connect([this] (const auto& i) { notifyDroppedInterest(i); });
  nReassembling.observe(&m_reassembler);
}

void
GenericLinkService::setOptions(const GenericLinkService::Options& options)
{
  m_options = options;
  m_fragmenter.setOptions(m_options.fragmenterOptions);
  m_reassembler.setOptions(m_options.reassemblerOptions);
  m_reliability.setOptions(m_options.reliabilityOptions);
}

ssize_t
GenericLinkService::getEffectiveMtu() const
{
  // Since MTU_UNLIMITED is negative, it will implicitly override any finite override MTU
  return std::min(m_options.overrideMtu, getTransport()->getMtu());
}

bool
GenericLinkService::canOverrideMtuTo(ssize_t mtu) const
{
  // Not allowed to override unlimited transport MTU
  if (getTransport()->getMtu() == MTU_UNLIMITED) {
    return false;
  }

  // Override MTU must be at least MIN_MTU (also implicitly forbids MTU_UNLIMITED and MTU_INVALID)
  return mtu >= MIN_MTU;
}

void
GenericLinkService::requestIdlePacket()
{
  // No need to request Acks to attach to this packet from LpReliability, as they are already
  // attached in sendLpPacket
  NFD_LOG_FACE_TRACE("IDLE packet requested");
  this->sendLpPacket({});
}

void
GenericLinkService::sendLpPacket(lp::Packet&& pkt)
{
  const ssize_t mtu = getEffectiveMtu();

  if (m_options.reliabilityOptions.isEnabled) {
    m_reliability.piggyback(pkt, mtu);
  }

  if (m_options.allowCongestionMarking) {
    checkCongestionLevel(pkt);
  }

  auto block = pkt.wireEncode();
  if (mtu != MTU_UNLIMITED && block.size() > static_cast<size_t>(mtu)) {
    ++nOutOverMtu;
    NFD_LOG_FACE_WARN("attempted to send packet over MTU limit");
    return;
  }
  this->sendPacket(block);
}

void
GenericLinkService::doSendInterest(const Interest& interest)
{
  lp::Packet lpPacket(interest.wireEncode());

  encodeLpFields(interest, lpPacket);

  this->sendNetPacket(std::move(lpPacket), true);
}

void
GenericLinkService::doSendData(const Data& data)
{
  lp::Packet lpPacket(data.wireEncode());

  encodeLpFields(data, lpPacket);

  this->sendNetPacket(std::move(lpPacket), false);
}

void
GenericLinkService::doSendNack(const lp::Nack& nack)
{
  lp::Packet lpPacket(nack.getInterest().wireEncode());
  lpPacket.add<lp::NackField>(nack.getHeader());

  encodeLpFields(nack, lpPacket);

  this->sendNetPacket(std::move(lpPacket), false);
}

void
GenericLinkService::assignSequences(std::vector<lp::Packet>& pkts)
{
  std::for_each(pkts.begin(), pkts.end(), [this] (lp::Packet& pkt) {
    pkt.set<lp::SequenceField>(++m_lastSeqNo);
  });
}
/**
 * ------------------------------------------------------------------------------------------------
 * //以下のメソッドでLPpacket要素の付加を行っている。　このタイミングで署名の付与をするべきではないだろうか
 * ------------------------------------------------------------------------------------------------
 */

void
GenericLinkService::encodeLpFields(const ndn::PacketBase& netPkt, lp::Packet& lpPacket)
{
  if (m_options.allowLocalFields) {
    auto incomingFaceIdTag = netPkt.getTag<lp::IncomingFaceIdTag>();
    if (incomingFaceIdTag != nullptr) {
      lpPacket.add<lp::IncomingFaceIdField>(*incomingFaceIdTag);
    }
  }

  auto congestionMarkTag = netPkt.getTag<lp::CongestionMarkTag>();
  if (congestionMarkTag != nullptr) {
    lpPacket.add<lp::CongestionMarkField>(*congestionMarkTag);
  }

  if (m_options.allowSelfLearning) {
    auto nonDiscoveryTag = netPkt.getTag<lp::NonDiscoveryTag>();
    if (nonDiscoveryTag != nullptr) {
      lpPacket.add<lp::NonDiscoveryField>(*nonDiscoveryTag);
    }

    auto prefixAnnouncementTag = netPkt.getTag<lp::PrefixAnnouncementTag>();
    if (prefixAnnouncementTag != nullptr) {
      lpPacket.add<lp::PrefixAnnouncementField>(*prefixAnnouncementTag);
    }
  }

  auto pitToken = netPkt.getTag<lp::PitToken>();
  if (pitToken != nullptr) {
    lpPacket.add<lp::PitTokenField>(*pitToken);
  }
}

void
GenericLinkService::sendNetPacket(lp::Packet&& pkt, bool isInterest)
{
  std::vector<lp::Packet> frags;
  ssize_t mtu = getEffectiveMtu();

  // Make space for feature fields in fragments
  if (m_options.reliabilityOptions.isEnabled && mtu != MTU_UNLIMITED) {
    mtu -= LpReliability::RESERVED_HEADER_SPACE;
  }

  if (m_options.allowCongestionMarking && mtu != MTU_UNLIMITED) {
    mtu -= CONGESTION_MARK_SIZE;
  }

  // An MTU of 0 is allowed but will cause all packets to be dropped before transmission
  BOOST_ASSERT(mtu == MTU_UNLIMITED || mtu >= 0);

  if (m_options.allowFragmentation && mtu != MTU_UNLIMITED) {
    bool isOk = false;
    std::tie(isOk, frags) = m_fragmenter.fragmentPacket(pkt, mtu);
    if (!isOk) {
      // fragmentation failed (warning is logged by LpFragmenter)
      ++nFragmentationErrors;
      return;
    }
  }
  else {
    if (m_options.reliabilityOptions.isEnabled) {
      frags.push_back(pkt);
    }
    else {
      frags.push_back(std::move(pkt));
    }
  }

  if (frags.size() == 1) {
    // even if indexed fragmentation is enabled, the fragmenter should not
    // fragment the packet if it can fit in MTU
    BOOST_ASSERT(!frags.front().has<lp::FragIndexField>());
    BOOST_ASSERT(!frags.front().has<lp::FragCountField>());
  }

  // Only assign sequences to fragments if reliability enabled or if packet contains >1 fragment
  if (m_options.reliabilityOptions.isEnabled || frags.size() > 1) {
    // Assign sequences to all fragments
    this->assignSequences(frags);
  }

  if (m_options.reliabilityOptions.isEnabled && frags.front().has<lp::FragmentField>()) {
    m_reliability.handleOutgoing(frags, std::move(pkt), isInterest);
  }

  for (lp::Packet& frag : frags) {
    this->sendLpPacket(std::move(frag));
  }
}

void
GenericLinkService::checkCongestionLevel(lp::Packet& pkt)
{
  ssize_t sendQueueLength = getTransport()->getSendQueueLength();
  // The transport must support retrieving the current send queue length
  if (sendQueueLength < 0) {
    return;
  }

  if (sendQueueLength > 0) {
    NFD_LOG_FACE_TRACE("txqlen=" << sendQueueLength << " threshold=" <<
                       m_options.defaultCongestionThreshold << " capacity=" <<
                       getTransport()->getSendQueueCapacity());
  }

  // sendQueue is above target
  if (static_cast<size_t>(sendQueueLength) > m_options.defaultCongestionThreshold) {
    const auto now = time::steady_clock::now();

    if (m_nextMarkTime == time::steady_clock::time_point::max()) {
      m_nextMarkTime = now + m_options.baseCongestionMarkingInterval;
    }
    // Mark packet if sendQueue stays above target for one interval
    else if (now >= m_nextMarkTime) {
      pkt.set<lp::CongestionMarkField>(1);
      ++nCongestionMarked;
      NFD_LOG_FACE_DEBUG("LpPacket was marked as congested");

      ++m_nMarkedSinceInMarkingState;
      // Decrease the marking interval by the inverse of the square root of the number of packets
      // marked in this incident of congestion
      time::nanoseconds interval(static_cast<time::nanoseconds::rep>(
                                   m_options.baseCongestionMarkingInterval.count() /
                                   std::sqrt(m_nMarkedSinceInMarkingState + 1)));
      m_nextMarkTime += interval;
    }
  }
  else if (m_nextMarkTime != time::steady_clock::time_point::max()) {
    // Congestion incident has ended, so reset
    NFD_LOG_FACE_DEBUG("Send queue length dropped below congestion threshold");
    m_nextMarkTime = time::steady_clock::time_point::max();
    m_nMarkedSinceInMarkingState = 0;
  }
}

void
GenericLinkService::doReceivePacket(const Block& packet, const EndpointId& endpoint)
{
  try {
    lp::Packet pkt(packet);

    if (m_options.reliabilityOptions.isEnabled) {
      if (!m_reliability.processIncomingPacket(pkt)) {
        NFD_LOG_FACE_TRACE("received duplicate fragment: DROP");
        ++nDuplicateSequence;
        return;
      }
    }

    if (!pkt.has<lp::FragmentField>()) {
      NFD_LOG_FACE_TRACE("received IDLE packet: DROP");
      return;
    }

    if ((pkt.has<lp::FragIndexField>() || pkt.has<lp::FragCountField>()) &&
        !m_options.allowReassembly) {
      NFD_LOG_FACE_WARN("received fragment, but reassembly disabled: DROP");
      return;
    }

    auto [isReassembled, netPkt, firstPkt] = m_reassembler.receiveFragment(endpoint, pkt);
    if (isReassembled) {
      this->decodeNetPacket(netPkt, firstPkt, endpoint);
    }
  }
  catch (const tlv::Error& e) {
    ++nInLpInvalid;
    NFD_LOG_FACE_WARN("packet parse error (" << e.what() << "): DROP");
  }
}

void
GenericLinkService::decodeNetPacket(const Block& netPkt, const lp::Packet& firstPkt,
                                    const EndpointId& endpointId)
{
  /**-----------------------------------------------------------------
   * ここに署名検証処理を加えるか。
   *-----------------------------------------------------------------
   */
  

  try {
    switch (netPkt.type()) {
      case tlv::Interest:
        if (firstPkt.has<lp::NackField>()) {
          this->decodeNack(netPkt, firstPkt, endpointId);
        }
        else {
          this->decodeInterest(netPkt, firstPkt, endpointId);
        }
        break;
      case tlv::Data:
        this->decodeData(netPkt, firstPkt, endpointId);
        break;
      default:
        ++nInNetInvalid;
        NFD_LOG_FACE_WARN("unrecognized network-layer packet TLV-TYPE " << netPkt.type() << ": DROP");
        return;
    }
  }
  catch (const tlv::Error& e) {
    ++nInNetInvalid;
    NFD_LOG_FACE_WARN("packet parse error (" << e.what() << "): DROP");
  }
}

void
GenericLinkService::decodeInterest(const Block& netPkt, const lp::Packet& firstPkt,
                                   const EndpointId& endpointId)
{
  BOOST_ASSERT(netPkt.type() == tlv::Interest);
  BOOST_ASSERT(!firstPkt.has<lp::NackField>());

  // forwarding expects Interest to be created with make_shared
  auto interest = make_shared<Interest>(netPkt);

  if (firstPkt.has<lp::NextHopFaceIdField>()) {
    if (m_options.allowLocalFields) {
      interest->setTag(make_shared<lp::NextHopFaceIdTag>(firstPkt.get<lp::NextHopFaceIdField>()));
    }
    else {
      NFD_LOG_FACE_WARN("received NextHopFaceId, but local fields disabled: DROP");
      return;
    }
  }

  if (firstPkt.has<lp::CachePolicyField>()) {
    ++nInNetInvalid;
    NFD_LOG_FACE_WARN("received CachePolicy with Interest: DROP");
    return;
  }

  if (firstPkt.has<lp::IncomingFaceIdField>()) {
    NFD_LOG_FACE_WARN("received IncomingFaceId: IGNORE");
  }

  if (firstPkt.has<lp::CongestionMarkField>()) {
    interest->setTag(make_shared<lp::CongestionMarkTag>(firstPkt.get<lp::CongestionMarkField>()));
  }

  if (firstPkt.has<lp::NonDiscoveryField>()) {
    if (m_options.allowSelfLearning) {
      interest->setTag(make_shared<lp::NonDiscoveryTag>(firstPkt.get<lp::NonDiscoveryField>()));
    }
    else {
      NFD_LOG_FACE_WARN("received NonDiscovery, but self-learning disabled: IGNORE");
    }
  }

  if (firstPkt.has<lp::PrefixAnnouncementField>()) {
    ++nInNetInvalid;
    NFD_LOG_FACE_WARN("received PrefixAnnouncement with Interest: DROP");
    return;
  }

  if (firstPkt.has<lp::PitTokenField>()) {
    interest->setTag(make_shared<lp::PitToken>(firstPkt.get<lp::PitTokenField>()));
  }

  this->receiveInterest(*interest, endpointId);
}

void
GenericLinkService::decodeData(const Block& netPkt, const lp::Packet& firstPkt,
                               const EndpointId& endpointId)
{
  BOOST_ASSERT(netPkt.type() == tlv::Data);

  // forwarding expects Data to be created with make_shared
  auto data = make_shared<Data>(netPkt);

  if (firstPkt.has<lp::NackField>()) {
    ++nInNetInvalid;
    NFD_LOG_FACE_WARN("received Nack with Data: DROP");
    return;
  }

  if (firstPkt.has<lp::NextHopFaceIdField>()) {
    ++nInNetInvalid;
    NFD_LOG_FACE_WARN("received NextHopFaceId with Data: DROP");
    return;
  }

  if (firstPkt.has<lp::CachePolicyField>()) {
    // CachePolicy is unprivileged and does not require allowLocalFields option.
    // In case of an invalid CachePolicyType, get<lp::CachePolicyField> will throw,
    // so it's unnecessary to check here.
    data->setTag(make_shared<lp::CachePolicyTag>(firstPkt.get<lp::CachePolicyField>()));
  }

  if (firstPkt.has<lp::IncomingFaceIdField>()) {
    NFD_LOG_FACE_WARN("received IncomingFaceId: IGNORE");
  }

  if (firstPkt.has<lp::CongestionMarkField>()) {
    data->setTag(make_shared<lp::CongestionMarkTag>(firstPkt.get<lp::CongestionMarkField>()));
  }

  if (firstPkt.has<lp::NonDiscoveryField>()) {
    ++nInNetInvalid;
    NFD_LOG_FACE_WARN("received NonDiscovery with Data: DROP");
    return;
  }

  if (firstPkt.has<lp::PrefixAnnouncementField>()) {
    if (m_options.allowSelfLearning) {
      data->setTag(make_shared<lp::PrefixAnnouncementTag>(firstPkt.get<lp::PrefixAnnouncementField>()));
    }
    else {
      NFD_LOG_FACE_WARN("received PrefixAnnouncement, but self-learning disabled: IGNORE");
    }
  }

  this->receiveData(*data, endpointId);
}

void
GenericLinkService::decodeNack(const Block& netPkt, const lp::Packet& firstPkt,
                               const EndpointId& endpointId)
{
  BOOST_ASSERT(netPkt.type() == tlv::Interest);
  BOOST_ASSERT(firstPkt.has<lp::NackField>());

  lp::Nack nack((Interest(netPkt)));
  nack.setHeader(firstPkt.get<lp::NackField>());

  if (firstPkt.has<lp::NextHopFaceIdField>()) {
    ++nInNetInvalid;
    NFD_LOG_FACE_WARN("received NextHopFaceId with Nack: DROP");
    return;
  }

  if (firstPkt.has<lp::CachePolicyField>()) {
    ++nInNetInvalid;
    NFD_LOG_FACE_WARN("received CachePolicy with Nack: DROP");
    return;
  }

  if (firstPkt.has<lp::IncomingFaceIdField>()) {
    NFD_LOG_FACE_WARN("received IncomingFaceId: IGNORE");
  }

  if (firstPkt.has<lp::CongestionMarkField>()) {
    nack.setTag(make_shared<lp::CongestionMarkTag>(firstPkt.get<lp::CongestionMarkField>()));
  }

  if (firstPkt.has<lp::NonDiscoveryField>()) {
    ++nInNetInvalid;
    NFD_LOG_FACE_WARN("received NonDiscovery with Nack: DROP");
    return;
  }

  if (firstPkt.has<lp::PrefixAnnouncementField>()) {
    ++nInNetInvalid;
    NFD_LOG_FACE_WARN("received PrefixAnnouncement with Nack: DROP");
    return;
  }

  this->receiveNack(nack, endpointId);
}

} // namespace nfd::face

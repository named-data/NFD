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

#ifndef NFD_DAEMON_FACE_ETHERNET_PROTOCOL_HPP
#define NFD_DAEMON_FACE_ETHERNET_PROTOCOL_HPP

#include "core/common.hpp"

#include <ndn-cxx/net/ethernet.hpp>
#include <net/ethernet.h>

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * NFD Ethernet Protocol Header
 * ---------------------------------------------------------------------------
 * 本ファイルは、Named Data Networking Forwarding Daemon (NFD) における
 * Ethernet フレーム処理関連の定義を提供します。
 *
 * 主な目的:
 *  - Ethernet フレームヘッダのチェック
 *  - ローカルおよび宛先アドレスに基づくフレームの検証
 *
 * 提供される関数:
 *  - nfd::ethernet::checkFrameHeader(span<const uint8_t> packet,
 *                                     const Address& localAddr,
 *                                     const Address& destAddr)
 *      - Ethernet フレームのヘッダを解析
 *      - 正常な場合は ether_header とペイロードを返す
 *
 * 注意:
 *  - ndn::ethernet 名前空間を利用
 *  - Linux の net/ethernet.h の構造体を使用
 * ---------------------------------------------------------------------------
 */

namespace nfd::ethernet {

using namespace ndn::ethernet;

std::tuple<const ether_header*, std::string>
checkFrameHeader(span<const uint8_t> packet, const Address& localAddr, const Address& destAddr);

} // namespace nfd::ethernet

#endif // NFD_DAEMON_FACE_ETHERNET_PROTOCOL_HPP

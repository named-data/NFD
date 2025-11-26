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

#ifndef NFD_DAEMON_FACE_NULL_FACE_HPP
#define NFD_DAEMON_FACE_NULL_FACE_HPP

#include "face.hpp"

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * NFD Null Face Header
 * ---------------------------------------------------------------------------
 * 本ファイルでは「Null Face」を提供します。
 * Null Face は物理的なトランスポートを持たず、
 * 送信されるすべてのパケットを破棄する特殊な Face です。
 *
 * 用途:
 * - デバッグやテスト
 * - 特定の Face の動作を無効化したい場合
 * ---------------------------------------------------------------------------

#ifndef NFD_DAEMON_FACE_NULL_FACE_HPP
#define NFD_DAEMON_FACE_NULL_FACE_HPP

#include "face.hpp"

namespace nfd::face {

 * \brief 物理トランスポートを持たず、全パケットを破棄する Face を生成
 * \param uri Face の URI（省略時は "null://"）
 * \return 生成された Face の shared_ptr
 
shared_ptr<Face>
makeNullFace(const FaceUri& uri = FaceUri("null://"));

} // namespace nfd::face

#endif // NFD_DAEMON_FACE_NULL_FACE_HPP
*/

namespace nfd::face {

/**
 * \brief Returns a Face that has no underlying transport and drops every packet.
 */
shared_ptr<Face>
makeNullFace(const FaceUri& uri = FaceUri("null://"));

} // namespace nfd::face

#endif // NFD_DAEMON_FACE_NULL_FACE_HPP

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2025,  Regents of the University of California,
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

#ifndef NFD_DAEMON_MGMT_FACE_MANAGER_HPP
#define NFD_DAEMON_MGMT_FACE_MANAGER_HPP

#include "manager-base.hpp"
#include "face/face.hpp"
#include "face/face-system.hpp"

#include <map>

/*
 * FaceManager.hpp
 *
 * 概要:
 *   FaceManager は NFD (Named Data Networking Forwarding Daemon) における
 *   Face（通信エンドポイント）管理用のクラスです。
 *   NFD Management Protocol に準拠し、Face の作成・更新・削除や状態通知、情報取得を行います。
 *
 * 主な役割:
 *   - Face の作成、更新、削除 (管理コマンドの処理)
 *   - Face およびチャネルの一覧提供 (StatusDataset)
 *   - Face の状態変化やイベントの通知 (NotificationStream)
 *
 * 特徴:
 *   - createFace(), updateFace(), destroyFace() で Face の制御操作を実行
 *   - listFaces(), listChannels(), queryFaces() で Face 情報やチャネル情報を提供
 *   - notifyFaceEvent() で Face イベント（追加・削除・状態変化）を通知
 *   - FaceSystem と FaceTable の参照を利用して Face の管理を行う
 *   - Face ごとの状態変化シグナルを管理し、必要に応じて通知を発行
 *
 * 内部構造:
 *   - m_faceSystem: 管理対象の FaceSystem への参照
 *   - m_faceTable: Face の登録テーブルへの参照
 *   - m_postNotification: NFD 管理プロトコルで通知を送信するためのオブジェクト
 *   - m_faceAddConn / m_faceRemoveConn: Face の追加・削除イベントのシグナル接続
 *   - m_faceStateChangeConn: Face ごとの状態変化シグナル接続を保持
 *
 * 参考:
 *   - NFD Management Protocol Wiki: https://redmine.named-data.net/projects/nfd/wiki/FaceMgmt
 *
 * 注意点:
 *   - Face 作成後の成功ハンドリングは afterCreateFaceSuccess() で行う
 *   - Face の状態変化やイベント通知は signal::ScopedConnection で接続管理
 */

namespace nfd {

/**
 * @brief Implements the Face Management of NFD Management Protocol.
 * @sa https://redmine.named-data.net/projects/nfd/wiki/FaceMgmt
 */
class FaceManager final : public ManagerBase
{
public:
  FaceManager(FaceSystem& faceSystem,
              Dispatcher& dispatcher, CommandAuthenticator& authenticator);

private: // ControlCommand
  void
  createFace(const ControlParameters& parameters,
             const CommandContinuation& done);

  void
  updateFace(const Interest& interest,
             const ControlParameters& parameters,
             const CommandContinuation& done);

  void
  destroyFace(const ControlParameters& parameters,
              const CommandContinuation& done);

private: // helpers for ControlCommand
  void
  afterCreateFaceSuccess(const shared_ptr<Face>& face,
                         const ControlParameters& parameters,
                         const CommandContinuation& done);

private: // StatusDataset
  void
  listFaces(ndn::mgmt::StatusDatasetContext& context);

  void
  listChannels(ndn::mgmt::StatusDatasetContext& context);

  void
  queryFaces(const Interest& interest, ndn::mgmt::StatusDatasetContext& context);

private: // NotificationStream
  void
  notifyFaceEvent(const Face& face, ndn::nfd::FaceEventKind kind);

  void
  connectFaceStateChangeSignal(const Face& face);

private:
  FaceSystem& m_faceSystem;
  FaceTable& m_faceTable;
  ndn::mgmt::PostNotification m_postNotification;
  signal::ScopedConnection m_faceAddConn;
  signal::ScopedConnection m_faceRemoveConn;

NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  std::map<FaceId, signal::ScopedConnection> m_faceStateChangeConn;
};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_FACE_MANAGER_HPP

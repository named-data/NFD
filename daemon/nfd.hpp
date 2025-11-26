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

#ifndef NFD_DAEMON_NFD_HPP
#define NFD_DAEMON_NFD_HPP

#include "common/config-file.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/mgmt/dispatcher.hpp>
#include <ndn-cxx/net/network-monitor.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

/* -------------------------------------------------------------------------
 * nfd.hpp の解説（NFD クラス定義）
 *
 * ■ 役割
 *   - NFD (Named Data Networking Forwarding Daemon) の中心クラス
 *   - NFD デーモンの各コンポーネント（FaceTable, FaceSystem, Forwarder 等）の生成と管理
 *   - 設定ファイルの読み込み・再読み込み機能を提供
 *   - 管理コマンドディスパッチャや各種 Manager (FaceManager, FibManager, CsManager, StrategyChoiceManager) を初期化
 *   - 内部 Face や NetworkMonitor を利用して NFD の動作を統括
 *
 * ■ 主なコンストラクタ
 *   - Nfd(const std::string& configFile, KeyChain& keyChain)
 *       ・設定ファイルパスを用いた初期化
 *   - Nfd(const ConfigSection& config, KeyChain& keyChain)
 *       ・解析済み ConfigSection を使った初期化
 *       ・NS-3 や Android など組み込み環境向け
 *   - Nfd(KeyChain& keyChain)
 *       ・内部用コンストラクタ
 *
 * ■ 主なメソッド
 *   - initialize()
 *       ・FaceTable、FaceSystem、Forwarder、各種 Manager を生成
 *       ・ログ設定、ネットワーク監視、Face 初期化
 *   - reloadConfigFile()
 *       ・設定ファイル全体の再読み込み
 *   - reloadConfigFileFaceSection()
 *       ・FaceSystem 部分のみ再読み込み（マルチキャスト Face 再初期化用）
 *   - configureLogging()
 *       ・ログ設定ファイルの読み込みと初期化
 *   - initializeManagement()
 *       ・管理用 Dispatcher と Authenticator の生成
 *       ・各種 Manager の生成
 *       ・管理用 FIB エントリ追加
 *
 * ■ 主なメンバ変数
 *   - m_configFile: 設定ファイルパス
 *   - m_configSection: 解析済み設定
 *   - m_faceTable: Face のテーブル
 *   - m_faceSystem: Face の生成・管理システム
 *   - m_forwarder: Forwarder 本体
 *   - m_keyChain: NDN セキュリティ用 KeyChain
 *   - m_internalFace, m_internalClientFace: 内部管理用 Face
 *   - m_dispatcher: 管理コマンドディスパッチャ
 *   - m_authenticator: 管理コマンド認証器
 *   - m_forwarderStatusManager, m_faceManager, m_fibManager, m_csManager, m_strategyChoiceManager:
 *       各種管理マネージャ
 *   - m_netmon: ネットワーク監視オブジェクト
 *   - m_reloadConfigEvent: 設定再読み込みイベント
 *
 * ■ まとめ
 *   Nfd クラスは NFD の中心クラスとして、Forwarder や Face、管理マネージャを初期化・統括し、
 *   設定ファイル管理やネットワーク監視まで行う。NFD デーモンの「脳」とも言える存在。
 * ------------------------------------------------------------------------- */

namespace nfd {

class FaceTable;
class Forwarder;

class CommandAuthenticator;
class ForwarderStatusManager;
class FaceManager;
class FibManager;
class CsManager;
class StrategyChoiceManager;

namespace face {
class Face;
class FaceSystem;
} // namespace face

/**
 * \brief Class representing the NFD instance.
 *
 * This class is used to initialize all components of NFD.
 */
class Nfd : noncopyable
{
public:
  /**
   * \brief Create NFD instance using an absolute or relative path to a configuration file.
   */
  Nfd(const std::string& configFile, ndn::KeyChain& keyChain);

  /**
   * \brief Create NFD instance using a parsed ConfigSection.
   *
   * This version of the constructor is more appropriate for integrated environments,
   * such as NS-3 or Android.
   *
   * \note When using this version of the constructor, error messages will show
   *       "internal://nfd.conf" when referring to configuration errors.
   */
  Nfd(const ConfigSection& config, ndn::KeyChain& keyChain);

  /**
   * \brief Destructor.
   */
  ~Nfd();

  /**
   * \brief Perform initialization of NFD instance.
   *
   * After initialization, NFD can be started by invoking `getGlobalIoService().run()`.
   */
  void
  initialize();

  /**
   * \brief Reload configuration file and apply updates (if any).
   */
  void
  reloadConfigFile();

private:
  explicit
  Nfd(ndn::KeyChain& keyChain);

  void
  configureLogging();

  void
  initializeManagement();

  void
  reloadConfigFileFaceSection();

private:
  std::string m_configFile;
  ConfigSection m_configSection;

  unique_ptr<FaceTable> m_faceTable;
  unique_ptr<face::FaceSystem> m_faceSystem;
  unique_ptr<Forwarder> m_forwarder;

  ndn::KeyChain& m_keyChain;
  shared_ptr<face::Face> m_internalFace;
  shared_ptr<ndn::Face> m_internalClientFace;
  unique_ptr<ndn::mgmt::Dispatcher> m_dispatcher;
  shared_ptr<CommandAuthenticator> m_authenticator;
  unique_ptr<ForwarderStatusManager> m_forwarderStatusManager;
  unique_ptr<FaceManager> m_faceManager;
  unique_ptr<FibManager> m_fibManager;
  unique_ptr<CsManager> m_csManager;
  unique_ptr<StrategyChoiceManager> m_strategyChoiceManager;

  shared_ptr<ndn::net::NetworkMonitor> m_netmon;
  ndn::scheduler::ScopedEventId m_reloadConfigEvent;
};

} // namespace nfd

#endif // NFD_DAEMON_NFD_HPP

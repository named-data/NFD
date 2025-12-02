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

#ifndef NFD_DAEMON_RIB_SERVICE_HPP
#define NFD_DAEMON_RIB_SERVICE_HPP

#include "common/config-file.hpp"
#include "mgmt/rib-manager.hpp"
#include "rib/fib-updater.hpp"
#include "rib/rib.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/mgmt/dispatcher.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/transport/transport.hpp>
#include <ndn-cxx/util/scheduler.hpp>

/**
 * @brief NFD-RIB Service 全体の概要
 *
 * このヘッダは、Named Data Networking Forwarding Daemon (NFD) における
 * RIB (Routing Information Base) サービスの中核クラス `nfd::rib::Service`
 * を定義する。
 *
 * 【主な役割】
 * - RIB サービススレッドを初期化し、NFD 内で唯一の RIB インスタンスを提供する。
 * - RIB 構成ファイル（nfd.conf）の読み込み、構文チェック、適用を行う。
 * - KeyChain を用いた管理用通信や署名を扱う。
 * - ndn::Face を作成し、管理パケット（Mgmt）や RIB Manager の操作を受け付ける。
 * - FIB Updater と連携し、RIB での経路変更を FIB に反映する。
 * - Readvertise（経路再広告）モジュールの管理。
 *
 * 【クラス全体の構造】
 * - RIB サービスは Singleton として実装され、同時に 1 インスタンスのみ生成可能。
 * - コンストラクタは設定ファイルまたは ConfigSection から初期化できる。
 * - get() 関数により唯一の Service インスタンスへアクセス可能。
 * - RibManager, Dispatcher, FIB Updater、Face、Controller など
 *   RIB 管理に関わるすべてのコンポーネントを統合して保持する。
 *
 * 【処理の流れ】
 * 1. Service インスタンスの生成
 * 2. Config のチェック（checkConfig）
 * 3. Config の適用（applyConfig）
 * 4. Face や Controller の初期化
 * 5. RIB Manager が管理パケットを処理可能な状態になる
 *
 * 【主要メンバー】
 * - m_keyChain: 署名・認証を行う KeyChain
 * - m_face: 管理用の ndn::Face
 * - m_nfdController: NFD へ管理コマンドを送信する Controller
 * - m_rib: RIB データベース本体
 * - m_fibUpdater: RIB → FIB の同期処理を担当
 * - m_dispatcher: 管理パケットのディスパッチャ
 * - m_ribManager: RIB エントリの追加・削除を扱うマネージャ
 * - Readvertise 関係: 経路再広告機構
 *
 * このファイルは、NFD における RIB 機能全体の基盤を成し、
 * RIB の初期化・設定・管理の中心的役割を担う重要なクラスを提供する。
 */

namespace nfd::rib {

class Readvertise;

/**
 * \brief Initializes and executes the NFD-RIB service thread.
 *
 * Only one instance of this class can be created at any time.
 * After initialization, NFD-RIB instance can be started by running the global io_context.
 */
class Service : noncopyable
{
public:
  /**
   * \brief Create NFD-RIB service.
   * \param configFile absolute or relative path of configuration file
   * \param keyChain the KeyChain
   * \throw std::logic_error Instance of rib::Service has been already constructed
   * \throw std::logic_error Instance of rib::Service is not constructed on RIB thread
   */
  Service(const std::string& configFile, ndn::KeyChain& keyChain);

  /**
   * \brief Create NFD-RIB service.
   * \param configSection parsed configuration section
   * \param keyChain the KeyChain
   * \note This constructor overload is more appropriate for integrated environments,
   *       such as NS-3 or android. Error messages related to configuration file
   *       will use "internal://nfd.conf" as configuration filename.
   * \throw std::logic_error Instance of rib::Service has been already constructed
   * \throw std::logic_error Instance of rib::Service is not constructed on RIB thread
   */
  Service(const ConfigSection& configSection, ndn::KeyChain& keyChain);

  ~Service();

  /**
   * \brief Get a reference to the only instance of this class.
   * \throw std::logic_error No instance has been constructed
   * \throw std::logic_error This function is invoked on a thread other than the RIB thread
   */
  static Service&
  get();

  RibManager&
  getRibManager() noexcept
  {
    return m_ribManager;
  }

private:
  template<typename ConfigParseFunc>
  Service(ndn::KeyChain& keyChain, shared_ptr<ndn::Transport> localNfdTransport,
          const ConfigParseFunc& configParse);

  void
  processConfig(const ConfigSection& section, bool isDryRun, const std::string& filename);

  void
  checkConfig(const ConfigSection& section, const std::string& filename);

  void
  applyConfig(const ConfigSection& section, const std::string& filename);

private:
  static inline Service* s_instance = nullptr;

  ndn::KeyChain& m_keyChain;
  ndn::Face m_face;
  ndn::nfd::Controller m_nfdController;

  Rib m_rib;
  FibUpdater m_fibUpdater;
  unique_ptr<Readvertise> m_readvertiseNlsr;
  unique_ptr<Readvertise> m_readvertisePropagation;
  ndn::mgmt::Dispatcher m_dispatcher;
  RibManager m_ribManager;
};

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_SERVICE_HPP

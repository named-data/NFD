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

#ifndef NFD_DAEMON_MGMT_MANAGER_BASE_HPP
#define NFD_DAEMON_MGMT_MANAGER_BASE_HPP

#include "command-authenticator.hpp"

#include <ndn-cxx/mgmt/dispatcher.hpp>
#include <ndn-cxx/mgmt/nfd/control-command.hpp>
#include <ndn-cxx/mgmt/nfd/control-parameters.hpp>
#include <ndn-cxx/mgmt/nfd/control-response.hpp>

#include <functional>

/*
 * ManagerBase.hpp
 *
 * 概要:
 *   このファイルは、NFD (Named Data Networking Forwarding Daemon) の
 *   管理モジュール（FaceManager、FibManager、StrategyChoiceManager など）が
 *   共通して利用する基底クラスを提供します。
 *   Dispatcher との連携や、NFD Management Protocol における
 *   コマンド処理の統一的仕組みを担当します。
 *
 * 主な機能:
 *   - 管理コマンド (ControlCommand) ハンドラ登録機能
 *   - ステータス情報 (StatusDataset) の提供ハンドラ登録
 *   - 通知 (NotificationStream) の登録と送信機能
 *   - 利用者認証 (Authorization) の提供
 *   - コマンド実行者識別の補助関数 (extractSigner)
 *
 * 提供インタフェース:
 *   - registerCommandHandler():
 *        指定コマンドに対応する処理関数を Dispatcher に登録
 *   - registerStatusDatasetHandler():
 *        運用状態情報の取得を提供するデータセットハンドラを登録
 *   - registerNotificationStream():
 *        イベント通知機構のストリームを登録
 *   - makeAuthorization():
 *        認証処理作成（必要に応じて派生クラスで上書き）
 *
 * 特徴:
 *   - 共通機能を一元化することで、個別 Manager 実装の簡素化を実現
 *   - デストラクタは protected:
 *        → 多態的利用を禁止し、誤った delete を防ぐ
 *   - テンプレートによる型安全なコマンドパラメータ抽出
 *
 * 使用される場面:
 *   - Face 管理、FIB 管理、フォワーダ状態管理などの
 *     各 Manager クラスがこの基底クラスを継承し、
 *     NFD Management Protocol の一部を実装する。
 *
 * 注意:
 *   - makeAuthorization() を使用しないコンストラクタを使う場合、
 *     必ず派生クラス側で makeAuthorization() を override する必要がある。
 */

namespace nfd {

using ndn::mgmt::Dispatcher;
using ndn::mgmt::CommandContinuation;
using ndn::nfd::ControlParameters;
using ndn::nfd::ControlResponse;

/**
 * @brief A collection of common functions shared by all NFD managers,
 *        such as communicating with the dispatcher and command validator.
 */
class ManagerBase : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  const std::string&
  getModule() const
  {
    return m_module;
  }

protected:
  /**
   * @warning If you use this constructor, you MUST override makeAuthorization().
   */
  ManagerBase(std::string_view module, Dispatcher& dispatcher);

  ManagerBase(std::string_view module, Dispatcher& dispatcher,
              CommandAuthenticator& authenticator);

  // ManagerBase is not supposed to be used polymorphically, so we make the destructor
  // protected to prevent deletion of derived objects through a pointer to the base class,
  // which would be UB when the destructor is non-virtual.
  ~ManagerBase();

NFD_PUBLIC_WITH_TESTS_ELSE_PROTECTED: // registrations to the dispatcher
  template<typename Command>
  using ControlCommandHandler = std::function<void(const Name& prefix, const Interest& interest,
                                                   const typename Command::RequestParameters& parameters,
                                                   const CommandContinuation& done)>;

  template<typename Command>
  void
  registerCommandHandler(ControlCommandHandler<Command> handler)
  {
    auto handle = [h = std::move(handler)] (const auto& prefix, const auto& interest,
                                            const auto& params, const auto& done) {
      const auto& reqParams = static_cast<const typename Command::RequestParameters&>(params);
      h(prefix, interest, reqParams, done);
    };
    m_dispatcher.addControlCommand<Command>(makeAuthorization(Command::verb.toUri()),
                                            std::move(handle));
  }

  void
  registerStatusDatasetHandler(const std::string& verb,
                               const ndn::mgmt::StatusDatasetHandler& handler);

  ndn::mgmt::PostNotification
  registerNotificationStream(const std::string& verb);

NFD_PUBLIC_WITH_TESTS_ELSE_PROTECTED: // helpers
  /**
   * @brief Extracts the name from the %KeyLocator of a ControlCommand request.
   *
   * This is called after the signature has been validated.
   * Returns an empty string if %SignatureInfo or %KeyLocator are missing or malformed.
   */
  static std::string
  extractSigner(const Interest& interest);

NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /**
   * @brief Returns an authorization function for a specific management module and verb.
   */
  virtual ndn::mgmt::Authorization
  makeAuthorization(const std::string& verb);

  /**
   * @brief Generates the relative prefix for a handler by appending the verb name to the module name.
   *
   * @param verb the verb name
   * @return the generated relative prefix
   */
  PartialName
  makeRelPrefix(const std::string& verb) const
  {
    return PartialName(m_module).append(verb);
  }

private:
  std::string m_module;
  Dispatcher& m_dispatcher;
  CommandAuthenticator* m_authenticator = nullptr;
};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_MANAGER_BASE_HPP

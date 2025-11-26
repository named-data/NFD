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

#ifndef NFD_DAEMON_MGMT_STRATEGY_CHOICE_MANAGER_HPP
#define NFD_DAEMON_MGMT_STRATEGY_CHOICE_MANAGER_HPP

#include "manager-base.hpp"

/*
 * StrategyChoiceManager.hpp
 *
 * 概要:
 *   本ファイルは、NFD (Named Data Networking Forwarding Daemon) における
 *   Strategy Choice Management の機能を提供するクラスを定義する。
 *   StrategyChoiceManager は、NFD Management Protocol の `strategy-choice/*`
 *   コマンドを受け付け、フォワーディング戦略の設定・解除・一覧取得を行う。
 *
 * 主な役割:
 *   - 特定のプレフィックスに対する転送戦略の設定 (setStrategy)
 *   - 戦略設定の解除 (unsetStrategy)
 *   - 現在の戦略選択テーブル一覧取得 (listChoices)
 *
 * 対象となるデータ構造:
 *   - StrategyChoice (strategy_choice::StrategyChoice)
 *       プレフィックスごとに適用されるフォワーディング戦略を管理
 *
 * 管理プロトコル:
 *   - NFD Management Protocol を通じてコマンドを登録・実行
 *   - ManagerBase を継承し、権限認証 (CommandAuthenticator) により安全な操作を実現
 *
 * 使用例:
 *   戦略変更例: `/localhost/nfd/strategy-choice/set`
 *   戦略解除例: `/localhost/nfd/strategy-choice/unset`
 *   設定一覧取得: `/localhost/nfd/strategy-choice/list`
 *
 * 位置づけ:
 *   - NFD が提供する複数のフォワーディング戦略選択機能の管理コンポーネント
 *   - 名前空間単位で戦略を柔軟に切り替えることでネットワーク性能の最適化を支援
 *
 * 関連仕様:
 *   https://redmine.named-data.net/projects/nfd/wiki/StrategyChoice
 *
 * このクラスにより、管理者はプレフィックス単位で forwarding strategy を動的に変更し、
 * NDN ノードのフォワーディング挙動を柔軟に制御できる。
 */

namespace nfd {

namespace strategy_choice {
class StrategyChoice;
} // namespace strategy_choice

/**
 * @brief Implements the Strategy Choice Management of NFD Management Protocol.
 * @sa https://redmine.named-data.net/projects/nfd/wiki/StrategyChoice
 */
class StrategyChoiceManager final : public ManagerBase
{
public:
  StrategyChoiceManager(strategy_choice::StrategyChoice& table,
                        Dispatcher& dispatcher, CommandAuthenticator& authenticator);

private:
  void
  setStrategy(ControlParameters parameters, const CommandContinuation& done);

  void
  unsetStrategy(ControlParameters parameters, const CommandContinuation& done);

  void
  listChoices(ndn::mgmt::StatusDatasetContext& context);

private:
  strategy_choice::StrategyChoice& m_table;
};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_STRATEGY_CHOICE_MANAGER_HPP

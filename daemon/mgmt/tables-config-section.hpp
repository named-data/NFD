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

#ifndef NFD_DAEMON_MGMT_TABLES_CONFIG_SECTION_HPP
#define NFD_DAEMON_MGMT_TABLES_CONFIG_SECTION_HPP

#include "common/config-file.hpp"
#include "fw/forwarder.hpp"

/*
 * TablesConfigSection.hpp
 *
 * 概要:
 *   NFD (Named Data Networking Forwarding Daemon) における
 *   `tables` セクションの設定処理を担当するクラス。
 *   Forwarder が持つ各種内部テーブル (CS・StrategyChoice・NetworkRegion 等) の
 *   設定内容を外部コンフィグから適用する役割を担う。
 *
 * 対応する設定例:
 *   tables {
 *     cs_max_packets 65536            // Content Store (CS) の最大保持パケット数
 *     cs_policy lru                   // CS 置換ポリシー
 *     cs_unsolicited_policy drop-all  // unsolicited Data の扱い
 *
 *     strategy_choice {               // プレフィックス毎の転送戦略設定
 *       /               /localhost/nfd/strategy/best-route
 *       /localhost      /localhost/nfd/strategy/multicast
 *       /localhost/nfd  /localhost/nfd/strategy/best-route
 *       /ndn/broadcast  /localhost/nfd/strategy/multicast
 *     }
 *
 *     network_region {                // Forwarder が属するネットワーク領域設定
 *       /example/region1
 *       /example/region2
 *     }
 *   }
 *
 * 主な動作仕様:
 *   - 設定ファイル再読み込み時:
 *       * CS 設定は上書き適用（省略時はデフォルトを使用）
 *       * strategy_choice は追加のみ（既存設定は削除しない）
 *       * network_region は省略時は現状維持
 *
 *   - 初期設定および再読み込み後に ensureConfigured() を呼び出す必要あり
 *     → tables セクション未記述時でも適切なデフォルトを適用するため
 *
 * クラス構成:
 *   - Forwarder への参照を保持し、設定変更を直接適用する
 *   - 設定ファイル解析時には dry-run 判定をサポート
 *   - strategy choice / network region に対して専用メソッドで解析処理を実行
 *
 * 位置付け:
 *   NFD の Forwarder 構成要素における基盤設定モジュールであり、
 *   管理者が forwarding 動作・ストレージ動作を柔軟に構成できるようにする。
 */

namespace nfd {

/** \brief Handles the `tables` configuration file section.
 *
 *  This class recognizes a config section that looks like
 *  \code{.unparsed}
 *  tables
 *  {
 *    cs_max_packets 65536
 *    cs_policy lru
 *    cs_unsolicited_policy drop-all
 *
 *    strategy_choice
 *    {
 *      /               /localhost/nfd/strategy/best-route
 *      /localhost      /localhost/nfd/strategy/multicast
 *      /localhost/nfd  /localhost/nfd/strategy/best-route
 *      /ndn/broadcast  /localhost/nfd/strategy/multicast
 *    }
 *
 *    network_region
 *    {
 *      /example/region1
 *      /example/region2
 *    }
 *  }
 *  \endcode
 *
 *  During a configuration reload,
 *  \li cs_max_packets, cs_policy, and cs_unsolicited_policy are applied;
 *      defaults are used if an option is omitted.
 *  \li strategy_choice entries are inserted, but old entries are not deleted.
 *  \li network_region is applied; it's kept unchanged if the section is omitted.
 *
 *  It's necessary to call ensureConfigured() after initial configuration and
 *  configuration reload, so that the correct defaults are applied in case
 *  tables section is omitted.
 */
class TablesConfigSection : noncopyable
{
public:
  explicit
  TablesConfigSection(Forwarder& forwarder);

  void
  setConfigFile(ConfigFile& configFile);

  /**
   * \brief Apply default configuration, if tables section was omitted in configuration file.
   */
  void
  ensureConfigured();

private:
  void
  processConfig(const ConfigSection& section, bool isDryRun, const std::string& filename);

  void
  processStrategyChoiceSection(const ConfigSection& section, bool isDryRun);

  void
  processNetworkRegionSection(const ConfigSection& section, bool isDryRun);

private:
  Forwarder& m_forwarder;
  bool m_isConfigured;
};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_TABLES_CONFIG_SECTION_HPP

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

#ifndef NFD_DAEMON_MGMT_LOG_CONFIG_SECTION_HPP
#define NFD_DAEMON_MGMT_LOG_CONFIG_SECTION_HPP

#include "common/config-file.hpp"

/*
 * LogConfigSection.hpp
 *
 * 概要:
 *   このファイルは、NFD (Named Data Networking Forwarding Daemon) における
 *   ログ設定 (Logging Configuration) を担当する関数の宣言を提供します。
 *   NFD の設定ファイル (ConfigFile) に「log」セクションを適用するために
 *   使用されます。
 *
 * 主な役割:
 *   - ログ出力に関する設定項目を ConfigFile に登録する。
 *   - ロギングシステムの初期設定を可能にする。
 *
 * 提供機能:
 *   - setConfigFile():
 *       ConfigFile オブジェクトに対して、ログ設定セクションを追加する。
 *       ログレベルやログ出力先などの設定内容を関連付ける。
 *
 * 特徴:
 *   - 実際のログ処理機能は担当せず、
 *     設定ファイルとの紐付けのみを責務とする。
 *   - nfd::log 名前空間内で定義され、
 *     他の設定セクションと明確に分離されている。
 *
 * 使用タイミング:
 *   - NFD 起動時または設定ファイル再読み込み時に呼び出される。
 *
 * 注意:
 *   - 設定項目の詳細なバリデーションは ConfigFile に依存する。
 */

namespace nfd::log {

void
setConfigFile(ConfigFile& config);

} // namespace nfd::log

#endif // NFD_DAEMON_MGMT_LOG_CONFIG_SECTION_HPP

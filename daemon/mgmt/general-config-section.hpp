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

#ifndef NFD_DAEMON_MGMT_GENERAL_CONFIG_SECTION_HPP
#define NFD_DAEMON_MGMT_GENERAL_CONFIG_SECTION_HPP

#include "common/config-file.hpp"

/*
 * GeneralConfigSection.hpp
 *
 * 概要:
 *   このファイルは、NFD (Named Data Networking Forwarding Daemon) の
 *   一般設定 (General Configuration) を処理するための関数を宣言しています。
 *
 * 主な役割:
 *   - NFD の設定ファイル (ConfigFile) に対して
 *     「general」セクションに関する設定を適用する。
 *   - 管理プロトコルではなく、NFD のローカル動作に関わる
 *     基本構成パラメータを扱う。
 *
 * 提供機能:
 *   - setConfigFile():
 *       NFD 起動時または設定再読み込み時に、
 *       ConfigFile オブジェクトへ一般設定を登録する。
 *
 * 特徴:
 *   - 設定値の保存やデータ構造管理は行わず、
 *     ConfigFile に対応する設定項目を追加する責務のみを持つ。
 *   - nfd::general 名前空間に属し、
 *     他の設定セクションと分離して管理できる。
 *
 * 使用シーン:
 *   - NFD 起動時に ConfigFile を初期化する段階
 *   - 設定反映の統合管理部分から呼び出される
 *
 * 注意:
 *   - 設定内容のバリデーションは ConfigFile へ委譲される
 */


namespace nfd::general {

void
setConfigFile(ConfigFile& config);

} // namespace nfd::general

#endif // NFD_DAEMON_MGMT_GENERAL_CONFIG_SECTION_HPP

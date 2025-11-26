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

#ifndef NFD_DAEMON_COMMON_CONFIG_FILE_HPP
#define NFD_DAEMON_COMMON_CONFIG_FILE_HPP

#include "core/common.hpp"

#include <boost/property_tree/ptree.hpp>

#include <functional>
#include <map>

/*
 * 【ConfigFile クラスの概要】
 *
 * NFD の設定ファイル（例：/usr/local/etc/ndn/nfd.conf）を解析し、
 * Forwarder モジュールへ正しく設定を反映させる中核コンポーネント。
 *
 * 設定内容はセクション単位で分割され、それぞれに対応する
 * セクションハンドラ（ConfigSectionHandler）が呼び出される。
 *
 * →「Forwarder の設定処理を一元管理する司令塔」
 *
 * 【主な機能】
 *  - 設定ファイルの読み込み（ファイル / 文字列 / ストリーム）
 *  - セクションごとの処理関数への振り分け
 *  - 未知のセクションの扱い制御（エラー／無視）
 *  - 設定値の型チェックとエラーレポート
 *
 * 例：yes/no、整数、範囲検証
 *
 *   bool enable = ConfigFile::parseYesNo(sec, "enable", "face_system");
 *   int size = ConfigFile::parseNumber<int>(sec, "capacity", "cs");
 *
 *
 * 【設定が適用される仕組み】
 *
 *   addSectionHandler("cs", handler) により購読登録
 *                         ↓
 *   parse() で設定を読み込む
 *                         ↓
 *   handler(sec, isDryRun, filename) が呼び出される
 *
 *
 * 【Dry-Run モード】
 *   isDryRun = true の場合は
 *   →設定の形式チェックのみ行い、実際の動作には反映しない
 *
 * 設定適用前の検証に使用（安全）
 *
 *
 * ============================================================
 * A. セクション例（nfd.confより）
 * ============================================================
 * 例：ContentStore の設定
 *
 *   content_store
 *   {
 *     capacity 65536
 *   }
 *
 * →対応するハンドラが呼ばれ、CSサイズを変更
 *
 *
 * 例：UDPフェース設定
 *
 *   face_system
 *   {
 *     enable_udp yes
 *     port 6363
 *   }
 *
 *
 * ============================================================
 * B. 新しい設定セクションを追加する手順
 * ============================================================
 * ① Forwarder側でセクション名に対応する関数を登録
 *
 *   configFile.addSectionHandler("my_section",
 *     [](const ConfigSection& sec, bool dryRun, const std::string& file) {
 *       // 設定適用ロジック
 *     });
 *
 * ② config へ my_section を書けば動作
 *
 *   my_section
 *   {
 *     key1 value
 *   }
 *
 *
 * ============================================================
 * C. MANET / 無線研究向けに触ることが多い設定
 * ============================================================
 *
 * 例：ContentStore 無効化（攻撃耐性評価で使用）
 *
 *   content_store
 *   {
 *     capacity 0
 *   }
 *
 * 例：Faceの保持時間（移動環境で有効）
 *
 *   face_system
 *   {
 *     idle_timeout 2000 ; ミリ秒
 *   }
 *
 *
 * ============================================================
 * D. Forwarder 起動時の設定処理フロー
 * ============================================================
 *
 *  nfd → Nfd::initialize()
 *       → ConfigFile::parse(nfd.conf)
 *           ↓
 *       各セクションハンドラを順次呼び出し
 *           ↓
 *       モジュール（CS/PIT/FIB/FaceSystem）が設定を反映
 *
 *
 * 【まとめ】
 *
 * - ConfigFile は NFD の設定読み込みすべてを管理する
 * - Forwarder の動作仕様を外から決める最重要コンポーネント
 * - 研究のために新しい設定項目を追加する際の入口となる
 */

namespace nfd {

/**
 * \brief A configuration file section.
 */
using ConfigSection = boost::property_tree::ptree;

/**
 * \brief An optional configuration file section.
 */
using OptionalConfigSection = boost::optional<const ConfigSection&>;

/**
 * \brief Callback to process a configuration file section.
 */
using ConfigSectionHandler = std::function<void(const ConfigSection& section, bool isDryRun,
                                                const std::string& filename)>;

/**
 * \brief Callback to process a configuration file section without a #ConfigSectionHandler.
 */
using UnknownConfigSectionHandler = std::function<void(const std::string& filename,
                                                       const std::string& sectionName,
                                                       const ConfigSection& section,
                                                       bool isDryRun)>;

/**
 * \brief Configuration file parsing utility.
 */
class ConfigFile : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  explicit
  ConfigFile(UnknownConfigSectionHandler unknownSectionCallback = throwErrorOnUnknownSection);

public: // unknown section handlers
  static void
  throwErrorOnUnknownSection(const std::string& filename,
                             const std::string& sectionName,
                             const ConfigSection& section,
                             bool isDryRun);

  static void
  ignoreUnknownSection(const std::string& filename,
                       const std::string& sectionName,
                       const ConfigSection& section,
                       bool isDryRun);

public: // parse helpers
  /** \brief Parse a config option that can be either "yes" or "no".
   *  \retval true "yes"
   *  \retval false "no"
   *  \throw Error the value is neither "yes" nor "no"
   */
  static bool
  parseYesNo(const ConfigSection& node, const std::string& key, const std::string& sectionName);

  static bool
  parseYesNo(const ConfigSection::value_type& option, const std::string& sectionName)
  {
    return parseYesNo(option.second, option.first, sectionName);
  }

  /**
   * \brief Parse a numeric (integral or floating point) config option.
   * \tparam T an arithmetic type
   *
   * \return the numeric value of the parsed option
   * \throw Error the value cannot be converted to the specified type
   */
  template<typename T>
  static T
  parseNumber(const ConfigSection& node, const std::string& key, const std::string& sectionName)
  {
    static_assert(std::is_arithmetic_v<T>);

    auto value = node.get_value_optional<T>();
    // Unsigned logic is workaround for https://redmine.named-data.net/issues/4489
    if (value &&
        (std::is_signed_v<T> || node.get_value<std::string>().find("-") == std::string::npos)) {
      return *value;
    }
    NDN_THROW(Error("Invalid value '" + node.get_value<std::string>() +
                    "' for option '" + key + "' in section '" + sectionName + "'"));
  }

  template<typename T>
  static T
  parseNumber(const ConfigSection::value_type& option, const std::string& sectionName)
  {
    return parseNumber<T>(option.second, option.first, sectionName);
  }

  /**
   * \brief Check that a value is within the inclusive range [min, max].
   * \throw Error the value is out of the acceptable range
   */
  template<typename T>
  static void
  checkRange(T value, T min, T max, const std::string& key, const std::string& sectionName)
  {
    static_assert(std::is_integral_v<T>);

    if (value < min || value > max) {
      NDN_THROW(Error("Invalid value '" + std::to_string(value) + "' for option '" + key +
                      "' in section '" + sectionName + "': out of acceptable range [" +
                      std::to_string(min) + ", " + std::to_string(max) + "]"));
    }
  }

public: // setup and parsing
  /// \brief Setup notification of configuration file sections.
  void
  addSectionHandler(const std::string& sectionName,
                    ConfigSectionHandler subscriber);

  /**
   * \param filename file to parse
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \throws ConfigFile::Error if file not found
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(const std::string& filename, bool isDryRun);

  /**
   * \param input configuration (as a string) to parse
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \param filename logical filename of the config file, can appear in error messages
   * \throws ConfigFile::Error if file not found
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(const std::string& input, bool isDryRun, const std::string& filename);

  /**
   * \param input stream to parse
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \param filename logical filename of the config file, can appear in error messages
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(std::istream& input, bool isDryRun, const std::string& filename);

  /**
   * \param config ConfigSection that needs to be processed
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \param filename logical filename of the config file, can appear in error messages
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(const ConfigSection& config, bool isDryRun, const std::string& filename);

private:
  void
  process(bool isDryRun, const std::string& filename) const;

private:
  UnknownConfigSectionHandler m_unknownSectionCallback;
  std::map<std::string, ConfigSectionHandler> m_subscriptions;
  ConfigSection m_global;
};

} // namespace nfd

#endif // NFD_DAEMON_COMMON_CONFIG_FILE_HPP

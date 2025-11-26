/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

 /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * NFD Logging Utilities
 * ---------------------------------------------------------------------------
 * 本ファイルは、Named Data Networking Forwarding Daemon (NFD) 内での
 * ロギング機能のためのヘッダです。ndn-cxx の Logger をラップしており、
 * NFD 内で統一的にログを出力するためのマクロを提供します。
 *
 * 提供される主なマクロ:
 *  1. NFD_LOG_INIT(name)
 *     - ログシステムを初期化
 *
 *  2. NFD_LOG_MEMBER_DECL()
 *     - クラス内でログメンバを宣言
 *
 *  3. NFD_LOG_MEMBER_INIT(cls, name)
 *     - クラス内のログメンバを初期化
 *
 *  4. ログレベル用マクロ
 *     - NFD_LOG_TRACE, NFD_LOG_DEBUG, NFD_LOG_INFO
 *     - NFD_LOG_WARN, NFD_LOG_ERROR, NFD_LOG_FATAL
 *
 * 利用例:
 *   NFD_LOG_INIT(myComponent);
 *   NFD_LOG_INFO("This is an informational message");
 *
 * 注意:
 *  - 本ヘッダは ndn-cxx の Logger をベースにしており、
 *    NFD 固有の名前空間管理 (nfd.name) を行う
 * ---------------------------------------------------------------------------
 */

#ifndef NFD_DAEMON_COMMON_LOGGER_HPP
#define NFD_DAEMON_COMMON_LOGGER_HPP

#include <ndn-cxx/util/logger.hpp>

#define NFD_LOG_INIT(name)                         NDN_LOG_INIT(nfd.name)
#define NFD_LOG_MEMBER_DECL()                      NDN_LOG_MEMBER_DECL()
#define NFD_LOG_MEMBER_DECL_SPECIALIZED(cls)       NDN_LOG_MEMBER_DECL_SPECIALIZED(cls)
#define NFD_LOG_MEMBER_INIT(cls, name)             NDN_LOG_MEMBER_INIT(cls, nfd.name)
#define NFD_LOG_MEMBER_INIT_SPECIALIZED(cls, name) NDN_LOG_MEMBER_INIT_SPECIALIZED(cls, nfd.name)

#define NFD_LOG_TRACE NDN_LOG_TRACE
#define NFD_LOG_DEBUG NDN_LOG_DEBUG
#define NFD_LOG_INFO  NDN_LOG_INFO
#define NFD_LOG_WARN  NDN_LOG_WARN
#define NFD_LOG_ERROR NDN_LOG_ERROR
#define NFD_LOG_FATAL NDN_LOG_FATAL

#endif // NFD_DAEMON_COMMON_LOGGER_HPP

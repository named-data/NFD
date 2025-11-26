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

#ifndef NFD_DAEMON_RIB_READVERTISE_READVERTISE_DESTINATION_HPP
#define NFD_DAEMON_RIB_READVERTISE_READVERTISE_DESTINATION_HPP

#include "readvertised-route.hpp"

/*
 * このファイルは ReadvertiseDestination クラスの定義を含む。
 * ReadvertiseDestination は、再広告（Readvertise）対象となる宛先を
 * 抽象化したインタフェースクラス（基底クラス）である。
 *
 * ◆役割概要
 * - 経路の再広告（追加・削除）機能のインタフェースを提供
 * - 具体的な宛先（例: NFD RIB など）は派生クラスで実装する
 * - 宛先が利用可能かどうかの状態管理も提供
 *
 * ◆主要機能（純粋仮想関数）
 * 1. advertise()
 *    - 指定された ReadvertisedRoute を宛先へ追加する処理を実装するための入口
 *    - 成功時・失敗時にコールバックを実行
 *
 * 2. withdraw()
 *    - 宛先から指定経路を削除するための入口
 *    - 成功時・失敗時にコールバックを実行
 *
 * ◆可用性制御
 * - isAvailable():
 *     宛先が利用可能かどうかを返す
 * - setAvailability():
 *     利用可能状態を更新（protected：派生クラスから操作）
 *
 * ◆イベント通知
 * - afterAvailabilityChange:
 *     利用可能状態の変化時にシグナル通知（true=利用可能 / false=利用不可）
 *
 * ◆まとめ
 * Readvertise（再広告）を行う際の
 * 「宛先の抽象インタフェース」として設計された基底クラス。
 * 利用可能状態の管理とイベント通知機能を備え、
 * 具象クラスで実際の RIB 操作やネットワーク制御を実装する。
 */

namespace nfd::rib {

/** \brief A destination to readvertise into.
 */
class ReadvertiseDestination : noncopyable
{
public:
  virtual
  ~ReadvertiseDestination() = default;

  virtual void
  advertise(const ReadvertisedRoute& rr,
            std::function<void()> successCb,
            std::function<void(const std::string&)> failureCb) = 0;

  virtual void
  withdraw(const ReadvertisedRoute& rr,
           std::function<void()> successCb,
           std::function<void(const std::string&)> failureCb) = 0;

  bool
  isAvailable() const
  {
    return m_isAvailable;
  }

protected:
  void
  setAvailability(bool isAvailable);

public:
  /** \brief Signals when the destination becomes available or unavailable.
   */
  signal::Signal<ReadvertiseDestination, bool> afterAvailabilityChange;

private:
  bool m_isAvailable = false;
};

} // namespace nfd::rib

#endif // NFD_DAEMON_RIB_READVERTISE_READVERTISE_DESTINATION_HPP

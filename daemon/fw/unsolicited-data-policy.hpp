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

#ifndef NFD_DAEMON_FW_UNSOLICITED_DATA_POLICY_HPP
#define NFD_DAEMON_FW_UNSOLICITED_DATA_POLICY_HPP

#include "face/face.hpp"

#include <functional>
#include <map>
#include <set>

/*
 * 【UnsolicitedDataPolicy の概要】
 *
 * 本クラスは、PIT に一致しない Data（＝ unsolicited Data）を受信した際に、
 * それを ContentStore に格納するか、破棄するかを決定するための
 * ポリシー（方針）を管理する仕組みを提供する。
 *
 * unsolicited Data とは？
 *   - PIT に対応する Interest が存在しない状態で届いた Data
 *   - 攻撃や無関係なパケットの可能性があるため扱いに注意が必要
 *
 * 【主な用途】
 *   - Forwarder の Data 処理パイプライン（onIncomingData 内）で利用され、
 *     unsolicited Data の安全かつ最適な取り扱いを制御する。
 *
 * 【UnsolicitedDataDecision】
 *   - DROP  : Data を破棄する
 *   - CACHE : ContentStore に登録する
 *
 * 【ポリシークラス】
 *   以下の派生クラスが用意されており、
 *   それぞれが Data の受信元に応じて判断を行う：
 *
 *   - DropAllUnsolicitedDataPolicy
 *       全て破棄（セキュリティを最重視）
 *
 *   - AdmitLocalUnsolicitedDataPolicy
 *       ローカル Face（アプリ等）からの Data をキャッシュ可とする
 *
 *   - AdmitNetworkUnsolicitedDataPolicy
 *       ネットワーク側 Face の Data を許可
 *
 *   - AdmitAllUnsolicitedDataPolicy
 *       全てキャッシュ（柔軟性を最優先）
 *
 * 【ポリシー登録と生成】
 *   - registerPolicy() により名前と一緒にレジストリへ登録できる
 *   - create(name) により Policy インスタンスを動的生成可能
 *   - getPolicyNames() で利用可能ポリシー一覧取得可能
 *
 *   ※ POLICY_NAME により設定ファイルから指定可能
 *
 * 【デフォルト設定】
 *   - DefaultUnsolicitedDataPolicy = DropAllUnsolicitedDataPolicy
 *     → unsolicited Data は基本破棄（安全志向）
 *
 * 【まとめ】
 * unsolicited Data の取り扱いは、性能とセキュリティのトレードオフとなる。
 * 本ポリシー機構により、NFD の運用環境に応じて柔軟に設定変更できる。
 */

namespace nfd::fw {

/**
 * \brief Decision made by UnsolicitedDataPolicy
 */
enum class UnsolicitedDataDecision {
  DROP, ///< the Data should be dropped
  CACHE ///< the Data should be cached in the ContentStore
};

std::ostream&
operator<<(std::ostream& os, UnsolicitedDataDecision d);

/**
 * \brief Determines how to process an unsolicited Data packet
 *
 * An incoming Data packet is *unsolicited* if it does not match any PIT entry.
 * This class assists forwarding pipelines to decide whether to drop an unsolicited Data
 * or admit it into the ContentStore.
 */
class UnsolicitedDataPolicy : noncopyable
{
public:
  virtual
  ~UnsolicitedDataPolicy() = default;

  virtual UnsolicitedDataDecision
  decide(const Face& inFace, const Data& data) const = 0;

public: // registry
  template<typename P>
  static void
  registerPolicy(std::string_view policyName = P::POLICY_NAME)
  {
    BOOST_ASSERT(!policyName.empty());
    auto r = getRegistry().insert_or_assign(std::string(policyName), [] { return make_unique<P>(); });
    BOOST_VERIFY(r.second);
  }

  /** \return an UnsolicitedDataPolicy identified by \p policyName,
   *          or nullptr if \p policyName is unknown
   */
  static unique_ptr<UnsolicitedDataPolicy>
  create(const std::string& policyName);

  /** \return a list of available policy names
   */
  static std::set<std::string>
  getPolicyNames();

private:
  using CreateFunc = std::function<unique_ptr<UnsolicitedDataPolicy>()>;
  using Registry = std::map<std::string, CreateFunc>; // indexed by policy name

  static Registry&
  getRegistry();
};

/**
 * \brief Drops all unsolicited Data
 */
class DropAllUnsolicitedDataPolicy final : public UnsolicitedDataPolicy
{
public:
  UnsolicitedDataDecision
  decide(const Face& inFace, const Data& data) const final;

public:
  static constexpr std::string_view POLICY_NAME{"drop-all"};
};

/**
 * \brief Admits unsolicited Data from local faces
 */
class AdmitLocalUnsolicitedDataPolicy final : public UnsolicitedDataPolicy
{
public:
  UnsolicitedDataDecision
  decide(const Face& inFace, const Data& data) const final;

public:
  static constexpr std::string_view POLICY_NAME{"admit-local"};
};

/**
 * \brief Admits unsolicited Data from non-local faces
 */
class AdmitNetworkUnsolicitedDataPolicy final : public UnsolicitedDataPolicy
{
public:
  UnsolicitedDataDecision
  decide(const Face& inFace, const Data& data) const final;

public:
  static constexpr std::string_view POLICY_NAME{"admit-network"};
};

/**
 * \brief Admits all unsolicited Data
 */
class AdmitAllUnsolicitedDataPolicy final : public UnsolicitedDataPolicy
{
public:
  UnsolicitedDataDecision
  decide(const Face& inFace, const Data& data) const final;

public:
  static constexpr std::string_view POLICY_NAME{"admit-all"};
};

/**
 * \brief The default UnsolicitedDataPolicy
 */
using DefaultUnsolicitedDataPolicy = DropAllUnsolicitedDataPolicy;

} // namespace nfd::fw

/**
 * \brief Registers an unsolicited data policy.
 * \param P A subclass of nfd::fw::UnsolicitedDataPolicy. \p P must have a static const data
 *          member `POLICY_NAME` convertible to std::string_view that contains the policy name.
 */
#define NFD_REGISTER_UNSOLICITED_DATA_POLICY(P)                     \
static class NfdAuto ## P ## UnsolicitedDataPolicyRegistrationClass \
{                                                                   \
public:                                                             \
  NfdAuto ## P ## UnsolicitedDataPolicyRegistrationClass()          \
  {                                                                 \
    ::nfd::fw::UnsolicitedDataPolicy::registerPolicy<P>();          \
  }                                                                 \
} g_nfdAuto ## P ## UnsolicitedDataPolicyRegistrationVariable

#endif // NFD_DAEMON_FW_UNSOLICITED_DATA_POLICY_HPP

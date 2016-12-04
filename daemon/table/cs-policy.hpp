/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#ifndef NFD_DAEMON_TABLE_CS_POLICY_HPP
#define NFD_DAEMON_TABLE_CS_POLICY_HPP

#include "cs-internal.hpp"
#include "cs-entry-impl.hpp"

namespace nfd {
namespace cs {

class Cs;

/** \brief represents a CS replacement policy
 */
class Policy : noncopyable
{
public: // registry
  template<typename P>
  static void
  registerPolicy(const std::string& policyName = P::POLICY_NAME)
  {
    Registry& registry = getRegistry();
    BOOST_ASSERT(registry.count(policyName) == 0);
    registry[policyName] = [] { return make_unique<P>(); };
  }

  /** \return a cs::Policy identified by \p policyName,
   *          or nullptr if \p policyName is unknown
   */
  static unique_ptr<Policy>
  create(const std::string& policyName);

  /** \return a list of available policy names
   */
  static std::set<std::string>
  getPolicyNames();

public:
  explicit
  Policy(const std::string& policyName);

  virtual
  ~Policy() = default;

  const std::string&
  getName() const;

public:
  /** \brief gets cs
   */
  Cs*
  getCs() const;

  /** \brief sets cs
   */
  void
  setCs(Cs *cs);

  /** \brief gets hard limit (in number of entries)
   */
  size_t
  getLimit() const;

  /** \brief sets hard limit (in number of entries)
   *  \post getLimit() == nMaxEntries
   *  \post cs.size() <= getLimit()
   *
   *  The policy may evict entries if necessary.
   */
  void
  setLimit(size_t nMaxEntries);

  /** \brief emits when an entry is being evicted
   *
   *  A policy implementation should emit this signal to cause CS to erase the entry from its index.
   *  CS should connect to this signal and erase the entry upon signal emission.
   */
  signal::Signal<Policy, iterator> beforeEvict;

  /** \brief invoked by CS after a new entry is inserted
   *  \post cs.size() <= getLimit()
   *
   *  The policy may evict entries if necessary.
   *  During this process, \p i might be evicted.
   */
  void
  afterInsert(iterator i);

  /** \brief invoked by CS after an existing entry is refreshed by same Data
   *
   *  The policy may witness this refresh to make better eviction decisions in the future.
   */
  void
  afterRefresh(iterator i);

  /** \brief invoked by CS before an entry is erased due to management command
   *  \warning CS must not invoke this method if an entry is erased due to eviction.
   */
  void
  beforeErase(iterator i);

  /** \brief invoked by CS before an entry is used to match a lookup
   *
   *  The policy may witness this usage to make better eviction decisions in the future.
   */
  void
  beforeUse(iterator i);

protected:
  /** \brief invoked after a new entry is created in CS
   *
   *  When overridden in a subclass, a policy implementation should decide whether to accept \p i.
   *  If \p i is accepted, it should be inserted into a cleanup index.
   *  Otherwise, \p beforeEvict signal should be emitted with \p i to inform CS to erase the entry.
   *  A policy implementation may decide to evict other entries by emitting \p beforeEvict signal,
   *  in order to keep CS size under limit.
   */
  virtual void
  doAfterInsert(iterator i) = 0;

  /** \brief invoked after an existing entry is refreshed by same Data
   *
   *  When overridden in a subclass, a policy implementation may witness this operation
   *  and adjust its cleanup index.
   */
  virtual void
  doAfterRefresh(iterator i) = 0;

  /** \brief invoked before an entry is erased due to management command
   *  \note This will not be invoked for an entry being evicted by policy.
   *
   *  When overridden in a subclass, a policy implementation should erase \p i
   *  from its cleanup index without emitted \p afterErase signal.
   */
  virtual void
  doBeforeErase(iterator i) = 0;

  /** \brief invoked before an entry is used to match a lookup
   *
   *  When overridden in a subclass, a policy implementation may witness this operation
   *  and adjust its cleanup index.
   */
  virtual void
  doBeforeUse(iterator i) = 0;

  /** \brief evicts zero or more entries
   *  \post CS size does not exceed hard limit
   */
  virtual void
  evictEntries() = 0;

protected:
  DECLARE_SIGNAL_EMIT(beforeEvict)

private: // registry
  typedef std::function<unique_ptr<Policy>()> CreateFunc;
  typedef std::map<std::string, CreateFunc> Registry; // indexed by policy name

  static Registry&
  getRegistry();

private:
  std::string m_policyName;
  size_t m_limit;
  Cs* m_cs;
};

inline const std::string&
Policy::getName() const
{
  return m_policyName;
}

inline Cs*
Policy::getCs() const
{
  return m_cs;
}

inline void
Policy::setCs(Cs *cs)
{
  m_cs = cs;
}

inline size_t
Policy::getLimit() const
{
  return m_limit;
}

} // namespace cs
} // namespace nfd

/** \brief registers a CS policy
 *  \param P a subclass of nfd::cs::Policy
 */
#define NFD_REGISTER_CS_POLICY(P)                      \
static class NfdAuto ## P ## CsPolicyRegistrationClass \
{                                                      \
public:                                                \
  NfdAuto ## P ## CsPolicyRegistrationClass()          \
  {                                                    \
    ::nfd::cs::Policy::registerPolicy<P>();            \
  }                                                    \
} g_nfdAuto ## P ## CsPolicyRegistrationVariable

#endif // NFD_DAEMON_TABLE_CS_POLICY_HPP

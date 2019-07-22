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

#ifndef NFD_DAEMON_TABLE_CS_HPP
#define NFD_DAEMON_TABLE_CS_HPP

#include "cs-policy.hpp"

namespace nfd {
namespace cs {

/** \brief implements the Content Store
 *
 *  This Content Store implementation consists of a Table and a replacement policy.
 *
 *  The Table is a container ( \c std::set ) sorted by full Names of stored Data packets.
 *  Data packets are wrapped in Entry objects. Each Entry contains the Data packet itself,
 *  and a few additional attributes such as when the Data becomes non-fresh.
 *
 *  The replacement policy is implemented in a subclass of \c Policy.
 */
class Cs : noncopyable
{
public:
  explicit
  Cs(size_t nMaxPackets = 10);

  /** \brief inserts a Data packet
   */
  void
  insert(const Data& data, bool isUnsolicited = false);

  /** \brief asynchronously erases entries under \p prefix
   *  \tparam AfterEraseCallback `void f(size_t nErased)`
   *  \param prefix name prefix of entries
   *  \param limit max number of entries to erase
   *  \param cb callback to receive the actual number of erased entries; must not be empty;
   *            it may be invoked either before or after erase() returns
   */
  template<typename AfterEraseCallback>
  void
  erase(const Name& prefix, size_t limit, AfterEraseCallback&& cb)
  {
    size_t nErased = eraseImpl(prefix, limit);
    cb(nErased);
  }

  /** \brief finds the best matching Data packet
   *  \tparam HitCallback `void f(const Interest&, const Data&)`
   *  \tparam MissCallback `void f(const Interest&)`
   *  \param interest the Interest for lookup
   *  \param hit a callback if a match is found; must not be empty
   *  \param miss a callback if there's no match; must not be empty
   *  \note A lookup invokes either callback exactly once.
   *        The callback may be invoked either before or after find() returns
   */
  template<typename HitCallback, typename MissCallback>
  void
  find(const Interest& interest, HitCallback&& hit, MissCallback&& miss) const
  {
    auto match = findImpl(interest);
    if (match == m_table.end()) {
      miss(interest);
      return;
    }
    hit(interest, match->getData());
  }

  /** \brief get number of stored packets
   */
  size_t
  size() const
  {
    return m_table.size();
  }

public: // configuration
  /** \brief get capacity (in number of packets)
   */
  size_t
  getLimit() const
  {
    return m_policy->getLimit();
  }

  /** \brief change capacity (in number of packets)
   */
  void
  setLimit(size_t nMaxPackets)
  {
    return m_policy->setLimit(nMaxPackets);
  }

  /** \brief get replacement policy
   */
  Policy*
  getPolicy() const
  {
    return m_policy.get();
  }

  /** \brief change replacement policy
   *  \pre size() == 0
   */
  void
  setPolicy(unique_ptr<Policy> policy);

  /** \brief get CS_ENABLE_ADMIT flag
   *  \sa https://redmine.named-data.net/projects/nfd/wiki/CsMgmt#Update-config
   */
  bool
  shouldAdmit() const
  {
    return m_shouldAdmit;
  }

  /** \brief set CS_ENABLE_ADMIT flag
   *  \sa https://redmine.named-data.net/projects/nfd/wiki/CsMgmt#Update-config
   */
  void
  enableAdmit(bool shouldAdmit);

  /** \brief get CS_ENABLE_SERVE flag
   *  \sa https://redmine.named-data.net/projects/nfd/wiki/CsMgmt#Update-config
   */
  bool
  shouldServe() const
  {
    return m_shouldServe;
  }

  /** \brief set CS_ENABLE_SERVE flag
   *  \sa https://redmine.named-data.net/projects/nfd/wiki/CsMgmt#Update-config
   */
  void
  enableServe(bool shouldServe);

public: // enumeration
  using const_iterator = Table::const_iterator;

  const_iterator
  begin() const
  {
    return m_table.begin();
  }

  const_iterator
  end() const
  {
    return m_table.end();
  }

private:
  std::pair<const_iterator, const_iterator>
  findPrefixRange(const Name& prefix) const;

  size_t
  eraseImpl(const Name& prefix, size_t limit);

  const_iterator
  findImpl(const Interest& interest) const;

  void
  setPolicyImpl(unique_ptr<Policy> policy);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  dump();

private:
  Table m_table;
  unique_ptr<Policy> m_policy;
  signal::ScopedConnection m_beforeEvictConnection;

  bool m_shouldAdmit = true; ///< if false, no Data will be admitted
  bool m_shouldServe = true; ///< if false, all lookups will miss
};

} // namespace cs

using cs::Cs;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_CS_HPP

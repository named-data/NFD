/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#ifndef NFD_DAEMON_TABLE_STRATEGY_CHOICE_HPP
#define NFD_DAEMON_TABLE_STRATEGY_CHOICE_HPP

#include "strategy-choice-entry.hpp"
#include "name-tree.hpp"

#include <boost/range/adaptor/transformed.hpp>

namespace nfd {

class Forwarder;

namespace strategy_choice {

/** \brief represents the Strategy Choice table
 *
 *  The Strategy Choice table maintains available Strategy types,
 *  and associates Name prefixes with Strategy types.
 *
 *  Each strategy is identified by a strategyName.
 *  It's recommended to include a version number as the last component of strategyName.
 *
 *  A Name prefix is owned by a strategy if a longest prefix match on the
 *  Strategy Choice table returns that strategy.
 */
class StrategyChoice : noncopyable
{
public:
  explicit
  StrategyChoice(Forwarder& forwarder);

  size_t
  size() const
  {
    return m_nItems;
  }

  /** \brief set the default strategy
   *
   *  This must be called by forwarder constructor.
   */
  void
  setDefaultStrategy(const Name& strategyName);

public: // Strategy Choice table
  class InsertResult
  {
  public:
    explicit
    operator bool() const
    {
      return m_status == OK;
    }

    bool
    isRegistered() const
    {
      return m_status == OK || m_status == EXCEPTION;
    }

    /** \brief get a status code for use in management command response
     */
    int
    getStatusCode() const
    {
      return static_cast<int>(m_status);
    }

  private:
    enum Status {
      OK             = 200,
      NOT_REGISTERED = 404,
      EXCEPTION      = 409,
      DEPTH_EXCEEDED = 414,
    };

    // implicitly constructible from Status
    InsertResult(Status status, const std::string& exceptionMessage = "");

  private:
    Status m_status;
    std::string m_exceptionMessage;

    friend class StrategyChoice;
    friend std::ostream& operator<<(std::ostream&, const InsertResult&);
  };

  /** \brief set strategy of \p prefix to be \p strategyName
   *  \param prefix the name prefix to change strategy
   *  \param strategyName strategy instance name, may contain version and parameters;
   *                      strategy must have been registered
   *  \return convertible to true on success; convertible to false on failure
   */
  InsertResult
  insert(const Name& prefix, const Name& strategyName);

  /** \brief make prefix to inherit strategy from its parent
   *
   *  not allowed for root prefix (ndn:/)
   */
  void
  erase(const Name& prefix);

  /** \brief get strategy Name of prefix
   *  \return true and strategyName at exact match, or false
   */
  std::pair<bool, Name>
  get(const Name& prefix) const;

public: // effective strategy
  /** \brief get effective strategy for prefix
   */
  fw::Strategy&
  findEffectiveStrategy(const Name& prefix) const;

  /** \brief get effective strategy for pitEntry
   *
   *  This is equivalent to .findEffectiveStrategy(pitEntry.getName())
   */
  fw::Strategy&
  findEffectiveStrategy(const pit::Entry& pitEntry) const;

  /** \brief get effective strategy for measurementsEntry
   *
   *  This is equivalent to .findEffectiveStrategy(measurementsEntry.getName())
   */
  fw::Strategy&
  findEffectiveStrategy(const measurements::Entry& measurementsEntry) const;

public: // enumeration
  typedef boost::transformed_range<name_tree::GetTableEntry<Entry>, const name_tree::Range> Range;
  typedef boost::range_iterator<Range>::type const_iterator;

  /** \return an iterator to the beginning
   *  \note Iteration order is implementation-defined.
   *  \warning Undefined behavior may occur if a FIB/PIT/Measurements/StrategyChoice entry
   *           is inserted or erased during enumeration.
   */
  const_iterator
  begin() const
  {
    return this->getRange().begin();
  }

  /** \return an iterator to the end
   *  \sa begin()
   */
  const_iterator
  end() const
  {
    return this->getRange().end();
  }

private:
  void
  changeStrategy(Entry& entry,
                 fw::Strategy& oldStrategy,
                 fw::Strategy& newStrategy);

  /** \tparam K a parameter acceptable to NameTree::findLongestPrefixMatch
   */
  template<typename K>
  fw::Strategy&
  findEffectiveStrategyImpl(const K& key) const;

  Range
  getRange() const;

private:
  Forwarder& m_forwarder;
  NameTree& m_nameTree;
  size_t m_nItems;
};

std::ostream&
operator<<(std::ostream& os, const StrategyChoice::InsertResult& res);

} // namespace strategy_choice

using strategy_choice::StrategyChoice;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_STRATEGY_CHOICE_HPP

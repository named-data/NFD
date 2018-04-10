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

#ifndef NFD_DAEMON_TABLE_PIT_HPP
#define NFD_DAEMON_TABLE_PIT_HPP

#include "pit-entry.hpp"
#include "pit-iterator.hpp"

namespace nfd {
namespace pit {

/** \class nfd::pit::DataMatchResult
 *  \brief An unordered iterable of all PIT entries matching Data.
 *
 *  This type has the following member functions:
 *  - `iterator<shared_ptr<Entry>> begin()`
 *  - `iterator<shared_ptr<Entry>> end()`
 *  - `size_t size() const`
 */
using DataMatchResult = std::vector<shared_ptr<Entry>>;

/** \brief represents the Interest Table
 */
class Pit : noncopyable
{
public:
  explicit
  Pit(NameTree& nameTree);

  /** \return number of entries
   */
  size_t
  size() const
  {
    return m_nItems;
  }

  /** \brief finds a PIT entry for Interest
   *  \param interest the Interest
   *  \return an existing entry with same Name and Selectors; otherwise nullptr
   */
  shared_ptr<Entry>
  find(const Interest& interest) const
  {
    return const_cast<Pit*>(this)->findOrInsert(interest, false).first;
  }

  /** \brief inserts a PIT entry for Interest
   *  \param interest the Interest; must be created with make_shared
   *  \return a new or existing entry with same Name and Selectors,
   *          and true for new entry, false for existing entry
   */
  std::pair<shared_ptr<Entry>, bool>
  insert(const Interest& interest)
  {
    return this->findOrInsert(interest, true);
  }

  /** \brief performs a Data match
   *  \return an iterable of all PIT entries matching data
   */
  DataMatchResult
  findAllDataMatches(const Data& data) const;

  /** \brief deletes an entry
   */
  void
  erase(Entry* entry)
  {
    this->erase(entry, true);
  }

  /** \brief deletes in-record and out-record for face
   */
  void
  deleteInOutRecords(Entry* entry, const Face& face);

public: // enumeration
  typedef Iterator const_iterator;

  /** \return an iterator to the beginning
   *  \note Iteration order is implementation-defined.
   *  \warning Undefined behavior may occur if a FIB/PIT/Measurements/StrategyChoice entry
   *           is inserted or erased during enumeration.
   */
  const_iterator
  begin() const;

  /** \return an iterator to the end
   *  \sa begin()
   */
  const_iterator
  end() const
  {
    return Iterator();
  }

private:
  void
  erase(Entry* pitEntry, bool canDeleteNte);

  /** \brief finds or inserts a PIT entry for Interest
   *  \param interest the Interest; must be created with make_shared if allowInsert
   *  \param allowInsert whether inserting new entry is allowed.
   *  \return if allowInsert, a new or existing entry with same Name+Selectors,
   *          and true for new entry, false for existing entry;
   *          if not allowInsert, an existing entry with same Name+Selectors and false,
   *          or {nullptr, true} if there's no existing entry
   */
  std::pair<shared_ptr<Entry>, bool>
  findOrInsert(const Interest& interest, bool allowInsert);

private:
  NameTree& m_nameTree;
  size_t m_nItems;
};

} // namespace pit

using pit::Pit;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_PIT_HPP

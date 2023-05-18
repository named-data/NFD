/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#ifndef NFD_DAEMON_TABLE_PIT_ITERATOR_HPP
#define NFD_DAEMON_TABLE_PIT_ITERATOR_HPP

#include "name-tree.hpp"
#include "pit-entry.hpp"

namespace nfd::pit {

/**
 * \brief PIT iterator.
 */
class Iterator
{
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type        = const Entry;
  using difference_type   = std::ptrdiff_t;
  using pointer           = value_type*;
  using reference         = value_type&;

  /** \brief Constructor.
   *  \param ntIt a name tree iterator that visits name tree entries with one or more PIT entries
   *  \param iPitEntry make this iterator to dereference to the i-th PIT entry in name tree entry
   */
  explicit
  Iterator(const NameTree::const_iterator& ntIt = {}, size_t iPitEntry = 0);

  const Entry&
  operator*() const
  {
    return *this->operator->();
  }

  const shared_ptr<Entry>&
  operator->() const
  {
    BOOST_ASSERT(m_ntIt != NameTree::const_iterator());
    BOOST_ASSERT(m_iPitEntry < m_ntIt->getPitEntries().size());
    return m_ntIt->getPitEntries()[m_iPitEntry];
  }

  Iterator&
  operator++();

  Iterator
  operator++(int);

  friend bool
  operator==(const Iterator& lhs, const Iterator& rhs) noexcept
  {
    return lhs.m_ntIt == rhs.m_ntIt &&
           lhs.m_iPitEntry == rhs.m_iPitEntry;
  }

  friend bool
  operator!=(const Iterator& lhs, const Iterator& rhs) noexcept
  {
    return !(lhs == rhs);
  }

private:
  NameTree::const_iterator m_ntIt; ///< current name tree entry
  size_t m_iPitEntry; ///< current PIT entry within m_ntIt->getPitEntries()
};

} // namespace nfd::pit

#endif // NFD_DAEMON_TABLE_PIT_ITERATOR_HPP

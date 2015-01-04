/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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
 *
 * \author Ilya Moiseenko <http://ilyamoiseenko.com/>
 * \author Junxiao Shi <http://www.cs.arizona.edu/people/shijunxiao/>
 * \author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 */

#ifndef NFD_DAEMON_TABLE_CS_SKIP_LIST_ENTRY_HPP
#define NFD_DAEMON_TABLE_CS_SKIP_LIST_ENTRY_HPP

#include "common.hpp"
#include "cs-entry.hpp"

namespace nfd {
namespace cs {
namespace skip_list {

/** \brief represents an entry in a CS with skip list implementation
 */
class Entry : public cs::Entry
{
public:
  typedef std::map<int, std::list<Entry*>::iterator > LayerIterators;

  Entry() = default;

  /** \brief releases reference counts on shared objects
   */
  void
  release();

  /** \brief saves the iterator pointing to the CS entry on a specific layer of skip list
   */
  void
  setIterator(int layer, const LayerIterators::mapped_type& layerIterator);

  /** \brief removes the iterator pointing to the CS entry on a specific layer of skip list
   */
  void
  removeIterator(int layer);

  /** \brief returns the table containing <layer, iterator> pairs.
   */
  const LayerIterators&
  getIterators() const;

private:
  /** \brief prints <layer, iterator> pairs.
   */
  void
  printIterators() const;

private:
  LayerIterators m_layerIterators;
};

inline const Entry::LayerIterators&
Entry::getIterators() const
{
  return m_layerIterators;
}

} // namespace skip_list
} // namespace cs
} // namespace nfd

#endif // NFD_DAEMON_TABLE_CS_SKIP_LIST_ENTRY_HPP

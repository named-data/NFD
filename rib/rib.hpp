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
 */

#ifndef NFD_RIB_RIB_HPP
#define NFD_RIB_RIB_HPP

#include "common.hpp"
#include "rib-entry.hpp"
#include <ndn-cxx/management/nfd-control-command.hpp>

namespace nfd {
namespace rib {

/** \brief represents the RIB
 */
class Rib : noncopyable
{
public:
  typedef std::list<shared_ptr<RibEntry> > RibEntryList;
  typedef std::map<Name, shared_ptr<RibEntry> > RibTable;
  typedef RibTable::const_iterator const_iterator;
  typedef std::map<uint64_t, std::list<shared_ptr<RibEntry> > > FaceLookupTable;

  Rib();

  ~Rib();

  const_iterator
  find(const Name& prefix) const;

  shared_ptr<FaceEntry>
  find(const Name& prefix, const FaceEntry& face) const;

  void
  insert(const Name& prefix, const FaceEntry& face);

  void
  erase(const Name& prefix, const FaceEntry& face);

  void
  erase(const uint64_t faceId);

  const_iterator
  begin() const;

  const_iterator
  end() const;

  size_t
  size() const;

  bool
  empty() const;


  shared_ptr<RibEntry>
  findParent(const Name& prefix) const;

  /** \brief finds namespaces under the passed prefix
   *  \return{ a list of entries which are under the passed prefix }
   */
  std::list<shared_ptr<RibEntry> >
  findDescendants(const Name& prefix) const;

  RibTable::iterator
  eraseEntry(RibTable::iterator it);

  void
  eraseEntry(const Name& name);

private:
  RibTable m_rib;
  FaceLookupTable m_faceMap;
  size_t m_nItems;
};

inline Rib::const_iterator
Rib::begin() const
{
  return m_rib.begin();
}

inline Rib::const_iterator
Rib::end() const
{
  return m_rib.end();
}

inline size_t
Rib::size() const
{
  return m_nItems;
}

inline bool
Rib::empty() const
{
  return m_rib.empty();
}

std::ostream&
operator<<(std::ostream& os, const FaceEntry& entry);

std::ostream&
operator<<(std::ostream& os, const RibEntry& entry);

std::ostream&
operator<<(std::ostream& os, const Rib& rib);

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_RIB_HPP

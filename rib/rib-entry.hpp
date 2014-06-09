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

#ifndef NFD_RIB_RIB_ENTRY_HPP
#define NFD_RIB_RIB_ENTRY_HPP

#include "face-entry.hpp"

namespace nfd {
namespace rib {

/** \class RibEntry
 *  \brief represents a namespace
 */
class RibEntry : public enable_shared_from_this<RibEntry>
{
public:
  typedef std::list<FaceEntry> FaceList;
  typedef FaceList::iterator iterator;
  typedef FaceList::const_iterator const_iterator;

  RibEntry()
  {
  }

  void
  setName(const Name& prefix);

  const Name&
  getName() const;

  shared_ptr<RibEntry>
  getParent() const;

  bool
  hasParent() const;

  void
  addChild(shared_ptr<RibEntry> child);

  void
  removeChild(shared_ptr<RibEntry> child);

  std::list<shared_ptr<RibEntry> >&
  getChildren();

  bool
  hasChildren() const;

  /** \brief inserts a new face into the entry's face list
   *  If another entry already exists with the same faceId and origin,
   *  the new face is not inserted.
   *  \return{ true if the face is inserted, false otherwise }
   */
  bool
  insertFace(const FaceEntry& face);

  /** \brief erases a FaceEntry with the same faceId and origin
   *  \return{ true if the face is removed, false otherwise }
   */
  bool
  eraseFace(const FaceEntry& face);

  /** \brief erases a FaceEntry with the passed iterator
   *  \return{ an iterator to the element that followed the erased iterator }
   */
  iterator
  eraseFace(FaceList::iterator face);

  bool
  hasFaceId(const uint64_t faceId) const;

  FaceList&
  getFaces();

  iterator
  findFace(const FaceEntry& face);

  const_iterator
  cbegin() const;

  const_iterator
  cend() const;

  iterator
  begin();

  iterator
  end();

private:
  void
  setParent(shared_ptr<RibEntry> parent);

private:
  Name m_name;
  std::list<shared_ptr<RibEntry> > m_children;
  shared_ptr<RibEntry> m_parent;
  FaceList m_faces;
  FaceList m_inheritedFaces;
};

inline void
RibEntry::setName(const Name& prefix)
{
  m_name = prefix;
}

inline const Name&
RibEntry::getName() const
{
  return m_name;
}

inline void
RibEntry::setParent(shared_ptr<RibEntry> parent)
{
  m_parent = parent;
}

inline shared_ptr<RibEntry>
RibEntry::getParent() const
{
  return m_parent;
}

inline std::list<shared_ptr<RibEntry> >&
RibEntry::getChildren()
{
  return m_children;
}

inline RibEntry::FaceList&
RibEntry::getFaces()
{
  return m_faces;
}

inline RibEntry::const_iterator
RibEntry::cbegin() const
{
  return m_faces.begin();
}

inline RibEntry::const_iterator
RibEntry::cend() const
{
  return m_faces.end();
}

inline RibEntry::iterator
RibEntry::begin()
{
  return m_faces.begin();
}

inline RibEntry::iterator
RibEntry::end()
{
  return m_faces.end();
}

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_RIB_ENTRY_HPP

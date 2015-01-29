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

#ifndef NFD_DAEMON_FW_FACE_TABLE_HPP
#define NFD_DAEMON_FW_FACE_TABLE_HPP

#include "face/face.hpp"
#include <boost/range/adaptor/map.hpp>

namespace nfd {

class Forwarder;

/** \brief container of all Faces
 */
class FaceTable : noncopyable
{
public:
  explicit
  FaceTable(Forwarder& forwarder);

  VIRTUAL_WITH_TESTS
  ~FaceTable();

  VIRTUAL_WITH_TESTS void
  add(shared_ptr<Face> face);

  /// add a special Face with a reserved FaceId
  VIRTUAL_WITH_TESTS void
  addReserved(shared_ptr<Face> face, FaceId faceId);

  VIRTUAL_WITH_TESTS shared_ptr<Face>
  get(FaceId id) const;

  size_t
  size() const;

public: // enumeration
  typedef std::map<FaceId, shared_ptr<Face>> FaceMap;

  typedef boost::select_second_const_range<FaceMap> ForwardRange;

  /** \brief ForwardIterator for shared_ptr<Face>
   */
  typedef boost::range_iterator<ForwardRange>::type const_iterator;

  const_iterator
  begin() const;

  const_iterator
  end() const;

public: // signals
  /** \brief fires after a Face is added
   */
  signal::Signal<FaceTable, shared_ptr<Face>> onAdd;

  /** \brief fires before a Face is removed
   *
   *  FaceId is valid when this event is fired
   */
  signal::Signal<FaceTable, shared_ptr<Face>> onRemove;

private:
  void
  addImpl(shared_ptr<Face> face, FaceId faceId);

  // remove is private because it's a handler of face.onFail signal.
  // face->close() closes the face and triggers .remove()
  void
  remove(shared_ptr<Face> face, const std::string& reason);

  ForwardRange
  getForwardRange() const;

private:
  Forwarder& m_forwarder;
  FaceId m_lastFaceId;
  FaceMap m_faces;
};

} // namespace nfd

#endif // NFD_DAEMON_FW_FACE_TABLE_HPP

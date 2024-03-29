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

#ifndef NFD_DAEMON_FW_FACE_TABLE_HPP
#define NFD_DAEMON_FW_FACE_TABLE_HPP

#include "face/face.hpp"

#include <boost/range/adaptor/indirected.hpp>
#include <boost/range/adaptor/map.hpp>

#include <map>

namespace nfd {

/**
 * \brief Container of all faces.
 */
class FaceTable : noncopyable
{
public:
  /** \brief Add a face.
   *
   *  FaceTable obtains shared ownership of the face.
   *  The channel or protocol factory that creates the face may retain ownership.
   */
  void
  add(shared_ptr<Face> face);

  /** \brief Add a special face with a reserved FaceId.
   */
  void
  addReserved(shared_ptr<Face> face, FaceId faceId);

  /** \brief Get face by FaceId.
   *  \return A pointer to the face if found, nullptr otherwise;
   *          `face->shared_from_this()` can be used if a `shared_ptr` is desired.
   */
  Face*
  get(FaceId id) const noexcept;

  /** \brief Return the total number of faces.
   */
  size_t
  size() const noexcept;

public: // enumeration
  using FaceMap = std::map<FaceId, shared_ptr<Face>>;
  using ForwardRange = boost::indirected_range<const boost::select_second_const_range<FaceMap>>;
  using const_iterator = boost::range_iterator<ForwardRange>::type;

  const_iterator
  begin() const;

  const_iterator
  end() const;

public: // signals
  /** \brief Fires immediately after a face is added.
   */
  signal::Signal<FaceTable, Face> afterAdd;

  /** \brief Fires immediately before a face is removed.
   *
   *  When this signal is emitted, the face is still in FaceTable and has a valid FaceId.
   */
  signal::Signal<FaceTable, Face> beforeRemove;

private:
  void
  addImpl(shared_ptr<Face> face, FaceId faceId);

  void
  remove(FaceId faceId);

  ForwardRange
  getForwardRange() const;

private:
  FaceId m_lastFaceId = face::FACEID_RESERVED_MAX;
  FaceMap m_faces;
};

} // namespace nfd

#endif // NFD_DAEMON_FW_FACE_TABLE_HPP

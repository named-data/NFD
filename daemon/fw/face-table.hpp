/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FW_FACE_TABLE_HPP
#define NFD_FW_FACE_TABLE_HPP

#include "face/face.hpp"
#include "core/map-value-iterator.hpp"

namespace nfd
{

class Forwarder;

/** \brief container of all Faces
 */
class FaceTable
{
public:
  explicit
  FaceTable(Forwarder& forwarder);

  VIRTUAL_WITH_TESTS
  ~FaceTable();

  VIRTUAL_WITH_TESTS void
  add(shared_ptr<Face> face);

  VIRTUAL_WITH_TESTS shared_ptr<Face>
  get(FaceId id) const;

  size_t
  size() const;

public: // enumeration
  typedef std::map<FaceId, shared_ptr<Face> > FaceMap;

  /** \brief ForwardIterator for shared_ptr<Face>
   */
  typedef MapValueIterator<FaceMap> const_iterator;

  /** \brief ReverseIterator for shared_ptr<Face>
   */
  typedef MapValueReverseIterator<FaceMap> const_reverse_iterator;

  const_iterator
  begin() const;

  const_iterator
  end() const;

  const_reverse_iterator
  rbegin() const;

  const_reverse_iterator
  rend() const;

public: // events
  /** \brief fires after a Face is added
   */
  EventEmitter<shared_ptr<Face> > onAdd;

  /** \brief fires before a Face is removed
   *
   *  FaceId is valid when this event is fired
   */
  EventEmitter<shared_ptr<Face> > onRemove;

private:
  // remove is private because it's a subscriber of face.onFail event.
  // face->close() closes a face and would trigger .remove(face)
  void
  remove(shared_ptr<Face> face);

private:
  Forwarder& m_forwarder;
  FaceId m_lastFaceId;
  FaceMap m_faces;
};

inline shared_ptr<Face>
FaceTable::get(FaceId id) const
{
  std::map<FaceId, shared_ptr<Face> >::const_iterator i = m_faces.find(id);
  return (i == m_faces.end()) ? (shared_ptr<Face>()) : (i->second);
}

inline size_t
FaceTable::size() const
{
  return m_faces.size();
}

inline FaceTable::const_iterator
FaceTable::begin() const
{
  return const_iterator(m_faces.begin());
}

inline FaceTable::const_iterator
FaceTable::end() const
{
  return const_iterator(m_faces.end());
}

inline FaceTable::const_reverse_iterator
FaceTable::rbegin() const
{
  return const_reverse_iterator(m_faces.rbegin());
}

inline FaceTable::const_reverse_iterator
FaceTable::rend() const
{
  return const_reverse_iterator(m_faces.rend());
}

} // namespace nfd

#endif // NFD_FW_FACE_TABLE_HPP

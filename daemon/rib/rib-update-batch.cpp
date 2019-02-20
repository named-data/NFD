/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include "rib-update-batch.hpp"

namespace nfd {
namespace rib {

RibUpdateBatch::RibUpdateBatch(uint64_t faceId)
  : m_faceId(faceId)
{
}

void
RibUpdateBatch::add(const RibUpdate& update)
{
  BOOST_ASSERT(m_faceId == update.getRoute().faceId);

  m_updates.push_back(update);
}

RibUpdateBatch::const_iterator
RibUpdateBatch::begin() const
{
  return m_updates.begin();
}

RibUpdateBatch::const_iterator
RibUpdateBatch::end() const
{
  return m_updates.end();
}

size_t
RibUpdateBatch::size() const
{
  return m_updates.size();
}

} // namespace rib
} // namespace nfd

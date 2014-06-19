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

namespace nfd {
namespace rib {
namespace tests {

inline FaceEntry
createFaceEntry(uint64_t faceId, uint64_t origin, uint64_t cost, uint64_t flags)
{
  FaceEntry temp;
  temp.faceId = faceId;
  temp.origin = origin;
  temp.cost = cost;
  temp.flags = flags;

  return temp;
}

inline bool
compareNameFaceIdCostAction(const shared_ptr<const FibUpdate>& lhs,
                            const shared_ptr<const FibUpdate>& rhs)
{
  if (lhs->name < rhs->name)
    {
      return true;
    }
  else if (lhs->name == rhs->name)
    {
      if (lhs->faceId < rhs->faceId)
        {
          return true;
        }
      else if (lhs->faceId == rhs->faceId)
        {
          if (lhs->cost < rhs->cost)
            {
              return true;
            }
          else if (lhs->cost == rhs->cost)
            {
              return lhs->action < rhs->action;
            }
        }
    }

  return false;
}

class FibUpdatesFixture : public nfd::tests::BaseFixture
{
public:
  void
  insertFaceEntry(const Name& name, uint64_t faceId, uint64_t origin, uint64_t cost, uint64_t flags)
  {
    rib::FaceEntry faceEntry;
    faceEntry.faceId = faceId;
    faceEntry.origin = origin;
    faceEntry.cost = cost;
    faceEntry.flags = flags;

    rib.insert(name, faceEntry);
  }

  void
  eraseFaceEntry(const Name& name, uint64_t faceId, uint64_t origin)
  {
    rib::FaceEntry faceEntry;
    faceEntry.faceId = faceId;
    faceEntry.origin = origin;

    rib.erase(name, faceEntry);
  }


  Rib::FibUpdateList
  getSortedFibUpdates()
  {
    Rib::FibUpdateList updates = rib.getFibUpdates();
    updates.sort(&compareNameFaceIdCostAction);
    return updates;
  }

public:
  rib::Rib rib;
};

} // namespace tests
} // namespace rib
} // namespace nfd

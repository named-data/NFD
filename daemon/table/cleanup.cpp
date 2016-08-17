/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "cleanup.hpp"

namespace nfd {

void
cleanupOnFaceRemoval(NameTree& nt, Fib& fib, Pit& pit, const Face& face)
{
  std::multimap<size_t, const name_tree::Entry*> maybeEmptyNtes;

  // visit FIB and PIT entries in one pass of NameTree enumeration
  for (const name_tree::Entry& nte : nt) {
    fib::Entry* fibEntry = nte.getFibEntry();
    if (fibEntry != nullptr) {
      fib.removeNextHop(*fibEntry, face);
    }

    for (const auto& pitEntry : nte.getPitEntries()) {
      pit.deleteInOutRecords(pitEntry.get(), face);
    }

    if (!nte.hasTableEntries()) {
      maybeEmptyNtes.emplace(nte.getName().size(), &nte);
    }
  }

  // try to erase longer names first, so that children are erased before parent is checked
  for (auto i = maybeEmptyNtes.rbegin(); i != maybeEmptyNtes.rend(); ++i) {
    nt.eraseIfEmpty(const_cast<name_tree::Entry*>(i->second), false);
  }

  BOOST_ASSERT(nt.size() == 0 ||
               std::none_of(nt.begin(), nt.end(),
                            [] (const name_tree::Entry& nte) { return nte.isEmpty(); }));
}

} // namespace nfd

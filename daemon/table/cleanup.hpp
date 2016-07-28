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

#ifndef NFD_DAEMON_TABLE_CLEANUP_HPP
#define NFD_DAEMON_TABLE_CLEANUP_HPP

#include "name-tree.hpp"
#include "fib.hpp"
#include "pit.hpp"

namespace nfd {

/** \brief cleanup tables when a face is destroyed
 *
 *  This function enumerates the NameTree, calls Fib::removeNextHop for each FIB entry,
 *  calls Pit::deleteInOutRecords for each PIT entry, and finally
 *  deletes any name tree entries that have become empty.
 *
 *  \note It's a design choice to let Fib and Pit classes decide what to do with each entry.
 *        This function is only responsible for implementing the enumeration procedure.
 *        The benefit of having this function instead of doing the enumeration in Fib and Pit
 *        classes is to perform both FIB and PIT cleanups in one pass of NameTree enumeration,
 *        so as to reduce performance overhead.
 */
void
cleanupOnFaceRemoval(NameTree& nt, Fib& fib, Pit& pit, const Face& face);

} // namespace nfd

#endif // NFD_DAEMON_TABLE_CLEANUP_HPP

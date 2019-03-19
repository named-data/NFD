/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#ifndef NFD_DAEMON_TABLE_CS_POLICY_PRIORITY_FIFO_HPP
#define NFD_DAEMON_TABLE_CS_POLICY_PRIORITY_FIFO_HPP

#include "cs-policy.hpp"

#include <list>

namespace nfd {
namespace cs {
namespace priority_fifo {

typedef std::list<iterator> Queue;
typedef Queue::iterator QueueIt;

enum QueueType {
  QUEUE_UNSOLICITED,
  QUEUE_STALE,
  QUEUE_FIFO,
  QUEUE_MAX
};

struct EntryInfo
{
  QueueType queueType;
  QueueIt queueIt;
  scheduler::EventId moveStaleEventId;
};

struct EntryItComparator
{
  bool
  operator()(const iterator& a, const iterator& b) const
  {
    return *a < *b;
  }
};

typedef std::map<iterator, EntryInfo*, EntryItComparator> EntryInfoMapFifo;

/** \brief Priority FIFO replacement policy
 *
 *  This policy maintains a set of cleanup queues to decide the eviction order of CS entries.
 *  The cleanup queues are three doubly linked lists that store Table iterators.
 *  The three queues keep track of unsolicited, stale, and fresh Data packet, respectively.
 *  Table iterator is placed into, removed from, and moved between suitable queues
 *  whenever an Entry is added, removed, or has other attribute changes.
 *  The Table iterator of an Entry should be in exactly one queue at any moment.
 *  Within each queue, the iterators are kept in first-in-first-out order.
 *  Eviction procedure exhausts the first queue before moving onto the next queue,
 *  in the order of unsolicited, stale, and fresh queue.
 */
class PriorityFifoPolicy : public Policy
{
public:
  PriorityFifoPolicy();

  ~PriorityFifoPolicy() override;

public:
  static const std::string POLICY_NAME;

private:
  void
  doAfterInsert(iterator i) override;

  void
  doAfterRefresh(iterator i) override;

  void
  doBeforeErase(iterator i) override;

  void
  doBeforeUse(iterator i) override;

  void
  evictEntries() override;

private:
  /** \brief evicts one entry
   *  \pre CS is not empty
   */
  void
  evictOne();

  /** \brief attaches the entry to an appropriate queue
   *  \pre the entry is not in any queue
   */
  void
  attachQueue(iterator i);

  /** \brief detaches the entry from its current queue
   *  \post the entry is not in any queue
   */
  void
  detachQueue(iterator i);

  /** \brief moves an entry from FIFO queue to STALE queue
   */
  void
  moveToStaleQueue(iterator i);

private:
  Queue m_queues[QUEUE_MAX];
  EntryInfoMapFifo m_entryInfoMap;
};

} // namespace priority_fifo

using priority_fifo::PriorityFifoPolicy;

} // namespace cs
} // namespace nfd

#endif // NFD_DAEMON_TABLE_CS_POLICY_PRIORITY_FIFO_HPP

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 *
 * Author: Ilya Moiseenko <iliamo@ucla.edu>
 */

#ifndef NFD_TABLE_CS_ENTRY_HPP
#define NFD_TABLE_CS_ENTRY_HPP

#include "common.hpp"
#include "core/time.hpp"
#include <ndn-cpp-dev/util/crypto.hpp>

namespace nfd {

namespace cs {

class Entry;

/** \brief represents a CS entry
 */
class Entry : noncopyable
{
public:
  typedef std::map<int, std::list< shared_ptr<Entry> >::iterator> LayerIterators;

  Entry(const Data& data, bool isUnsolicited = false);

  ~Entry();

  /** \brief returns the name of the Data packet stored in the CS entry
   *  \return{ NDN name }
   */
  const Name&
  getName() const;

  /** \brief Data packet is unsolicited if this particular NDN node
   *  did not receive an Interest packet for it, or the Interest packet has already expired
   *  \return{ True if the Data packet is unsolicited; otherwise False  }
   */
  bool
  isUnsolicited() const;

  /** \brief Returns True if CS entry was refreshed by a duplicate Data packet
   */
  bool
  wasRefreshedByDuplicate() const;

  /** \brief returns the absolute time when Data becomes expired
   *  \return{ Time (resolution up to milliseconds) }
   */
  const time::Point&
  getStaleTime() const;

  /** \brief returns the Data packet stored in the CS entry
   */
  const Data&
  getData() const;

  /** \brief changes the content of CS entry and recomputes digest
   */
  void
  setData(const Data& data);

  /** \brief changes the content of CS entry and modifies digest
   */
  void
  setData(const Data& data, const ndn::ConstBufferPtr& digest);

  /** \brief refreshes the time when Data becomes expired
   *  according to the current absolute time.
   */
  void
  updateStaleTime();

  /** \brief returns the digest of the Data packet stored in the CS entry.
   */
  const ndn::ConstBufferPtr&
  getDigest() const;

  /** \brief saves the iterator pointing to the CS entry on a specific layer of skip list
   */
  void
  setIterator(int layer, const LayerIterators::mapped_type& layerIterator);

  /** \brief removes the iterator pointing to the CS entry on a specific layer of skip list
   */
  void
  removeIterator(int layer);

  /** \brief returns the table containing <layer, iterator> pairs.
   */
  const LayerIterators&
  getIterators() const;

private:
  /** \brief prints <layer, iterator> pairs.
   */
  void
  printIterators() const;

private:
  time::Point m_staleAt;
  shared_ptr<const Data> m_dataPacket;

  bool m_isUnsolicited;
  bool m_wasRefreshedByDuplicate;

  Name m_nameWithDigest;

  mutable ndn::ConstBufferPtr m_digest;

  LayerIterators m_layerIterators;
};

} // namespace cs
} // namespace nfd

#endif // NFD_TABLE_CS_ENTRY_HPP

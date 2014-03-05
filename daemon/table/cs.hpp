/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 *
 * Author: Ilya Moiseenko <iliamo@ucla.edu>
 */

#ifndef NFD_TABLE_CS_HPP
#define NFD_TABLE_CS_HPP

#include "common.hpp"
#include "cs-entry.hpp"

namespace nfd {

typedef std::list< shared_ptr<cs::Entry> > SkipListLayer;
typedef std::list<SkipListLayer*> SkipList;

/** \brief represents Content Store
 */
class Cs : noncopyable
{
public:
  explicit
  Cs(int nMaxPackets = 65536); // ~500MB with average packet size = 8KB

  ~Cs();

  /** \brief inserts a Data packet
   *  This method does not consider the payload of the Data packet.
   *
   *  Packets are considered duplicate if the name matches.
   *  The new Data packet with the identical name, but a different payload
   *  is not placed in the Content Store
   *  \return{ whether the Data is added }
   */
  bool
  insert(const Data& data, bool isUnsolicited = false);

  /** \brief finds the best match Data for an Interest
   *  \return{ the best match, if any; otherwise 0 }
   */
  const Data*
  find(const Interest& interest) const;

  /** \brief deletes CS entry by the exact name
   */
  void
  erase(const Name& exactName);

  /** \brief sets maximum allowed size of Content Store (in packets)
   */
  void
  setLimit(size_t nMaxPackets);

  /** \brief returns maximum allowed size of Content Store (in packets)
   *  \return{ number of packets that can be stored in Content Store }
   */
  size_t
  getLimit() const;

  /** \brief returns current size of Content Store measured in packets
   *  \return{ number of packets located in Content Store }
   */
  size_t
  size() const;

protected:
  /** \brief removes one Data packet from Content Store based on replacement policy
   *  \return{ whether the Data was removed }
   */
  bool
  evictItem();

private:
  /** \brief returns True if the Content Store is at its maximum capacity
   *  \return{ True if Content Store is full; otherwise False}
   */
  bool
  isFull() const;

  /** \brief Computes the layer where new Content Store Entry is placed
   *
   *  Reference: "Skip Lists: A Probabilistic Alternative to Balanced Trees" by W.Pugh
   *  \return{ returns random layer (number) in a skip list}
   */
  int
  pickRandomLayer() const;

  /** \brief Inserts a new Content Store Entry in a skip list
   *  \return{ returns a pair containing a pointer to the CS Entry,
   *  and a flag indicating if the entry was newly created (True) or refreshed (False) }
   */
  std::pair< shared_ptr<cs::Entry>, bool>
  insertToSkipList(const Data& data, bool isUnsolicited = false);

  /** \brief Removes a specific CS Entry from all layers of a skip list
   *  \return{ returns True if CS Entry was succesfully removed and False if CS Entry was not found}
   */
  bool
  eraseFromSkipList(shared_ptr<cs::Entry> entry);

  /** \brief Prints contents of the skip list, starting from the top layer
   */
  void
  printSkipList() const;

  /** \brief Implements child selector (leftmost, rightmost, undeclared).
   *  Operates on the first layer of a skip list.
   *
   *  startingPoint must be less than Interest Name.
   *  startingPoint can be equal to Interest Name only when the item is in the begin() position.
   *
   *  Iterates toward greater Names, terminates when CS entry falls out of Interest prefix.
   *  When childSelector = leftmost, returns first CS entry that satisfies other selectors.
   *  When childSelector = rightmost, it goes till the end, and returns CS entry that satisfies
   *  other selectors. Returned CS entry is the leftmost child of the rightmost child.
   *  \return{ the best match, if any; otherwise 0 }
   */
  const Data*
  selectChild(const Interest& interest, SkipListLayer::iterator startingPoint) const;

  /** \brief checks if Content Store entry satisfies Interest selectors (MinSuffixComponents,
   *  MaxSuffixComponents, Implicit Digest, MustBeFresh)
   *  \return{ true if satisfies all selectors; false otherwise }
   */
  bool
  doesComplyWithSelectors(const Interest& interest, shared_ptr<cs::Entry> entry) const;

  /** \brief interprets minSuffixComponent and name lengths to understand if Interest contains
   *  implicit digest of the data
   *  \return{ True if Interest name contains digest; False otherwise }
   */
  bool
  recognizeInterestWithDigest(const Interest& interest, shared_ptr<cs::Entry> entry) const;

private:
  class StalenessComparator
  {
  public:
    bool
    operator() (shared_ptr<cs::Entry> entry1, shared_ptr<cs::Entry> entry2)
    {
      return entry1->getStaleTime() > entry2->getStaleTime();
    }
  };

  SkipList m_skipList;
  size_t m_nMaxPackets; //user defined maximum size of the Content Store in packets

  std::queue< shared_ptr<cs::Entry> > m_unsolicitedContent; //FIFO for unsolicited Data
  std::queue< shared_ptr<cs::Entry> > m_contentByArrival;   //FIFO index
  std::priority_queue< shared_ptr<cs::Entry>, std::vector< shared_ptr<cs::Entry> >, StalenessComparator> m_contentByStaleness; //index by staleness time
};

} // namespace nfd

#endif // NFD_TABLE_CS_HPP

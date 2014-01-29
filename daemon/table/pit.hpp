/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_PIT_HPP
#define NFD_TABLE_PIT_HPP

#include "pit-entry.hpp"
namespace nfd {
namespace pit {

/** \class DataMatchResult
 *  \brief an unordered iterable of all PIT entries matching Data
 *  This type shall support:
 *    iterator<shared_ptr<pit::Entry>> begin()
 *    iterator<shared_ptr<pit::Entry>> end()
 */
typedef std::vector<shared_ptr<pit::Entry> > DataMatchResult;

} // namespace pit

/** \class Pit
 *  \brief represents the PIT
 */
class Pit : noncopyable
{
public:
  Pit();
  
  ~Pit();
  
  /** \brief inserts a FIB entry for prefix
   *  If an entry for exact same prefix exists, that entry is returned.
   *  \return{ the entry, and true for new entry, false for existing entry }
   */
  std::pair<shared_ptr<pit::Entry>, bool>
  insert(const Interest& interest);
  
  /** \brief performs a Data match
   *  \return{ an iterable of all PIT entries matching data }
   */
  shared_ptr<pit::DataMatchResult>
  findAllDataMatches(const Data& data) const;
  
  /// removes a PIT entry
  void
  remove(shared_ptr<pit::Entry> pitEntry);

private:
  std::list<shared_ptr<pit::Entry> > m_table;
};

} // namespace nfd

#endif // NFD_TABLE_PIT_HPP

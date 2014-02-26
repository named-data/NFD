/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "measurements.hpp"
#include <algorithm>
#include "fib-entry.hpp"
#include "pit-entry.hpp"

namespace nfd {

const time::Duration Measurements::s_defaultLifetime = time::seconds(4);

Measurements::Measurements()
{
}

Measurements::~Measurements()
{
}

shared_ptr<measurements::Entry>
Measurements::get(const Name& name)
{
  std::map<Name, shared_ptr<measurements::Entry> >::iterator it = m_table.find(name);
  if (it != m_table.end()) {
    return it->second;
  }

  shared_ptr<measurements::Entry> entry = make_shared<measurements::Entry>(name);
  std::pair<std::map<Name, shared_ptr<measurements::Entry> >::iterator, bool> pair =
    m_table.insert(std::make_pair(name, entry));
  this->extendLifetimeInternal(pair.first, s_defaultLifetime);

  return entry;
}

shared_ptr<measurements::Entry>
Measurements::get(const fib::Entry& fibEntry)
{
  return this->get(fibEntry.getPrefix());
}

shared_ptr<measurements::Entry>
Measurements::get(const pit::Entry& pitEntry)
{
  return this->get(pitEntry.getName());
}

shared_ptr<measurements::Entry>
Measurements::getParent(shared_ptr<measurements::Entry> child)
{
  if (child->getName().size() == 0) {
    return shared_ptr<measurements::Entry>();
  }

  return this->get(child->getName().getPrefix(-1));
}

//shared_ptr<fib::Entry>
//Measurements::findLongestPrefixMatch(const Name& name) const
//{
//}
//
//shared_ptr<fib::Entry>
//Measurements::findExactMatch(const Name& name) const
//{
//}

void
Measurements::extendLifetime(measurements::Entry& entry, const time::Duration& lifetime)
{
  std::map<Name, shared_ptr<measurements::Entry> >::iterator it =
      m_table.find(entry.getName());
  BOOST_ASSERT(it != m_table.end());

  this->extendLifetimeInternal(it, lifetime);
}

void
Measurements::extendLifetimeInternal(
    std::map<Name, shared_ptr<measurements::Entry> >::iterator it,
    const time::Duration& lifetime)
{
  shared_ptr<measurements::Entry>& entry = it->second;

  time::Point expiry = time::now() + lifetime;
  if (entry->m_expiry >= expiry) { // has longer lifetime, not extending
    return;
  }

  scheduler::cancel(entry->m_cleanup);
  entry->m_expiry = expiry;
  entry->m_cleanup = scheduler::schedule(lifetime,
                         bind(&Measurements::cleanup, this, it));
}

void
Measurements::cleanup(std::map<Name, shared_ptr<measurements::Entry> >::iterator it)
{
  m_table.erase(it);
}

} // namespace nfd

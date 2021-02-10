/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2021,  Regents of the University of California,
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

#ifndef NFD_TESTS_TEST_COMMON_HPP
#define NFD_TESTS_TEST_COMMON_HPP

#include "core/common.hpp"
#include "tests/boost-test.hpp"

#include <ndn-cxx/prefix-announcement.hpp>

#ifdef NFD_HAVE_PRIVILEGE_DROP_AND_ELEVATE
#include <unistd.h>
#define SKIP_IF_NOT_SUPERUSER() \
  do { \
    if (::geteuid() != 0) { \
      BOOST_WARN_MESSAGE(false, "skipping assertions that require superuser privileges"); \
      return; \
    } \
  } while (false)
#else
#define SKIP_IF_NOT_SUPERUSER()
#endif // NFD_HAVE_PRIVILEGE_DROP_AND_ELEVATE

namespace nfd {
namespace tests {

/**
 * \brief Create an Interest
 */
shared_ptr<Interest>
makeInterest(const Name& name, bool canBePrefix = false,
             optional<time::milliseconds> lifetime = nullopt,
             optional<Interest::Nonce> nonce = nullopt);

/**
 * \brief Create a Data with a null (i.e., empty) signature
 *
 * If a "real" signature is desired, use KeyChainFixture and sign again with `m_keyChain`.
 */
shared_ptr<Data>
makeData(const Name& name);

/**
 * \brief Add a null signature to \p data
 */
Data&
signData(Data& data);

/**
 * \brief Add a null signature to \p data
 */
inline shared_ptr<Data>
signData(shared_ptr<Data> data)
{
  signData(*data);
  return data;
}

/**
 * \brief Create a Nack
 */
lp::Nack
makeNack(Interest interest, lp::NackReason reason);

/**
 * \brief Replace a name component in a packet
 * \param[inout] pkt the packet
 * \param index the index of the name component to replace
 * \param args arguments to name::Component constructor
 */
template<typename Packet, typename ...Args>
void
setNameComponent(Packet& pkt, ssize_t index, Args&& ...args)
{
  Name name = pkt.getName();
  name.set(index, name::Component(std::forward<Args>(args)...));
  pkt.setName(name);
}

/**
 * \brief Create a prefix announcement without signing
 */
ndn::PrefixAnnouncement
makePrefixAnn(const Name& announcedName, time::milliseconds expiration,
              optional<ndn::security::ValidityPeriod> validity = nullopt);

/**
 * \brief Create a prefix announcement without signing
 * \param announcedName announced name
 * \param expiration expiration period
 * \param validityFromNow validity period, relative from now
 */
ndn::PrefixAnnouncement
makePrefixAnn(const Name& announcedName, time::milliseconds expiration,
              std::pair<time::seconds, time::seconds> validityFromNow);

/**
 * \brief Sign a prefix announcement
 */
ndn::PrefixAnnouncement
signPrefixAnn(ndn::PrefixAnnouncement&& pa, ndn::KeyChain& keyChain,
              const ndn::security::SigningInfo& si = ndn::security::SigningInfo(),
              optional<uint64_t> version = nullopt);

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_TEST_COMMON_HPP

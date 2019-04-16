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

#ifndef NFD_TESTS_DAEMON_TABLE_CS_FIXTURE_HPP
#define NFD_TESTS_DAEMON_TABLE_CS_FIXTURE_HPP

#include "table/cs.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"

#include <cstring>

namespace nfd {
namespace cs {
namespace tests {

using namespace nfd::tests;

#define CHECK_CS_FIND(expected) find([&] (uint32_t found) { BOOST_CHECK_EQUAL(expected, found); });

class CsFixture : public GlobalIoTimeFixture
{
protected:
  Name
  insert(uint32_t id, const Name& name, const std::function<void(Data&)>& modifyData = nullptr,
         bool isUnsolicited = false)
  {
    auto data = makeData(name);
    data->setContent(reinterpret_cast<const uint8_t*>(&id), sizeof(id));

    if (modifyData != nullptr) {
      modifyData(*data);
    }

    data->wireEncode();
    cs.insert(*data, isUnsolicited);

    return data->getFullName();
  }

  Interest&
  startInterest(const Name& name)
  {
    interest = make_shared<Interest>(name);
    interest->setCanBePrefix(false);
    return *interest;
  }

  void
  find(const std::function<void(uint32_t)>& check)
  {
    bool hasResult = false;
    cs.find(*interest,
            [&] (const Interest& interest, const Data& data) {
              hasResult = true;
              const Block& content = data.getContent();
              uint32_t found = 0;
              std::memcpy(&found, content.value(), sizeof(found));
              check(found);
            },
            bind([&] {
              hasResult = true;
              check(0);
            }));

    // current Cs::find implementation is synchronous
    BOOST_CHECK(hasResult);
  }

  size_t
  erase(const Name& prefix, size_t limit)
  {
    optional<size_t> nErased;
    cs.erase(prefix, limit, [&] (size_t nErased1) { nErased = nErased1; });

    // current Cs::erase implementation is synchronous
    // if callback was not invoked, bad_optional_access would occur
    return *nErased;
  }

protected:
  Cs cs;
  shared_ptr<Interest> interest;
};

} // namespace tests
} // namespace cs
} // namespace nfd

#endif // NFD_TESTS_DAEMON_TABLE_CS_FIXTURE_HPP

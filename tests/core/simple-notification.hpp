/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#ifndef NFD_TESTS_CORE_SIMPLE_NOTIFICATION_HPP
#define NFD_TESTS_CORE_SIMPLE_NOTIFICATION_HPP

#include "common.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

class SimpleNotification
{
public:
  SimpleNotification()
  {
  }

  SimpleNotification(const std::string& message)
    : m_message(message)
  {
  }

  ~SimpleNotification()
  {
  }

  Block
  wireEncode() const
  {
    ndn::EncodingBuffer buffer;
    prependStringBlock(buffer, 0x8888, m_message);
    return buffer.block();
  }

  void
  wireDecode(const Block& block)
  {
    m_message.assign(reinterpret_cast<const char*>(block.value()),
                     block.value_size());

    // error for testing
    if (!m_message.empty() && m_message[0] == '\x07')
      BOOST_THROW_EXCEPTION(tlv::Error("0x07 error"));
  }

public:
  const std::string&
  getMessage() const
  {
    return m_message;
  }

  void
  setMessage(const std::string& message)
  {
    m_message = message;
  }

private:
  std::string m_message;
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_CORE_SIMPLE_NOTIFICATION_HPP

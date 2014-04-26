/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#ifndef NFD_TESTS_DAEMON_FACE_PACKET_DATASETS_HPP
#define NFD_TESTS_DAEMON_FACE_PACKET_DATASETS_HPP

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

class CorruptedInterestWithLocalControlHeader
{
public:
  typedef std::vector<ndn::Buffer> Container;

  static std::string
  getName()
  {
    return "CorruptedInterestWithLocalControlHeader";
  }

  CorruptedInterestWithLocalControlHeader()
  {
    static const uint8_t interest[] = {
      0x50, 0x22, 0x51, 0x81, 0x0a, 0x05, 0x1d, 0x07, 0x14, 0x08, 0x05, 0x6c, 0x6f, 0x63, 0x61,
      0x6c, 0x08, 0x03, 0x6e, 0x64, 0x6e, 0x08, 0x06, 0x70, 0x72, 0x65, 0x66, 0x69, 0x78, 0x09,
      0x02, 0x12, 0x00, 0x0a, 0x01, 0x01
    };

    data.push_back(ndn::Buffer(interest, sizeof(interest)));
  }
public:
  Container data;
};

class CorruptedInterest
{
public:
  typedef std::vector<ndn::Buffer> Container;

  static std::string
  getName()
  {
    return "CorruptedInterest";
  }

  CorruptedInterest()
  {
    static const uint8_t interest[] = {
      0x05, 0x1d, 0x07, 0x84, 0x08, 0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x08, 0x03, 0x6e, 0x64,
      0x6e, 0x08, 0x06, 0x70, 0x72, 0x65, 0x66, 0x69, 0x78, 0x09, 0x02, 0x12, 0x00, 0x0a, 0x01,
      0x01
    };

    data.push_back(ndn::Buffer(interest, sizeof(interest)));
  }
public:
  Container data;
};


typedef boost::mpl::vector< CorruptedInterestWithLocalControlHeader,
                            CorruptedInterest> CorruptedPackets;

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_PACKET_DATASETS_HPP

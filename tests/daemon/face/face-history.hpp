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

#ifndef NFD_TESTS_DAEMON_FACE_FACE_HISTORY_HPP
#define NFD_TESTS_DAEMON_FACE_FACE_HISTORY_HPP

#include "face/face.hpp"

namespace nfd {
namespace tests {

/** \brief captures signals from Face
 */
class FaceHistory : noncopyable
{
public:
  explicit
  FaceHistory(Face& face)
    : m_limitedIo(nullptr)
  {
    this->construct(face);
  }

  FaceHistory(Face& face, LimitedIo& limitedIo)
    : m_limitedIo(&limitedIo)
  {
    this->construct(face);
  }

private:
  void
  construct(Face& face)
  {
    m_receiveInterestConn = face.onReceiveInterest.connect([this] (const Interest& interest) {
      this->receivedInterests.push_back(interest);
      this->afterOp();
    });
    m_receiveDataConn = face.onReceiveData.connect([this] (const Data& data) {
      this->receivedData.push_back(data);
      this->afterOp();
    });
    m_failConn = face.onFail.connect([this] (const std::string& reason) {
      this->failures.push_back(reason);
      this->afterOp();
    });
  }

  void
  afterOp()
  {
    if (m_limitedIo != nullptr) {
      m_limitedIo->afterOp();
    }
  }

public:
  std::vector<Interest> receivedInterests;
  std::vector<Data> receivedData;
  std::vector<std::string> failures;

private:
  LimitedIo* m_limitedIo;
  signal::ScopedConnection m_receiveInterestConn;
  signal::ScopedConnection m_receiveDataConn;
  signal::ScopedConnection m_failConn;
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_FACE_HISTORY_HPP

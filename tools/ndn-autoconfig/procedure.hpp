/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#ifndef NFD_TOOLS_NDN_AUTOCONFIG_PROCEDURE_HPP
#define NFD_TOOLS_NDN_AUTOCONFIG_PROCEDURE_HPP

#include "stage.hpp"
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/security/key-chain.hpp>

namespace ndn {
namespace tools {
namespace autoconfig {

struct Options
{
  std::string ndnFchUrl = "http://ndn-fch.named-data.net"; ///< HTTP base URL of NDN-FCH service
};

class Procedure : noncopyable
{
public:
  Procedure(Face& face, KeyChain& keyChain);

  virtual
  ~Procedure() = default;

  void
  initialize(const Options& options);

  /** \brief run HUB discovery procedure once
   */
  void
  runOnce();

  boost::asio::io_service&
  getIoService()
  {
    return m_face.getIoService();
  }

private:
  VIRTUAL_WITH_TESTS void
  makeStages(const Options& options);

  void
  connect(const FaceUri& hubFaceUri);

  void
  registerPrefixes(uint64_t hubFaceId, size_t index = 0);

public:
  /** \brief signal when procedure completes
   *
   *  Argument indicates whether the procedure succeeds (true) or fails (false).
   */
  util::Signal<Procedure, bool> onComplete;

PROTECTED_WITH_TESTS_ELSE_PRIVATE:
  std::vector<unique_ptr<Stage>> m_stages;

private:
  Face& m_face;
  KeyChain& m_keyChain;
  nfd::Controller m_controller;
};

} // namespace autoconfig
} // namespace tools
} // namespace ndn

#endif // NFD_TOOLS_NDN_AUTOCONFIG_PROCEDURE_HPP

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

#ifndef NFD_TOOLS_NDN_AUTOCONFIG_BASE_HPP
#define NFD_TOOLS_NDN_AUTOCONFIG_BASE_HPP

#include "common.hpp"

#include <boost/noncopyable.hpp>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/management/nfd-controller.hpp>
#include <ndn-cxx/management/nfd-face-status.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <ndn-cxx/util/face-uri.hpp>

namespace ndn {
namespace tools {
namespace autoconfig {

/**
 * @brief Base class for discovery stages
 */
class Base : boost::noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  /**
   * @brief Callback to be called when the stage fails
   */
  typedef std::function<void(const std::string&)> NextStageCallback;

  /**
   * @brief Start the stage
   */
  virtual void
  start() = 0;

protected:
  /**
   * @brief Initialize variables and create nfd::Controller instance
   * @param face Face to be used for all operations (e.g., will send registration commands)
   * @param keyChain KeyChain object
   * @param nextStageOnFailure Callback to be called after the stage failed
   */
  Base(Face& face, KeyChain& keyChain, const NextStageCallback& nextStageOnFailure);

  /**
   * @brief Attempt to connect to local hub using the \p uri FaceUri
   * @throw Base::Error when failed to establish the tunnel
   */
  void
  connectToHub(const std::string& uri);

private:
  void
  onCanonizeSuccess(const util::FaceUri& canonicalUri);

  void
  onCanonizeFailure(const std::string& reason);

  void
  onHubConnectSuccess(const nfd::ControlParameters& resp);

  void
  onHubConnectError(uint32_t code, const std::string& error);

  void
  registerPrefix(const Name& prefix, uint64_t faceId);

  void
  onPrefixRegistrationSuccess(const nfd::ControlParameters& commandSuccessResult);

  void
  onPrefixRegistrationError(uint32_t code, const std::string& error);

protected:
  Face& m_face;
  KeyChain& m_keyChain;
  nfd::Controller m_controller;
  NextStageCallback m_nextStageOnFailure;
};

} // namespace autoconfig
} // namespace tools
} // namespace ndn

#endif // NFD_TOOLS_NDN_AUTOCONFIG_BASE_HPP

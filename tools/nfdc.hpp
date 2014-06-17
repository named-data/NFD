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

#ifndef NFD_TOOLS_NFDC_HPP
#define NFD_TOOLS_NFDC_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/management/nfd-controller.hpp>

namespace nfdc {


using namespace ndn::nfd;

class Nfdc : boost::noncopyable
{
public:

  static const ndn::time::milliseconds DEFAULT_EXPIRATION_PERIOD;
  static const uint64_t DEFAULT_COST;

  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  explicit
  Nfdc(ndn::Face& face);

  ~Nfdc();

  bool
  dispatch(const std::string& cmd);

  /**
   * \brief Adds a nexthop to a FIB entry
   *
   * If the FIB entry does not exist, it is inserted automatically
   *
   * cmd format:
   *  [-c cost]  name faceId|faceUri
   *
   */
  void
  fibAddNextHop();

  /**
   * \brief Adds a nexthop to a FIB entry using faceId provided in the faceCreateResult
   *
   */
  void
  fibAddNextHop(const ControlParameters& faceCreateResult);

  /**
   * \brief Removes a nexthop from an existing FIB entry
   *
   * If the last nexthop record in a FIB entry is removed, the FIB entry is also deleted
   *
   * cmd format:
   *  name faceId
   *
   */
  void
  fibRemoveNextHop();

  /**
   * \brief Registers name to the given faceId or faceUri
   *
   * cmd format:
   *  [-I] [-C] [-c cost] name faceId|faceUri
   */
  void
  ribRegisterPrefix();

  /**
   * \brief Registers name to the faceId provided in faceCreateResult
   *
   */
  void
  ribRegisterPrefix(const ControlParameters& faceCreateResult);

  /**
   * \brief Unregisters name from the given faceId
   *
   * cmd format:
   *  name faceId
   */
  void
  ribUnregisterPrefix();

  /**
   * \brief Creates new face
   *
   * This command allows creation of UDP unicast and TCP faces only
   *
   * cmd format:
   *  uri
   *
   */
  void
  faceCreate();

  /**
   * \brief Destroys face
   *
   * cmd format:
   *  faceId|faceUri
   *
   */
  void
  faceDestroy();

  /**
   * \brief Destroys face based on faceId provided in faceCreateResult
   *
   * cmd format:
   *  faceId|faceUri
   *
   */
  void
  faceDestroy(const ControlParameters& faceCreateResult);

  /**
   * \brief Sets the strategy for a namespace
   *
   * cmd format:
   *  name strategy
   *
   */
  void
  strategyChoiceSet();

  /**
   * \brief Unset the strategy for a namespace
   *
   * cmd format:
   *  name strategy
   *
   */
  void
  strategyChoiceUnset();

private:

  void
  onSuccess(const ControlParameters& commandSuccessResult,
            const std::string& message);

  void
  onError(uint32_t code, const std::string& error, const std::string& message);

public:
  const char* m_programName;

  // command parameters without leading 'cmd' component
  const char* const* m_commandLineArguments;
  int m_nOptions;
  uint64_t m_flags;
  uint64_t m_cost;
  uint64_t m_faceId;
  uint64_t m_origin;
  ndn::time::milliseconds m_expires;
  std::string m_name;


private:
  Controller m_controller;
};

} // namespace nfdc

#endif // NFD_TOOLS_NFDC_HPP

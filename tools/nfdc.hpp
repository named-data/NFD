/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#ifndef NFD_TOOLS_NFDC_HPP
#define NFD_TOOLS_NFDC_HPP

#include <ndn-cpp-dev/face.hpp>
#include <ndn-cpp-dev/management/controller.hpp>
#include <ndn-cpp-dev/management/nfd-controller.hpp>
#include <vector>

namespace nfdc {

using namespace ndn::nfd;

class Nfdc
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

  explicit
  Nfdc(ndn::Face& face);

  ~Nfdc();

  bool
  dispatch(const std::string& cmd,
           const char* cmdOptions[],
           int nOptions);

  /**
   * \brief Adds a nexthop to a FIB entry.
   *
   * If the FIB entry does not exist, it is inserted automatically
   *
   * cmd format:
   *   name
   *
   * \param cmdOptions Add next hop command parameters without leading 'add-nexthop' component
   */
  void
  fibAddNextHop(const char* cmdOptions[], bool hasCost);

  /**
   * \brief Removes a nexthop from an existing FIB entry
   *
   * If the last nexthop record in a FIB entry is removed, the FIB entry is also deleted
   *
   * cmd format:
   *   name faceId
   *
   * \param cmdOptions Remove next hop command parameters without leading
   *                   'remove-nexthop' component
   */
  void
  fibRemoveNextHop(const char* cmdOptions[]);

  /**
   * \brief create new face
   *
   * This command allows creation of UDP unicast and TCP faces only.
   *
   * cmd format:
   *   uri
   *
   * \param cmdOptions Create face command parameters without leading 'create' component
   */
  void
  faceCreate(const char* cmdOptions[]);

  /**
   * \brief destroy a face
   *
   * cmd format:
   *   faceId
   *
   * \param cmdOptions Destroy face command parameters without leading 'destroy' component
   */
  void
  faceDestroy(const char* cmdOptions[]);

  /**
   * \brief Set the strategy for a namespace
   *
   *
   * cmd format:
   *   name strategy
   *
   * \param cmdOptions Set strategy choice command parameters without leading
   *                   'set-strategy' component
   */
  void
  strategyChoiceSet(const char* cmdOptions[]);

  /**
   * \brief Unset the strategy for a namespace
   *
   *
   * cmd format:
   *   name strategy
   *
   * \param cmdOptions Unset strategy choice command parameters without leading
   *                   'unset-strategy' component
   */
  void
  strategyChoiceUnset(const char* cmdOptions[]);

private:
  void
  onSuccess(const ControlParameters& parameters,
            const std::string& message);

  void
  onError(uint32_t code, const std::string& error, const std::string& message);

public:
  const char* m_programName;

private:
  Controller m_controller;
};

} // namespace nfdc

#endif // NFD_TOOLS_NFDC_HPP

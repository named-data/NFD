/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#ifndef NFD_TOOLS_NFDC_EXECUTE_COMMAND_HPP
#define NFD_TOOLS_NFDC_EXECUTE_COMMAND_HPP

#include "command-arguments.hpp"
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/mgmt/nfd/command-options.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/mgmt/nfd/control-command.hpp>
#include <ndn-cxx/mgmt/nfd/control-parameters.hpp>
#include <ndn-cxx/mgmt/nfd/control-response.hpp>
#include <ndn-cxx/mgmt/nfd/status-dataset.hpp>
#include <ndn-cxx/security/key-chain.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

using ndn::Face;
using ndn::KeyChain;
using ndn::nfd::ControlParameters;
using ndn::nfd::ControlResponse;
using ndn::nfd::Controller;

/** \brief context for command execution
 */
class ExecuteContext
{
public:
  /** \return timeout for each step
   */
  time::nanoseconds
  getTimeout() const;

  ndn::nfd::CommandOptions
  makeCommandOptions() const;

  /** \return handler for command execution failure
   *  \param commandName command name used in error message (present continuous tense)
   */
  Controller::CommandFailCallback
  makeCommandFailureHandler(const std::string& commandName);

  /** \return handler for dataset retrieval failure
   *  \param datasetName dataset name used in error message (noun phrase)
   */
  Controller::DatasetFailCallback
  makeDatasetFailureHandler(const std::string& datasetName);

public:
  const std::string& noun;
  const std::string& verb;
  const CommandArguments& args;

  int exitCode; ///< program exit code
  std::ostream& out; ///< output stream
  std::ostream& err; ///< error stream

  Face& face;
  KeyChain& keyChain;
  ///\todo validator
  Controller& controller;
};

/** \brief a function to execute a command
 */
using ExecuteCommand = std::function<void(ExecuteContext& ctx)>;

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_EXECUTE_COMMAND_HPP

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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
#include <ndn-cxx/security/key-chain.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

using ndn::Face;
using ndn::KeyChain;

/** \brief context for command execution
 */
struct ExecuteContext
{
  const std::string& noun;
  const std::string& verb;
  const CommandArguments& args;

  Face& face;
  KeyChain& keyChain;
  ///\todo validator
};

/** \brief a function to execute a command
 *  \return exit code
 */
typedef std::function<int(ExecuteContext& ctx)> ExecuteCommand;

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_EXECUTE_COMMAND_HPP

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

#include "execute-command.hpp"

namespace nfd {
namespace tools {
namespace nfdc {

time::nanoseconds
ExecuteContext::getTimeout() const
{
  return time::seconds(4);
}

ndn::nfd::CommandOptions
ExecuteContext::makeCommandOptions() const
{
  return ndn::nfd::CommandOptions()
           .setTimeout(time::duration_cast<time::milliseconds>(this->getTimeout()));
}

Controller::CommandFailCallback
ExecuteContext::makeCommandFailureHandler(const std::string& commandName)
{
  return [=] (const ControlResponse& resp) {
    this->exitCode = 1;
    this->err << "Error " << resp.getCode() << " when " << commandName << ": " << resp.getText() << '\n';
  };
}

Controller::DatasetFailCallback
ExecuteContext::makeDatasetFailureHandler(const std::string& datasetName)
{
  return [=] (uint32_t code, const std::string& reason) {
    this->exitCode = 1;
    this->err << "Error " << code << " when fetching " << datasetName << ": " << reason << '\n';
  };
}

} // namespace nfdc
} // namespace tools
} // namespace nfd

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#ifndef NFD_TOOLS_NFDC_MODULE_HPP
#define NFD_TOOLS_NFDC_MODULE_HPP

#include <ndn-cxx/mgmt/nfd/command-options.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>

#include <ostream>

namespace nfd::tools::nfdc {

using ndn::nfd::CommandOptions;

/**
 * \brief Provides access to an NFD management module.
 * \note This type is an interface. It should not have any member fields.
 */
class Module
{
public:
  virtual
  ~Module() = default;

  /** \brief Collect status from NFD.
   *  \pre no other fetchStatus is in progress
   *  \param controller a controller through which StatusDataset can be requested
   *  \param onSuccess invoked when status has been collected into this instance
   *  \param onFailure passed to controller.fetch
   *  \param options passed to controller.fetch
   */
  virtual void
  fetchStatus(ndn::nfd::Controller& controller,
              const std::function<void()>& onSuccess,
              const ndn::nfd::DatasetFailureCallback& onFailure,
              const CommandOptions& options) = 0;

  /** \brief Format collected status as XML.
   *  \pre fetchStatus has been successful
   *  \param os output stream
   */
  virtual void
  formatStatusXml(std::ostream& os) const = 0;

  /** \brief Format collected status as text.
   *  \pre fetchStatus has been successful
   *  \param os output stream
   */
  virtual void
  formatStatusText(std::ostream& os) const = 0;
};

} // namespace nfd::tools::nfdc

#endif // NFD_TOOLS_NFDC_MODULE_HPP
